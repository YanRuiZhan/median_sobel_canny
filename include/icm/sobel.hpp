#ifndef SOBEL_HPP
#define SOBEL_HPP

#include "image_io.hpp"

namespace icm {

// Sobel 边缘检测：先灰度化，再高斯平滑、求双方向梯度幅值
// 输出单通道灰度图，不叠加原图，忽略 alpha
Image sobel_sharpen(const Image& src);

// 改进版 Sobel：在水平、垂直、45°、135°四个方向计算梯度幅值。
Image sobel_sharpen_improved(const Image& src);

}  // namespace icm

#endif
