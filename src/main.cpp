# include "includes/convolution.hpp"
# include <cstddef>
# include <iostream>

int main(){
    Matrix matrix = {{1,2,3},{1,2,2},{2,2,1}};
    Matrix filter = {{1,4,3},{1,4,2},{2,5,7}};
    auto res = conv(matrix, filter);
    for(std::size_t i = 0;i<res.size();++i){
        for(std::size_t j = 0;j<res[0].size();++j){
            std::cout<<res[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
    return 0;
}
