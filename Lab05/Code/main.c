#include "optimizer.h"

int main(int argc, char** argv) {
    // ./cc input.ir output.ir
    if (argc < 3)
        return 1;
    // 读取输入
    readInput(argv[1]);
    // 优化
    optimize();
    // 打印输出
    writeOutput(argv[2]);
    return 0;
}