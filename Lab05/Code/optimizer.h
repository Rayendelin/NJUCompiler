#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 描述程序的数据结构
typedef struct Operand_d Operand_;
typedef Operand_* Operand;
typedef struct InterCode_d InterCode_;
typedef InterCode_* InterCode;
typedef struct BasicBlock_d BasicBlock_;
typedef BasicBlock_* BasicBlock;

// 操作数（变量，临时变量，标记，函数）
struct Operand_d {
    enum {
        VARIABLE_OP, TEMP_VAR_OP, CONSTANT_OP, LABEL_OP,
        FUNCTION_OP, GET_ADDR_OP, GET_VAL_OP
    } kind;
    union {
        int no; // 临时变量的编号，变量的编号，标记的编号
        int value;  // 常量的值
        char name[32];  // 函数的名字
        Operand opr; // 取地址和解引用指向的操作数
    };
    int ver;    // 版本号，用于基本块内部优化
};

// 单条指令
struct InterCode_d {
    enum {
        LABEL_IR, FUNC_IR, ASSIGN_IR, PLUS_IR, SUB_IR, MUL_IR, 
        DIV_IR, TO_MEM_IR, GOTO_IR,
        IF_GOTO_IR, RETURN_IR, DEC_IR, ARG_IR, CALL_IR, PARAM_IR,
        READ_IR, WRITE_IR, NULL_IR
    } kind;
    Operand ops[3]; // 操作数指针数组
    union {
        char relop[32];
        int size;
    };  // 额外信息：IF_GOTO_IR中的比较符号，DEC_IR中的数值大小
    InterCode pre;  // 前一条指令
    InterCode next; // 后一条指令
};

// 基本块
struct BasicBlock_d {
    int no; // 基本块编号
    int dead;   // 不可达为1，可达为0
    InterCode interCodes;   // 基本块内部指令
    BasicBlock next[2];  // 后驱基本块，基本块后驱应该至多只有两个
    int preNum; // 前驱数量
    BasicBlock* pre;    // 前驱基本块，数量不确定
    char func[32];  // 所属函数的名字
};

// 描述基本块内部语句结构的数据结构
typedef struct DAGNode_d DAGNode_;
typedef DAGNode_* DAGNode;

// DAG图的节点
struct DAGNode_d {
    enum {
        ASSIGN_NODE, PLUS_NODE, SUB_NODE, MUL_NODE, DIV_NODE, LEAF_NODE
    } kind; // 节点的类型
    Operand op; // 该节点代表的语句修改的操作数
    DAGNode children[2];   // LEAF没有子节点，ASSIGN有一个子节点，剩余四种有两个子节点
};

// 描述基本块状态的数据结构
typedef struct BlockState_d BlockState_;
typedef BlockState_* BlockState;

struct BlockState_d {
    int size;
    int* in;
    int* out;
    int* inValue; // 常量传播中对应的常量
    int* outValue;
};

void readInput(char* filename);
void optimize();
void printOperand(Operand op, FILE* fp);
void writeOutput(char* filename);

#endif