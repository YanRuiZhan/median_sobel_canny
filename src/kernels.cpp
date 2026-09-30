//滤波核配置

/*
 一、中值滤波（一种统计排序滤波器）：
    将n*n范围内元素做从大到小排序后，中间的数作为卷积运算的结果。

 二、sobel算子：
    sobx:[[-1,-2,-1]      soby:[[-1, 0, 1]
         ,[ 0, 0, 0]           ,[-2, 0, 2]
         ,[ 1, 2, 1]]          ,[-1, 0, 1]]

 三、canny算子:
    1.高斯模糊去噪（canny算子对噪声敏感）
    2.计算两个方向梯度canx，cany
    3.梯度方向上非极大值抑制（NMS）
    4.双阈值（low & high）
    5.将low-high阈值中间边线与high部分边线连接（BFS扩散）
 */

#include "includes/kernels.hpp"

//初始化n*n的平滑滤波卷积核
int n = 3;
Matrix blur_kernel(n, std::vector<int>(n, 0));

//初始化sobel算子x、y方向卷积核,sobel算子固定为3*3模板
Matrix sobx = {{-1,-2,-1},{ 0, 0, 0},{ 1, 2, 1}};
Matrix soby = {{-1, 0, 1},{-2, 0, 2},{-1, 0, 1}};
