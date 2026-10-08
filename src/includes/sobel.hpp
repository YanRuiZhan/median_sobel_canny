#ifndef SOBEL_HPP
#define SOBEL_HPP

#include "image_io.hpp"

// Sobel 算子锐化图像
// 步骤：灰度化 -> 高斯平滑 -> Sobel 求 Gx/Gy -> 梯度幅值 -> 与原灰度叠加
// 灰度(ch==1)与彩色(ch==3)统一走逐通道流程
Image sobel_sharpen(const Image& src,const double k);

#endif
