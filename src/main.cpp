#include "includes/image_io.hpp"
#include "includes/median_blur.hpp"
#include "includes/sobel.hpp"
#include "includes/canny.hpp"
#include "includes/kernels.hpp"
#include <iostream>
#include <string>
#include <chrono>

// 计时辅助：记录起始时刻，单位毫秒
using Clock = std::chrono::steady_clock;
static Clock::time_point tick() { return Clock::now(); }
static double tock_ms(const Clock::time_point& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main(int argc, char** argv) {
    if (argc < 4) {
        /*
         argv[0]:二进制文件路径
         argv[1]:输入图像路径
         argv[2]:输出图像路径
         argv[3]:图像处理方法(median/sobel/canny)
         argv[4]:处理方法参数
         */
        std::cerr << "用法: " << argv[0] << " <输入图> <输出图> <处理方式> [窗口大小n]\n";
        std::cerr << "  处理方式: median   - 中值滤波\n";
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

    //1.如果传入中值滤波参数
    if (method == "median") {
        // 窗口大小：默认值在 kernels.cpp 中设置，命令行可覆盖
        int win = median_default_window;
        if (argc >= 5) {
            win = std::atoi(argv[4]);
            if (win <= 0 || win % 2 == 0) {
                std::cerr << "窗口大小必须是正奇数\n";
                return 1;
            }
        }

        auto t0 = tick();
        Image out = median_blur(img, win);
        std::cout << "中值滤波完成，窗口 " << win << "x" << win
                  << "  用时 " << tock_ms(t0) << " ms\n";

        if (!save_image(argv[2], out)) {
            std::cerr << "保存失败,请创建目标文件夹\n";
            return 1;
        }
        std::cout << "已保存: " << argv[2] << "\n";
        return 0;
    }

    //2.如果传入sobel算子参数
    else if (method == "sobel") {
        auto t0 = tick();
        Image out = sobel_sharpen(img);
        std::cout << "sobel图像锐化完成  用时 " << tock_ms(t0) << " ms\n";

        if (!save_image(argv[2], out)) {
            std::cerr << "保存失败\n";
            return 1;
        }
        std::cout << "已保存: " << argv[2] << "\n";
        return 0;
    }

    //3.如果传入canny算子参数
    else if (method == "canny") {
        auto t0 = tick();
        Image out = canny_edge(img);
        std::cout << "canny算子提取图像边缘完成  用时 " << tock_ms(t0) << " ms\n";

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
