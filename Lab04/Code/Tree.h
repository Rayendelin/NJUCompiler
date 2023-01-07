#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <string.h>

// 树节点类型枚举，决定使用什么打印格式
typedef enum {
    ENUM_SYN_NOT_NULL = 1,
    ENUM_SYN_NULL,
    ENUM_LEX_ID,
    ENUM_LEX_TYPE,
    ENUM_LEX_INT,
    ENUM_LEX_FLOAT,
    ENUM_LEX_OTHER
} NodeType;

// 树节点定义
typedef struct Node_{
    char* name; // 节点名称
    NodeType nodeType;  // 节点类型
    int lineno; // 该节点对应语法/词法单元的行号
    union { // 该节点需要存储的值信息
        char strVal[32];    // 定义成char*会报段错误，因为初始化Node时不会给指针分配指向的空间
        int intVal;
        float floatVal;
    };
    int childNum;   // 该节点的子节点个数
    struct Node_** children;  // 该节点的子节点数组
} Node;

// 树的创建、插入和遍历相关函数
Node* createNode(char* name, NodeType nodeType, int lineno, int childNum, Node** children);
void printTree(Node* root, int depth);

#endif