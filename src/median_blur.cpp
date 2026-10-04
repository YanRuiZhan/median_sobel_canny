//中值平滑滤波

#include "includes/median_blur.hpp"
#include "includes/convolution.hpp"

#include <algorithm>
#include <utility>
#include <vector>

// 插入排序法找中间值，排序后直接返回中位数
/*
时间复杂度 O(n^2)，n 为窗口内元素个数
空间复杂度 O(1)，原地排序，无需额外分配
窗口较小时（如 3x3、5x5）常数因子小、缓存友好
*/
int insertion_sort(std::vector<int>& arry) {
    for (std::size_t i = 1; i < arry.size(); ++i) {
        int key = arry[i];
        std::size_t j = i-1;
        while (j > 0 && arry[j] > key) {
            // j下标后移覆盖key的位置
            arry[j+1] = arry[j];
            --j;
        }
        // key插入到前面合适位置
        arry[j+1] = key;
    }
    return arry[arry.size() / 2];
}


// 对单通道进行中值滤波，模板窗口大小为win
Matrix median_blur_channel(const Matrix& src, int win) {
    if (src.empty() || src[0].empty()) return {};
    if (win <= 0 || win % 2 == 0) return src;   // 窗口必须是正奇数，否则原样返回

    const int h = static_cast<int>(src.size());
    const int w = static_cast<int>(src[0].size());

    // 复用卷积里的 padding：0 补边，保持输出尺寸与输入一致
    Matrix padded = padding(const_cast<Matrix&>(src), win);

    Matrix img(h, std::vector<int>(w, 0));
    std::vector<int> window;
    // 预订数组空间
    window.reserve(static_cast<std::size_t>(win) * win);

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            // 清除 window 移动到下个位置继续计算
            window.clear();
            for (int p = 0; p < win; ++p)
                for (int q = 0; q < win; ++q)
                    window.push_back(padded[i + p][j + q]);
            img[i][j] = insertion_sort(window);
        }
    }
    return img;
}

// 对多通道图像进行中值滤波
Image median_blur(const Image& src, int win) {
    Image img;
    img.w = src.w;
    img.h = src.h;
    img.ch = src.ch;
    img.data = Matrix(src.h, std::vector<int>(src.w * src.ch, 0));

    // 逐通道拆分 -> 滤波 -> 合回，避免跨通道串味
    // ch==1（灰度）时循环只跑一次
    Matrix channel(src.h, std::vector<int>(src.w, 0));
    for (int c = 0; c < src.ch; ++c) {
        // 拆出第 c 通道
        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                channel[y][x] = src.data[y][x * src.ch + c];

        Matrix filtered = median_blur_channel(channel, win);

        // 合回第 c 通道
        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                img.data[y][x * src.ch + c] = filtered[y][x];
    }
    return img;
}
