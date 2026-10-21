#ifndef CANNY_HPP
#define CANNY_HPP

#include "image_io.hpp"

namespace icm {

// Canny 算子边缘检测
// 步骤：高斯平滑 -> Sobel 求梯度 -> 梯度方向非极大值抑制(NMS)
//       -> 双阈值二值化 -> 弱边缘连接(BFS)
// 返回单通道二值边缘图(边缘=255，背景=0)
Image canny_edge(const Image& src,const int low_threshold,const int high_threshold);

// 双边滤波 + 四方向 Sobel；梯度幅值为四方向平方和开根号，直接与阈值比较。
// diameter 为正奇数；sigma_space 为像素距离尺度，sigma_color 为灰度差尺度。
// 参数无效时抛出 std::invalid_argument。返回单通道二值边缘图。
Image canny_edge_improved(const Image& src, int low_threshold, int high_threshold,
                          int diameter = 3, double sigma_space = 1.5,
                          double sigma_color = 45.0);

}  // namespace icm

#endif
