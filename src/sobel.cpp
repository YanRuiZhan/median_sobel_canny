//Sobel算子锐化图像

//STEP：
//1. 对每个通道分别处理（灰度图只有 1 个通道）
//2. 对通道做高斯滤波（平滑降噪，减少噪声对边缘检测的干扰）
//3. 分别用水平方向和垂直方向的Sobel卷积核与图像做卷积，得到水平梯度Gx和垂直梯度Gy
//4. 计算每个像素的梯度幅值：G = sqrt(Gx*Gx + Gy*Gy)
//5. 将浮点梯度幅值按 sobel_amount 缩放后叠加到原图
//6. 将最终结果截断到 [0, 255]，得到锐化后的通道

#include "includes/sobel.hpp"
#include "includes/convolution.hpp"
#include "includes/kernels.hpp"

#include <cmath>
#include <vector>

// 对单通道图像做 Sobel 锐化，返回同尺寸单通道结果
Matrix sobel_sharpen_channel(const Matrix& ch) {
    // 将std::size_t（无符号整形）转为 int
    const int h = static_cast<int>(ch.size());
    const int w = static_cast<int>(ch[0].size());

    // 1. 高斯平滑滤波
    Matrix blurred_raw = conv(const_cast<Matrix&>(ch), gauss_kernel);

    // 归一化得到blurred
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / gauss_kernel_sum;

    // 2. Sobel 求水平、垂直方向梯度（复用全局 sobx / soby 卷积核）
    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);

    // 3. 保留浮点梯度幅值，缩放叠加后才截断，避免提前丢失强边缘信息
    Matrix sharp(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double dx = gx[y][x];
            const double dy = gy[y][x];
            // 计算梯度幅值
            const double magnitude = std::sqrt(dx * dx + dy * dy);
            sharp[y][x] = clamp255(static_cast<int>(
                ch[y][x] + sobel_amount * magnitude));
        }
    return sharp;
}

Image sobel_sharpen(const Image& raw) {
    Image out;
    out.w = raw.w;
    out.h = raw.h;
    out.ch = raw.ch;
    out.data = Matrix(out.h, std::vector<int>(out.w * out.ch, 0));

    // 逐通道拆分 -> Sobel 锐化 -> 合回，彩色图各通道颜色/边缘独立保留
    // ch==1（灰度）时循环只跑一次
    Matrix channel(raw.h, std::vector<int>(raw.w, 0));

    for (int c = 0; c < raw.ch; ++c) {
        // 拆出第 c 通道
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                channel[y][x] = raw.data[y][x * raw.ch + c];
        Matrix sharp = sobel_sharpen_channel(channel);

        // 合回第 c 通道
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                out.data[y][x * raw.ch + c] = sharp[y][x];
    }
    return out;
}
