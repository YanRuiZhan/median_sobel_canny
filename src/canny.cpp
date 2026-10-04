//canny算子边缘检测提取（基于项目自有的 Image / Matrix，不依赖 OpenCV）
//
//步骤：
//1. 高斯平滑滤波
//2. 计算两个方向上梯度大小（复用 sobelx、sobely 算子），同时求梯度方向
//3. 梯度方向上非极大值抑制（NMS）
//4. 双阈值处理二值化（高阈值=强边缘，低阈值=弱边缘候选）
//5. 弱边缘连接：仅当弱边缘与强边缘相连时才保留（BFS 扩散）

#include "includes/canny.hpp"
#include "includes/convolution.hpp"
#include "includes/kernels.hpp"

#include <cmath>
#include <vector>
#include <queue>

namespace {

// 高斯平滑系数，与 sobel.cpp 保持一致
constexpr double kGaussSum = 16.0;

// 逐通道转灰度系数（BT.601）
Matrix to_gray(const Image& src) {
    Matrix gray(src.h, std::vector<int>(src.w, 0));
    for (int y = 0; y < src.h; ++y) {
        for (int x = 0; x < src.w; ++x) {
            if (src.ch == 1) {
                gray[y][x] = src.data[y][x];
            } else {
                int b = src.data[y][x * src.ch + 0];
                int g = src.data[y][x * src.ch + 1];
                int r = src.data[y][x * src.ch + 2];
                gray[y][x] = static_cast<int>(0.114 * b + 0.587 * g + 0.299 * r);
            }
        }
    }
    return gray;
}

Matrix gauss_kernel() {
    return {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
}

}  // namespace

Image canny_edge(const Image& src) {
    const int h = src.h;
    const int w = src.w;

    // 1. 灰度化 + 高斯平滑
    Matrix gray = to_gray(src);

    Matrix gk = gauss_kernel();
    Matrix blurred_raw = conv(gray, gk);
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / static_cast<int>(kGaussSum);

    // 2. 两个方向梯度 + 梯度方向（弧度）
    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);

    std::vector<std::vector<double>> mag(h, std::vector<double>(w, 0.0));
    std::vector<std::vector<double>> dir(h, std::vector<double>(w, 0.0));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double dx = gx[y][x];
            double dy = gy[y][x];
            mag[y][x] = std::sqrt(dx * dx + dy * dy);
            // 方向归一到 [0, 180) 度
            double ang = std::atan2(dy, dx) * 180.0 / M_PI;
            if (ang < 0) ang += 180.0;
            dir[y][x] = ang;
        }
    }

    // 3. 梯度方向非极大值抑制
    std::vector<std::vector<double>> nms(h, std::vector<double>(w, 0.0));
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double a = dir[y][x];
            double m = mag[y][x];
            double m1, m2;
            if ((a >= 0 && a < 22.5) || (a >= 157.5 && a <= 180)) {
                m1 = mag[y][x - 1];
                m2 = mag[y][x + 1];
            } else if (a < 67.5) {
                m1 = mag[y - 1][x + 1];
                m2 = mag[y + 1][x - 1];
            } else if (a < 112.5) {
                m1 = mag[y - 1][x];
                m2 = mag[y + 1][x];
            } else {
                m1 = mag[y - 1][x - 1];
                m2 = mag[y + 1][x + 1];
            }
            if (m >= m1 && m >= m2)
                nms[y][x] = m;
        }
    }

    // 4. 双阈值二值化
    const double weak = 50.0;
    const double strong = 150.0;

    // 0=背景, 1=强边缘, 2=弱边缘候选
    std::vector<std::vector<unsigned char>> flag(h, std::vector<unsigned char>(w, 0));
    std::queue<std::pair<int, int>> q;
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double m = nms[y][x];
            if (m >= strong) {
                flag[y][x] = 1;
                q.push({y, x});
            } else if (m >= weak) {
                flag[y][x] = 2;
            }
        }
    }

    // 5. 弱边缘连接：从所有强边缘 BFS，把连通的弱边缘提升为强边缘
    const int dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    while (!q.empty()) {
        auto [cy, cx] = q.front();
        q.pop();
        for (int k = 0; k < 8; ++k) {
            int ny = cy + dy8[k];
            int nx = cx + dx8[k];
            if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
            if (flag[ny][nx] == 2) {
                flag[ny][nx] = 1;   // 弱边缘被强边缘连接，保留
                q.push({ny, nx});
            }
        }
    }

    // 输出单通道二值边缘图：仅保留强边缘(255)
    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            out.data[y][x] = (flag[y][x] == 1) ? 255 : 0;

    return out;
}
