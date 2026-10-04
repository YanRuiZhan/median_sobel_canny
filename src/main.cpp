#include "includes/image_io.hpp"
#include "includes/median_blur.hpp"
#include "includes/sobel.hpp"
#include "includes/canny.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "用法: " << argv[0] << " <输入图> <输出图> <处理方式> [窗口大小n]\n";
        std::cerr << "  处理方式: median  - 中值滤波\n";
        std::cerr << "            sobel   - sobel图像锐化\n";
        std::cerr << "            canny   - canny算子提取图像边缘\n";
        return 1;
    }

    Image img = read_image(argv[1]);
    if (img.data.empty()) {
        std::cerr << "读取失败\n";
        return 1;
    }
    std::cout << "读入: " << img.w << "x" << img.h
              << "  通道数: " << img.ch << "\n";

    std::string method = argv[3];

    if (method == "median") {
        // 窗口大小：命令行第 4 个参数，默认 3
        int win = 3;
        if (argc >= 5) {
            win = std::atoi(argv[4]);
            if (win <= 0 || win % 2 == 0) {
                std::cerr << "窗口大小必须是正奇数\n";
                return 1;
            }
        }

        Image out = median_blur(img, win);
        std::cout << "中值滤波完成，窗口 " << win << "x" << win << "\n";

        if (!save_image(argv[2], out)) {
            std::cerr << "保存失败,请创建目标文件夹\n";
            return 1;
        }
        std::cout << "已保存: " << argv[2] << "\n";
        return 0;
    }
    else if (method == "sobel") {
        Image out = sobel_sharpen(img);
        std::cout << "sobel图像锐化完成\n";

        if (!save_image(argv[2], out)) {
            std::cerr << "保存失败\n";
            return 1;
        }
        std::cout << "已保存: " << argv[2] << "\n";
        return 0;
    }
    else if (method == "canny") {
        Image out = canny_edge(img);
        std::cout << "canny算子提取图像边缘完成\n";

        if (!save_image(argv[2], out)) {
            std::cerr << "保存失败\n";
            return 1;
        }
        std::cout << "已保存: " << argv[2] << "\n";
        return 0;
    }
    else {
        std::cerr << "未知的处理方式: " << method << "\n";
        std::cerr << "可选: median / sobel / canny\n";
        return 1;
    }
}
