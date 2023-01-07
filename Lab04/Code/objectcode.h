#ifndef OBJECTCODE_H
#define OBJECTCODE_H

#include "intercode.h"

typedef struct RegDes_d RegDes_;
typedef RegDes_* RegDes;
typedef struct VarDes_d VarDes_;
typedef VarDes_* VarDes;
typedef struct FrameDes_d FrameDes_;
typedef FrameDes_* FrameDes;

// 寄存器描述符
struct RegDes_d {
    int free;   // 标记是否可用，用来协调寄存器分配的互不抢占
    int interval;   // 距上次访问的间隔，用于寄存器选择
    char name[6];   // 寄存器别名
    VarDes var; // 存储在该寄存器中的变量的描述符
};

// 变量描述符
struct VarDes_d {
    // int regNo;  // 存储该变量的寄存器的编号，-1表示无
    int offset; // 该变量相当于当前栈帧底部的偏移量
    Operand op; // 描述的操作数信息
    VarDes next;    // 链接下一个变量描述符
};

// 栈帧描述符
struct FrameDes_d {
    char name[32];  // 该栈帧对应函数的名称
    VarDes vars; // 该栈帧中预定存放对应函数的所有变量/临时变量，通过翻译前对中间代码的扫描预先安排好次序，方便对变量地址的定位
    FrameDes next;  // 链接下一个栈帧描述符
};

void printObjectCodes(char* name);

#endif