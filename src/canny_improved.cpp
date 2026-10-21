#include "icm/canny.hpp"
#include "icm/convolution.hpp"
#include "icm/kernels.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace icm {

    // 与原 Canny 共用 BT.601 灰度转换。
    Matrix to_gray(const Image& raw);

    // 双边滤波
    Matrix bilateral_blur(const Matrix& gray, int window, double sigma_space, double sigma_color) {
        const int h = static_cast<int>(gray.size()), w = static_cast<int>(gray[0].size());
        const int radius = window / 2;
        std::vector<std::vector<double>> spatial(window, std::vector<double>(window));
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx)
                spatial[dy + radius][dx + radius] =
                    std::exp(-0.5 * (double(dx) * dx + double(dy) * dy) / sigma_space / sigma_space);

        Matrix blurred(h, std::vector<int>(w, 0));
        // 复制边界；空间权重与灰度相似度权重共同决定平滑程度。
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                double sum = 0.0, weights = 0.0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        int value = gray[std::clamp(y + dy, 0, h - 1)][std::clamp(x + dx, 0, w - 1)];
                        double diff = double(value) - gray[y][x];
                        double weight = spatial[dy + radius][dx + radius] *
                            std::exp(-0.5 * (diff / sigma_color) * (diff / sigma_color));
                        sum += weight * value;
                        weights += weight;
                    }
                }
                blurred[y][x] = static_cast<int>(std::lround(sum / weights));
            }
        }
        return blurred;
    }

    Image canny_edge_improved(const Image& src, int low_threshold, int high_threshold,
                              int window, double sigma_space, double sigma_color) {
        const int h = src.h, w = src.w;
        Matrix blurred = bilateral_blur(to_gray(src), window, sigma_space, sigma_color);


    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);
    Matrix g45 = conv(blurred, sob45);
    Matrix g135 = conv(blurred, sob135);

    // 梯度幅值，四个梯度方向的二范数
    std::vector<std::vector<double>> magnitude(h, std::vector<double>(w, 0.0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            double dx = gx[y][x], dy = gy[y][x];
            double d45 = g45[y][x], d135 = g135[y][x];
            magnitude[y][x] = std::sqrt(dx * dx + dy * dy + d45 * d45 + d135 * d135);
        }

    std::vector<std::vector<unsigned char>> flag(h, std::vector<unsigned char>(w, 0));
    std::queue<std::pair<int, int>> queue;

    // NMS
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            // 3.14159 即 π，用于弧度转角度
            double angle = std::atan2(double(gy[y][x]), double(gx[y][x])) * 180.0 / 3.14159;

            if (angle < 0.0) angle += 180.0;
            int dx = 0, dy = 0;
            if (angle < 22.5 || angle >= 157.5) dx = 1;
            else if (angle < 67.5) { dx = 1; dy = 1; }
            else if (angle < 112.5) dy = 1;
            else { dx = -1; dy = 1; }
            double m = magnitude[y][x];
            // 即使阈值为零，也不能将被抑制点或平坦区域判成边缘。
            if (m <= 0.0 || m < magnitude[y - dy][x - dx] || m < magnitude[y + dy][x + dx])
                continue;
            if (m >= high_threshold) {
                flag[y][x] = 1;
                queue.push({y, x});
            } else if (m >= low_threshold) {
                flag[y][x] = 2;
            }
        }
    }

    // 用BFS连通弱边缘
    while (!queue.empty()) {
        auto [y, x] = queue.front();
        queue.pop();
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int ny = y + dy, nx = x + dx;
                if (ny < 0 || ny >= h || nx < 0 || nx >= w || flag[ny][nx] != 2) continue;
                flag[ny][nx] = 1;
                queue.push({ny, nx});
            }
    }

    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            out.data[y][x] = flag[y][x] == 1 ? 255 : 0;
    return out;
    }

}  // namespace icm
