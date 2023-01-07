#ifndef INTERCODE_H
#define INTERCODE_H

#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"

typedef struct Operand_d Operand_;
typedef Operand_* Operand;
typedef struct InterCode_d InterCode_;
typedef InterCode_* InterCode;

// 操作数（变量，临时变量，标记，函数）
struct Operand_d {
    enum {
        VARIABLE_OP, TEMP_VAR_OP, CONSTANT_OP, LABEL_OP, 
        FUNCTION_OP, GET_ADDR_OP, GET_VAL_OP
    } kind;
    union {
        int no; // 临时变量的编号，标记的编号
        int value;  // 常量的值
        char name[32];  // 函数以及变量的名字
        Operand opr; // 取地址和解引用指向的操作数
    };
    Type type;  // 存放数组/结构体类型变量的类型
    Operand next;   // 给argList链接操作数用
};

// 单条指令
struct InterCode_d {
    enum {
        LABEL_IR, FUNC_IR, ASSIGN_IR, PLUS_IR, SUB_IR, MUL_IR, 
        DIV_IR, TO_MEM_IR, GOTO_IR,
        IF_GOTO_IR, RETURN_IR, DEC_IR, ARG_IR, CALL_IR, PARAM_IR,
        READ_IR, WRITE_IR, NULL_IR
    } kind;
    // 操作数指针数组
    Operand ops[3];
    // 额外信息
    union {
        char relop[32];
        int size;
    };
    // 前一条和后一条指令
    InterCode pre;
    InterCode next;
};

void initInterCodes();
void insertInterCode(InterCode code, InterCode interCodes);
void printInterCodes(char* name);
void printOperand(Operand op, FILE* fp);

InterCode translateExp(Node* root, Operand place);
InterCode translateArgs(Node* root, Operand argList);
InterCode translateStmt(Node* root);
InterCode translateCond(Node* root, Operand labelTrue, Operand labelFalse);
void translateProgram(Node* root);
InterCode translateExtDefList(Node* root);
InterCode translateExtDef(Node* root);
InterCode translateCompSt(Node* root, char* funcName);
InterCode translateStmtList(Node* root);
InterCode translateDefList(Node* root, IdType class);
InterCode translateDef(Node* root, IdType class);
FieldList translateDecList(Node* root, Type type, IdType class, InterCode code);
FieldList translateDec(Node* root, Type type, IdType class, InterCode code);

#endif