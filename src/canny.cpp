//canny算子边缘检测提取

//STEP：
//0. 彩色图灰度化
    /*BT.601 是国际电信联盟(ITU-R)制定的标清电视(SDTV)色度编码标准，
      其中定义了将 RGB 转换为亮度（灰度） Y 的加权系数：
      Y = 0.299*R + 0.587*G + 0.114*B
      系数按人眼对绿色最敏感、红色次之、蓝色最不敏感的特性加权，
      因此绿色权重最大(0.587)，蓝色权重最小(0.114)，
      且三者之和为 1，保证灰度值范围与原始亮度一致。*/
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

// 逐通道转灰度系数（BT.601）
Matrix to_gray(const Image& raw) {
    Matrix gray(raw.h, std::vector<int>(raw.w, 0));
    for (int y = 0; y < raw.h; ++y) {
        for (int x = 0; x < raw.w; ++x) {
            // 只有单通道或灰度加透明度双通道时直接赋值
            if (raw.ch == 1 || raw.ch == 2) {
                gray[y][x] = raw.data[y][x];
            } else {
                int r = raw.data[y][x * raw.ch + 0];
                int g = raw.data[y][x * raw.ch + 1];
                int b = raw.data[y][x * raw.ch + 2];
                gray[y][x] = static_cast<int>(0.299 * r+0.587 * g+0.114 * b );
            }
        }
    }
    return gray;
}

Image canny_edge(const Image& raw,const int low_threshold,const int high_threshold) {
    const int h = raw.h;
    const int w = raw.w;

    // 1. 灰度化 + 高斯平滑
    Matrix gray = to_gray(raw);

    Matrix blurred_raw = conv(gray, gauss_kernel);
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred_raw[y][x] / gauss_kernel_sum;

    // 2. 两个方向梯度 + 梯度方向角度
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
            double angle = std::atan2(dy, dx) * 180.0 / M_PI;
            if (angle < 0) angle += 180.0;
            dir[y][x] = angle;
        }
    }

    // 3. 梯度方向非极大值抑制
    std::vector<std::vector<double>> nms(h, std::vector<double>(w, 0.0));
    // y、x防止数组越界，nms最外圈直接为0
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double angle = dir[y][x];
            double m = mag[y][x];
            double m1, m2;
            // 水平方向梯度
            if ((angle >= 0 && angle < 22.5) ||
                (angle >= 157.5 && angle <= 180)) {
                m1 = mag[y][x - 1];
                m2 = mag[y][x + 1];
            // 主对角线方向梯度
            } else if (angle < 67.5) {
                m1 = mag[y - 1][x - 1];
                m2 = mag[y + 1][x + 1];
            // 竖直方向梯度
            } else if (angle < 112.5) {
                m1 = mag[y - 1][x];
                m2 = mag[y + 1][x];
            // 副对角线方向梯度
            } else {
                m1 = mag[y - 1][x + 1];
                m2 = mag[y + 1][x - 1];
            }
            // 只有梯度方向上的幅值m1、m2同时小于等于该点梯度幅值m时该位置为m，否则置0
            if (m >= m1 && m >= m2)
                nms[y][x] = m;
        }
    }

    // 4. 双阈值二值化

    // 0=背景, 1=强边缘, 2=弱边缘候选
    std::vector<std::vector<unsigned char>> flag(h, std::vector<unsigned char>(w, 0));
    std::queue<std::pair<int, int>> q;
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            double m = nms[y][x];
            if (m >= high_threshold) {
                // 大于high_threshold赋值为1
                flag[y][x] = 1;
                q.push({y, x});
            } else if (m >= low_threshold) {
                flag[y][x] = 2;
            }
        }
    }

    // 5. 弱边缘连接：从所有强边缘 BFS，把8邻域连通的弱边缘提升为强边缘
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
            // 0、1映射为0、255
            out.data[y][x] = (flag[y][x] == 1) ? 255 : 0;

    return out;
}
