# median_sobel_canny

工业相机测量课程作业1： C++ 实现 **中值滤波**、**Sobel 边缘检测**、**Canny 边缘检测** 三个算子。
本次课程作业在本人ubuntu22.04电脑上完成,使用C++17编译器编译构建，需要 CMake ≥ 3.16

## 构建与启动

```bash
# 构建
mkdir -p build && cd build && cmake .. && cmake --build . -j
# 启动gui程序
cd .. && build/bin/icm_gui
```

## 目录结构

```
median_sobel_canny/
├── CMakeLists.txt
├── include/
│   └── icm/                  # 自己实现的头文件(icm是课程简称) 
│       ├── canny.hpp
│       ├── convolution.hpp
│       ├── image_io.hpp
│       ├── kernels.hpp
│       ├── median_blur.hpp
│       └── sobel.hpp
├── src/                      # 只放 .cpp 实现
│   ├── main.cpp              # 命令行入口，分发三种处理方式并计时
│   ├── kernels.cpp           # Sobel / 高斯卷积核、默认参数、clamp255
│   ├── convolution.cpp       # 复制边界填充 padding + 卷积
│   ├── image_io.cpp          # stb_image / stb_image_write 封装
│   ├── median_blur.cpp       # 中值滤波（滑动直方图）
│   ├── sobel.cpp             # Sobel 边缘检测
│   └── canny.cpp             # Canny 边缘检测
├── third_party/              # 第三方库，这里用于图片读写
│   └── stb/
│       ├── stb_image.h
│       └── stb_image_write.h
└── docs/images/              # README 用的原图与结果对比图
```
