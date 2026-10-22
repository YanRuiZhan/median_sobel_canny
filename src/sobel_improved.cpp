// 改进版 Sobel：四方向梯度（水平、垂直、45°、135°）

#include "icm/sobel.hpp"
#include "icm/convolution.hpp"
#include "icm/kernels.hpp"

#include <cmath>
#include <vector>

namespace icm {

Image sobel_sharpen_improved(const Image& raw) {
    if (raw.data.empty() || raw.w <= 0 || raw.h <= 0) return {};

    Matrix gray(raw.h, std::vector<int>(raw.w, 0));
    for (int y = 0; y < raw.h; ++y)
        for (int x = 0; x < raw.w; ++x) {
            const int offset = x * raw.ch;
            if (raw.ch == 1 || raw.ch == 2) {
                gray[y][x] = raw.data[y][offset];
            } else {
                const int r = raw.data[y][offset + 0];
                const int g = raw.data[y][offset + 1];
                const int b = raw.data[y][offset + 2];
                gray[y][x] = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
            }
        }

    const int h = raw.h;
    const int w = raw.w;
    Matrix blurred_raw = conv(gray, gauss_kernel);
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / gauss_kernel_sum;

    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);
    Matrix g45 = conv(blurred, sob45);
    Matrix g135 = conv(blurred, sob135);

    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double magnitude = std::sqrt(
                double(gx[y][x]) * gx[y][x] + double(gy[y][x]) * gy[y][x] +
                double(g45[y][x]) * g45[y][x] + double(g135[y][x]) * g135[y][x]);
            out.data[y][x] = clamp255(static_cast<int>(magnitude));
        }
    return out;
}

}  // namespace icm
