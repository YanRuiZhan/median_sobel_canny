#ifndef CANNY_HPP
#define CANNY_HPP

#include "image_io.hpp"

namespace icm {

// Canny 算子边缘检测
// 步骤：高斯平滑 -> Sobel 求梯度 -> 梯度方向非极大值抑制(NMS)
//       -> 双阈值二值化 -> 弱边缘连接(BFS)
// 返回单通道二值边缘图(边缘=255，背景=0)
Image canny_edge(const Image& src,const int low_threshold,const int high_threshold);

// 3x3 高斯平滑 + 四方向 Sobel；返回单通道二值边缘图。
Image canny_edge_improved(const Image& src, int low_threshold, int high_threshold);

}  // namespace icm

#endif
