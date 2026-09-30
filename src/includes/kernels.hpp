#ifndef kernels_HPP
#define kernels_HPP

#include <vector>

//卷积核类型别名，等价于 std::vector<std::vector<int>>
using Matrix = std::vector<std::vector<int>>;

extern int n;
extern Matrix blur_Matrix;
extern Matrix sobx;
extern Matrix soby;

#endif
