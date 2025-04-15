// main.c
#include <stdio.h>

// 声明在 a.c 和 b.c 中定义的函数
void functionA();
void functionB();

int main() {
    // 在 main 函数中调用 a.c 中的函数
    functionA();
    
    // 在 main 函数中调用 b.c 中的函数
    functionB();

    return 0;
}
