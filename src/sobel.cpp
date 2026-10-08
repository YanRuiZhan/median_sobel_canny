// Sobel 锐化滤波：灰度化 -> 高斯平滑 -> 双方向梯度 -> 梯度幅值

#include "includes/sobel.hpp"
#include "includes/convolution.hpp"
#include "includes/kernels.hpp"

#include <cmath>
#include <vector>

// 对单通道图像计算 Sobel 梯度幅值
Matrix sobel_sharpen_channel(Matrix& gray) {
    const int h = static_cast<int>(gray.size());
    const int w = static_cast<int>(gray[0].size());

    // 高斯平滑滤波
    Matrix blurred_raw = conv(gray, gauss_kernel);
    Matrix blurred(h, std::vector<int>(w, 0));
    // 归一化
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / gauss_kernel_sum;

    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);

    Matrix edges(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double dx = gx[y][x];
            const double dy = gy[y][x];
            const double magnitude = std::sqrt(dx * dx + dy * dy);
            edges[y][x] = clamp255(static_cast<int>(magnitude));
        }
    return edges;
}

Image sobel_sharpen(const Image& raw) {
    if (raw.data.empty() || raw.w <= 0 || raw.h <= 0) return {};

    Matrix gray(raw.h, std::vector<int>(raw.w, 0));
    for (int y = 0; y < raw.h; ++y)
        for (int x = 0; x < raw.w; ++x) {
            const int offset = x * raw.ch;
            if (raw.ch == 1 || raw.ch == 2) {
                // 灰度或灰度 + alpha，只取灰度通道
                gray[y][x] = raw.data[y][offset];
            } else {
                // stb 图像为 RGB/RGBA 顺序，忽略 alpha
                const int r = raw.data[y][offset + 0];
                const int g = raw.data[y][offset + 1];
                const int b = raw.data[y][offset + 2];

                /*BT.601 是国际电信联盟(ITU-R)制定的标清电视(SDTV)色度编码标准，
                  其中定义了将 RGB 转换为亮度（灰度） Y 的加权系数：
                  Y = 0.299*R + 0.587*G + 0.114*B
                  系数按人眼对绿色最敏感、红色次之、蓝色最不敏感的特性加权，
                  因此绿色权重最大(0.587)，蓝色权重最小(0.114)，
                  且三者之和为 1，保证灰度值范围与原始亮度一致。*/

                gray[y][x] = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
            }
        }

    Image out;
    out.w = raw.w;
    out.h = raw.h;
    out.ch = 1;
    out.data = sobel_sharpen_channel(gray);
    return out;
}
