#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include "Tree.h"

// 两个基本类型
#define INT_TYPE 0
#define FLOAT_TYPE 1

// 哈希表大小
#define HASH_SIZE 31

typedef struct Type_d Type_;
typedef Type_* Type;
typedef struct FieldList_d FieldList_;
typedef FieldList_* FieldList;
typedef struct Structure_d Structure_;
typedef Structure_* Structure;
typedef struct Function_d Function_;
typedef Function_* Function;
typedef struct Entry_d Entry_;
typedef Entry_* Entry;

typedef enum {
    ENUM_BASIC,
    ENUM_ARRAY,
    ENUM_STRUCT,
    ENUM_STRUCT_DEF,
    ENUM_FUNC
} Kind;

typedef enum {
    ENUM_VAR,
    ENUM_FIELD
} IdType;

struct Type_d {
    Kind kind;
    union {
        // 基本类型
        int basic;
        // 数组类型包括元素类型与数组大小构成
        struct {
            Type elem;
            int size;
        } array;
        // 结构体类型是一个链表
        Structure structure;
        // 函数类型
        Function func;
    };
};

// 结构体域链表节点
struct FieldList_d {
    // 域的名字
    char name[32];
    // 域的类型
    Type type;
    // 指向下一个域的指针
    FieldList next;
};

// 结构体类型
struct Structure_d {
    char name[32];
    FieldList head;
};

// 函数类型
struct Function_d {
    char name[32];
    // 返回值类型
    Type returnType;
    // 参数个数
    int parmNum;
    // 参数链表头节点指针
    FieldList head;
    // 是否已经定义
    int hasDefined;
    // 所在行数
    int lineno;
};

// 符号表条目类型
struct Entry_d {
    char name[32];
    Type type;
    // 指向同一槽位的下一个条目
    Entry hashNext;
    // 指向同一层次的下一个条目
    Entry layerNext;
    // 在Lab3中区分该条目是否为函数的参数，默认为0（不是）
    int isArg;
};

void insertSymbol(Entry symbol);
Entry findSymbolAll(char* name);
Entry findSymbolFunc(char* name);
void pushLayer();
void popLayer();
void initSymbolTable();

void semanticAnalyse(Node* root);
void Program(Node* root);
void check();
void ExtDefList(Node* root);
void ExtDef(Node* root);
Type Specifier(Node* root);
void ExtDecList(Node* root, Type type);
Function FunDec(Node* root);
void CompSt(Node* root, char* funcName, Type reType);
Type StructSpecifier(Node* root);
FieldList DefList(Node* root, IdType class);
FieldList Def(Node* root, IdType class);
FieldList DecList(Node* root, Type type, IdType class);
FieldList Dec(Node* root, Type type, IdType class);
FieldList VarDec(Node* root, Type type, IdType class);
FieldList VarList(Node* root);
FieldList ParamDec(Node* root);
void StmtList(Node* root, Type retype);
void Stmt(Node* root, Type reType);
Type Exp(Node* root);
FieldList Args(Node* root);
void printArgs(FieldList head);
void printType(Type type);

#endif