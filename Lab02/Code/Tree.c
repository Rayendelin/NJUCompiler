#include "Tree.h"
#include "malloc.h"

Node* createNode(char* name, NodeType nodeType, int lineno, int childNum, Node** children) {
    Node* res = (Node*)malloc(sizeof(Node));
    res->name = name;
    res->nodeType = nodeType;
    res->lineno = lineno;
    res->childNum = childNum;
    res->children = children;
    // 空串可能会向上传递
    int nullFlag = 1;
    for (int i = 0; i < res->childNum; i++)
        if ((res->children)[i]->nodeType != ENUM_SYN_NULL)
            nullFlag = 0;
    if (nodeType == ENUM_SYN_NOT_NULL && nullFlag == 1)
        res->nodeType = ENUM_SYN_NULL;
    return res;
}

void printTree(Node* root, int depth) {
    // 打印缩进
    if (root->nodeType != ENUM_SYN_NULL)
        for (int i = 0; i < depth; i++)
            printf("  ");
    // 根据节点类型选择打印该节点的部分信息
    switch (root->nodeType) {
        case ENUM_SYN_NOT_NULL:
            printf("%s (%d)\n", root->name, root->lineno);
            break;
        case ENUM_SYN_NULL:
            break;
        case ENUM_LEX_ID:
            printf("%s: %s\n", root->name, root->strVal);
            break;
        case ENUM_LEX_TYPE:
            printf("%s: %s\n", root->name, root->strVal);
            break;
        case ENUM_LEX_INT:
            printf("%s: %d\n", root->name, root->intVal);
            break;
        case ENUM_LEX_FLOAT:
            printf("%s: %f\n", root->name, root->floatVal);
            break;
        case ENUM_LEX_OTHER:
            printf("%s\n", root->name);
            break;
    }
    // 递归打印子节点
    for (int i = 0; i < root->childNum; i++) {
        printTree(root->children[i], depth + 1);
    }
}