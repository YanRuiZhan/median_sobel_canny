// Independent benchmark; production sources are linked without modification.
#include "icm/median_blur.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using icm::Image;
using icm::Matrix;
using Clock = std::chrono::steady_clock;

static Matrix sorted_channel(const Matrix& raw, int k, bool reference = false) {
    const int h = static_cast<int>(raw.size());
    const int w = static_cast<int>(raw[0].size());
    const int r = k / 2;
    Matrix out(h, std::vector<int>(w));
    // Reuse the window buffer rather than allocate for every pixel.
    std::vector<int> window(k * k);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int n = 0;
            for (int dy = -r; dy <= r; ++dy)
                for (int dx = -r; dx <= r; ++dx)
                    window[n++] = raw[std::clamp(y + dy, 0, h - 1)]
                                     [std::clamp(x + dx, 0, w - 1)];
            if (reference) {
                std::sort(window.begin(), window.end());
            } else {
                for (int i = 1; i < n; ++i) {
                    const int key = window[i];
                    int j = i;
                    while (j > 0 && window[j - 1] > key) {
                        window[j] = window[j - 1];
                        --j;
                    }
                    window[j] = key;
                }
            }
            out[y][x] = window[n / 2];
        }
    }
    return out;
}

// Same split/filter/merge and alpha semantics as icm::median_blur.
static Image sorted_image(const Image& raw, int k, bool reference = false) {
    Image img;
    img.w = raw.w;
    img.h = raw.h;
    img.ch = raw.ch;
    img.data = Matrix(raw.h, std::vector<int>(raw.w * raw.ch, 0));
    const bool has_alpha = raw.ch == 2 || raw.ch == 4;
    Matrix channel(raw.h, std::vector<int>(raw.w, 0));
    for (int c = 0; c < raw.ch; ++c) {
        if (has_alpha && c == raw.ch - 1) {
            for (int y = 0; y < raw.h; ++y)
                for (int x = 0; x < raw.w; ++x)
                    img.data[y][x * raw.ch + c] = raw.data[y][x * raw.ch + c];
            continue;
        }
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                channel[y][x] = raw.data[y][x * raw.ch + c];
        Matrix filtered = sorted_channel(channel, k, reference);
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                img.data[y][x * raw.ch + c] = filtered[y][x];
    }
    return img;
}

static void equal(const Image& a, const Image& b) {
    if (a.w != b.w || a.h != b.h || a.ch != b.ch || a.data != b.data)
        throw std::runtime_error("Pixel comparison failed");
}

static void self_test() {
    std::mt19937 rng(20261020);
    int tests = 0;
    for (int ch : {1, 2, 3, 4}) {
        for (auto shape : {std::pair<int, int>{1, 1}, {1, 7}, {7, 1}, {8, 9}}) {
            for (int pattern = 0; pattern < 4; ++pattern) {
                Image raw{shape.first, shape.second, ch,
                          Matrix(shape.second, std::vector<int>(shape.first * ch))};
                for (int y = 0; y < raw.h; ++y)
                    for (int x = 0; x < raw.w * ch; ++x)
                        raw.data[y][x] = pattern == 0 ? 0 : pattern == 1 ? 255 :
                            pattern == 2 ? ((x + y) % 2) * 255 : rng() % 256;
                for (int k : {1, 3, 5, 15, 31}) {
                    auto ref = sorted_image(raw, k, true);
                    equal(ref, sorted_image(raw, k));
                    equal(ref, icm::median_blur(raw, k));
                    ++tests;
                }
            }
        }
    }
    std::cerr << "SELF_TEST_PASS cases=" << tests << '\n';
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") {
            self_test();
            return 0;
        }
        if (argc < 4) {
            std::cerr << "Usage: benchmark image repetitions odd_kernel...\n";
            return 1;
        }
        const int repeats = std::stoi(argv[2]);
        if (repeats < 1) throw std::runtime_error("Invalid repetitions");
        Image raw = icm::read_image(argv[1]);
        if (raw.data.empty()) throw std::runtime_error("Image load failed");
        std::cout << std::fixed << std::setprecision(6);
        for (int arg = 3; arg < argc; ++arg) {
            const int k = std::stoi(argv[arg]);
            if (k < 1 || k > 99 || k % 2 == 0)
                throw std::runtime_error("Kernel must be odd, 1..99");
            // One untimed warm-up of each implementation for every kernel.
            { auto a = icm::median_blur(raw, k); auto b = sorted_image(raw, k); equal(a, b); }
            for (int rep = 0; rep < repeats; ++rep) {
                Image hist, insertion;
                double hist_ms = 0, insertion_ms = 0;
                auto run_hist = [&] {
                    const auto t = Clock::now();
                    hist = icm::median_blur(raw, k);
                    hist_ms = std::chrono::duration<double, std::milli>(Clock::now() - t).count();
                };
                auto run_insertion = [&] {
                    const auto t = Clock::now();
                    insertion = sorted_image(raw, k);
                    insertion_ms = std::chrono::duration<double, std::milli>(Clock::now() - t).count();
                };
                if (rep % 2 == 0) { run_hist(); run_insertion(); }
                else { run_insertion(); run_hist(); }
                equal(hist, insertion); // Outside timing; includes alpha and all border pixels.
                std::cout << argv[1] << ',' << raw.w << ',' << raw.h << ',' << raw.ch
                          << ',' << k << ',' << rep + 1 << ',' << hist_ms << ','
                          << insertion_ms << ",PASS\n" << std::flush;
            }
        }
        std::cerr << "BENCHMARK_PASS " << argv[1] << '\n';
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
