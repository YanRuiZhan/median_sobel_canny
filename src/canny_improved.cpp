#include "icm/canny.hpp"
#include "icm/convolution.hpp"
#include "icm/kernels.hpp"


#include <cmath>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace icm {

    // 与原 Canny 共用 BT.601 灰度转换。
    Matrix to_gray(const Image& raw);


    Image canny_edge_improved(const Image& src, int low_threshold, int high_threshold) {
        const int h = src.h, w = src.w;
        Matrix gray = to_gray(src);
        Matrix blurred_raw = conv(gray, gauss_kernel);
        Matrix blurred(h, std::vector<int>(w, 0));
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                blurred[y][x] = blurred_raw[y][x] / gauss_kernel_sum;


    Matrix gx = conv(blurred, sobx);
    Matrix gy = conv(blurred, soby);
    Matrix g45 = conv(blurred, sob45);
    Matrix g135 = conv(blurred, sob135);

    // 梯度幅值，四个梯度方向的二范数
    std::vector<std::vector<double>> magnitude(h, std::vector<double>(w, 0.0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            double dx = gx[y][x], dy = gy[y][x];
            double d45 = g45[y][x], d135 = g135[y][x];
            magnitude[y][x] = std::sqrt(dx * dx + dy * dy + d45 * d45 + d135 * d135);
        }

    // 梯度方向（度，范围 [0, 180]）
    std::vector<std::vector<double>> dir(h, std::vector<double>(w, 0.0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            // 3.14159 即 π，用于弧度转角度
            double angle = std::atan2(double(gx[y][x]), double(gy[y][x])) * 180.0 / 3.14159;
            if (angle < 0.0) angle += 180.0;
            dir[y][x] = angle;
        }

    // 3. 梯度方向非极大值抑制
    std::vector<std::vector<double>> nms(h, std::vector<double>(w, 0.0));
    // y、x防止数组越界，nms最外圈直接为0
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double angle = dir[y][x];
            double m = magnitude[y][x];
            double m1, m2;
            // 水平方向梯度
            if ((angle >= 0 && angle < 22.5) ||
                (angle >= 157.5 && angle <= 180)) {
                m1 = magnitude[y][x - 1];
                m2 = magnitude[y][x + 1];
            // 主对角线方向梯度
            } else if (angle < 67.5) {
                m1 = magnitude[y - 1][x - 1];
                m2 = magnitude[y + 1][x + 1];
            // 竖直方向梯度
            } else if (angle < 112.5) {
                m1 = magnitude[y - 1][x];
                m2 = magnitude[y + 1][x];
            // 副对角线方向梯度
            } else {
                m1 = magnitude[y - 1][x + 1];
                m2 = magnitude[y + 1][x - 1];
            }
            // 只有梯度方向上的幅值m1、m2同时小于等于该点梯度幅值m时该位置为m，否则置0
            if (m >= m1 && m >= m2)
                nms[y][x] = m;
        }
    }

    std::vector<std::vector<unsigned char>> flag(h, std::vector<unsigned char>(w, 0));
    std::queue<std::pair<int, int>> queue;

    // 双阈值检测
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double m = nms[y][x];
            // 即使阈值为零，也不能将被抑制点或平坦区域判成边缘。
            if (m <= 0.0) continue;
            if (m >= high_threshold) {
                flag[y][x] = 1;
                queue.push({y, x});
            } else if (m >= low_threshold) {
                flag[y][x] = 2;
            }
        }
    }

    // 用BFS连通弱边缘
    while (!queue.empty()) {
        auto [y, x] = queue.front();
        queue.pop();
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int ny = y + dy, nx = x + dx;
                if (ny < 0 || ny >= h || nx < 0 || nx >= w || flag[ny][nx] != 2) continue;
                flag[ny][nx] = 1;
                queue.push({ny, nx});
            }
    }

    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            out.data[y][x] = flag[y][x] == 1 ? 255 : 0;
    return out;
    }

}  // namespace icm
