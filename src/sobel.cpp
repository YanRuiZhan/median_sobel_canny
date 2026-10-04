//Sobel算子锐化图像（基于项目自有的 Image / Matrix，不依赖 OpenCV）
//
//步骤：
//1. 对每个通道分别处理（灰度图只有 1 个通道）
//2. 对通道做高斯滤波（平滑降噪，减少噪声对边缘检测的干扰）
//3. 分别用水平方向和垂直方向的Sobel卷积核与图像做卷积，得到水平梯度Gx和垂直梯度Gy
//4. 计算每个像素的梯度幅值：G = sqrt(Gx*Gx + Gy*Gy)
//5. 将梯度幅值截断到 [0, 255] 范围，得到边缘强度图
//6. 将边缘强度图与该通道原图叠加（channel + edges），得到锐化后的通道

#include "includes/sobel.hpp"
#include "includes/convolution.hpp"
#include "includes/kernels.hpp"

#include <cmath>
#include <vector>

namespace {

// 3x3 高斯平滑卷积核（和为 16），降低噪声引起的伪边缘
Matrix gauss_kernel() {
    return {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
}

int clamp255(int v) {
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

// 对单通道图像做 Sobel 锐化，返回同尺寸单通道结果
Matrix sobel_sharpen_channel(const Matrix& ch) {
    const int h = static_cast<int>(ch.size());
    const int w = static_cast<int>(ch[0].size());

    // 1. 高斯平滑。conv() 是整数卷积，输出未归一化，这里手动除以核和(16)
    Matrix gk = gauss_kernel();
    Matrix blurred_raw = conv(const_cast<Matrix&>(ch), gk);
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / 16;

    // 2. Sobel 求水平、垂直方向梯度（复用全局 sobx / soby 卷积核）
    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);

    // 3/4. 梯度幅值 G = sqrt(Gx^2 + Gy^2)，并截断到 [0,255]
    Matrix edges(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            edges[y][x] = clamp255(static_cast<int>(
                std::sqrt(static_cast<double>(gx[y][x]) * gx[y][x] +
                          static_cast<double>(gy[y][x]) * gy[y][x])));

    // 5. 边缘强度图与该通道原图叠加，得到锐化结果
    Matrix sharp(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            sharp[y][x] = clamp255(ch[y][x] + edges[y][x]);

    return sharp;
}

}  // namespace

Image sobel_sharpen(const Image& src) {
    Image out;
    out.w = src.w;
    out.h = src.h;
    out.ch = src.ch;
    out.data = Matrix(out.h, std::vector<int>(out.w * out.ch, 0));

    // 逐通道拆分 -> Sobel 锐化 -> 合回，彩色图各通道颜色/边缘独立保留
    // ch==1（灰度）时循环只跑一次
    Matrix channel(src.h, std::vector<int>(src.w, 0));
    for (int c = 0; c < src.ch; ++c) {
        // 拆出第 c 通道
        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                channel[y][x] = src.data[y][x * src.ch + c];

        Matrix sharp = sobel_sharpen_channel(channel);

        // 合回第 c 通道
        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                out.data[y][x * src.ch + c] = sharp[y][x];
    }

    return out;
}
