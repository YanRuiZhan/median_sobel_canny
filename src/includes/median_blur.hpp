#ifndef MEDIAN_BLUR_HPP
#define MEDIAN_BLUR_HPP

#include "image_io.hpp"
#include "kernels.hpp"

// 对 Image 做 n*n 窗口的中值滤波
// 灰度(ch==1)与彩色(ch==3)统一走逐通道流程，无需分别处理
Image median_blur(const Image& src, int win);

#endif
