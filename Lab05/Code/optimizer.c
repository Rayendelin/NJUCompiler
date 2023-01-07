#include "optimizer.h"

BasicBlock* blocks;    // 基本块数组，初始容量设为64
int blocksCapacity;    // 基本块数组容量
int blocksSize;    // 基本块数组实际大小
// BasicBlock entryBlock;
// BasicBlock exitBlock;
BlockState* states;

// 获取一条空指令
InterCode getNullInterCode() {
    InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
    code1->kind = NULL_IR;
    return code1;
}

// 新建基本块
BasicBlock createBlock(char* func) {
    BasicBlock new = (BasicBlock)malloc(sizeof(BasicBlock_));
    new->no = blocksSize;
    new->interCodes = getNullInterCode();
    strcpy(new->func, func);
    return new;
}

// 将基本块加入基本块数组
void addBlock(BasicBlock block) {
    // 尚未给数组分配空间
    if (blocks == NULL) {
        blocksCapacity = 64;
        blocksSize = 0;
        blocks = (BasicBlock*)malloc(sizeof(BasicBlock)*blocksCapacity);
    }
    // 若当前数组没有空位，则开辟一个两倍大的新数组，将原数组的内容复制到新数组
    else if (blocksSize == blocksCapacity) {
        BasicBlock* tmp = (BasicBlock*)malloc(sizeof(BasicBlock)*2*blocksCapacity);
        for (int i = 0; i < blocksSize; i++)
            tmp[i] = blocks[i];
        blocks = tmp;
        blocksCapacity *= 2;
    }
    blocks[blocksSize] = block;
    blocksSize += 1;
    return;
}

// 向指令链表尾部插入多条指令组成的双向链表
void insertInterCode(InterCode code, InterCode interCodes) {
    if (interCodes == NULL) {
        printf("Inserting code to a null interCodes.\n");
        return;
    }
    if (code == NULL) {
        printf("Inserting a null code to interCodes has nothing to do.\n");
        return;
    }
    if (interCodes->next == NULL) {
        interCodes->next = code;
        if (code->next == NULL) {
            code->next = interCodes;
            code->pre = interCodes;
            interCodes->pre = code;
        }
        else {
            code->pre->next = interCodes;
            interCodes->pre = code->pre;
            code->pre = interCodes;
        }
    }
    else {
        interCodes->pre->next = code;
        if (code->next == NULL) {
            code->next = interCodes;
            code->pre = interCodes->pre;
            interCodes->pre = code;
        }
        else {
            code->pre->next = interCodes;
            InterCode codePre = code->pre;
            code->pre = interCodes->pre;
            interCodes->pre = codePre;
        }
    }
    return;
}

// 从指令链表中删除指令
void deleteInterCode(InterCode code) {
    code->pre->next = code->next;
    code->next->pre = code->pre;
}

// 根据字符串填充操作数信息（限五种操作数）
void fillOperand(char* name, Operand op) {
    // 临时变量
    if (name[0] == 't') {
        op->kind = TEMP_VAR_OP;
        sscanf(name, "t%d", &op->no);
    }
    // 变量
    else if (name[0] == 'v') {
        op->kind = VARIABLE_OP;
        sscanf(name, "v%d", &op->no);
    }
    // 取地址
    else if (name[0] == '&') {
        op->kind = GET_ADDR_OP;
        char tmp[32];
        sscanf(name, "&%s", tmp);
        Operand opr = (Operand)malloc(sizeof(Operand_));
        fillOperand(tmp, opr);
        op->opr = opr;
    }
    // 取值
    else if (name[0] == '*') {
        op->kind = GET_VAL_OP;
        char tmp[32];
        sscanf(name, "*%s", tmp);
        Operand opr = (Operand)malloc(sizeof(Operand_));
        fillOperand(tmp, opr);
        op->opr = opr;
    }
    // 常量
    else if (name[0] == '#') {
        op->kind = CONSTANT_OP;
        sscanf(name, "#%d", &op->value);
    }
}

// 读取输入
void readInput(char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        exit(1);
    }
    BasicBlock currBlock = NULL;   // 当前基本块
    char currFunc[32];  // 当前所在函数名
    // TODO：处理不了一行超过256个字符的情况
    char line[256];
    while (fgets(line, 256, f) != NULL) {
        int len = strlen(line); // 当前行的实际长度
        // 空行，舍去
        if (line[0] == '\n')
            continue;
        // FUNC_IR，分割基本块
        else if (line[0] == 'F') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = FUNC_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            op->kind = FUNCTION_OP;
            sscanf(line, "FUNCTION %s :", op->name);
            strcpy(currFunc, op->name);
            code->ops[0] = op;
            // 将之前基本块加入数组
            if (currBlock != NULL)
                addBlock(currBlock);
            currBlock = createBlock(currFunc);
            insertInterCode(code, currBlock->interCodes);
        }
        // LABEL_IR，分割基本块
        else if (line[0] == 'L') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = LABEL_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            op->kind = LABEL_OP;
            sscanf(line, "LABEL label%d :", &op->no);
            code->ops[0] = op;
            if (currBlock != NULL)
                addBlock(currBlock);
            currBlock = createBlock(currFunc);
            insertInterCode(code, currBlock->interCodes);
        }
        // GOTO_IR，基本块末尾
        else if (line[0] == 'G') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = GOTO_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            op->kind = LABEL_OP;
            sscanf(line, "GOTO label%d", &op->no);
            code->ops[0] = op;
            if (currBlock == NULL)
                currBlock = createBlock(currFunc);
            insertInterCode(code, currBlock->interCodes);
            addBlock(currBlock);
            currBlock = NULL;
        }
        // IF_GOTO_IR, 基本块末尾
        else if (line[0] == 'I') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = IF_GOTO_IR;
            Operand op1 = (Operand)malloc(sizeof(Operand_));
            Operand op2 = (Operand)malloc(sizeof(Operand_));
            Operand op3 = (Operand)malloc(sizeof(Operand_));
            char name1[32];
            char name2[32];
            op3->kind = LABEL_OP;
            sscanf(line, "IF %s %s %s GOTO label%d", name1, code->relop, name2, &op3->no);
            fillOperand(name1, op1);
            fillOperand(name2, op2);
            code->ops[0] = op1;
            code->ops[1] = op2;
            code->ops[2] = op3;
            if (currBlock == NULL)
                currBlock = createBlock(currFunc);
            insertInterCode(code, currBlock->interCodes);
            addBlock(currBlock);
            currBlock = NULL;
        }
        // RETURN_IR, 基本块末尾
        else if (line[0] == 'R' && line[2] == 'T') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = RETURN_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "RETURN %s", name);
            fillOperand(name, op);
            code->ops[0] = op;
            if (currBlock == NULL)
                currBlock = createBlock(currFunc);
            insertInterCode(code, currBlock->interCodes);
            addBlock(currBlock);
            currBlock = NULL;
        }
        // DEC_IR
        else if (line[0] == 'D') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = DEC_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "DEC %s %d", name, &code->size);
            fillOperand(name, op);
            code->ops[0] = op;
            insertInterCode(code, currBlock->interCodes);
        }
        // ARG_IR
        else if (line[0] == 'A') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = ARG_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "ARG %s", name);
            fillOperand(name, op);
            code->ops[0] = op;
            insertInterCode(code, currBlock->interCodes);
        }
        // PARAM_IR
        else if (line[0] == 'P') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = PARAM_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "PARAM %s", name);
            fillOperand(name, op);
            code->ops[0] = op;
            insertInterCode(code, currBlock->interCodes);
        }
        // READ_IR
        else if (line[0] == 'R' && line[2] == 'A') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = READ_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "READ %s", name);
            fillOperand(name, op);
            code->ops[0] = op;
            insertInterCode(code, currBlock->interCodes);
        }
        // WRITE_IR
        else if (line[0] == 'W') {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = WRITE_IR;
            Operand op = (Operand)malloc(sizeof(Operand_));
            char name[32];
            sscanf(line, "WRITE %s", name);
            fillOperand(name, op);
            code->ops[0] = op;
            insertInterCode(code, currBlock->interCodes);
        }
        // ASSIGN_IR, PLUS_IR, SUB_IR, MUL_IR, DIV_IR, CALL_IR（TO_MEM_IR不再特意给出）
        else {
            InterCode code = (InterCode)malloc(sizeof(InterCode_));
            code->kind = NULL_IR;
            for (int i = 0; i < len; i++) {
                if (line[i] == 'C') {
                    code->kind = CALL_IR;
                    break;
                }
                if (line[i] == '+') {
                    code->kind = PLUS_IR;
                    break;
                }
                if (line[i] == '-') {
                    code->kind = SUB_IR;
                    break;
                }
                // *两侧必须有空格，避免和取值符号混淆
                if (line[i] == '*' && i - 1 >= 0 && line[i-1] == ' ' && i + 1 < strlen(line) && line[i+1] == ' ') {
                    code->kind = MUL_IR;
                    break;
                }
                if (line[i] == '/') {
                    code->kind = DIV_IR;
                    break;
                }
            }
            if (code->kind == NULL_IR)
                code->kind = ASSIGN_IR;
            switch (code->kind) {
                case ASSIGN_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    char name2[32];
                    sscanf(line, "%s := %s", name1, name2);
                    fillOperand(name1, op1);
                    fillOperand(name2, op2);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
                case PLUS_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    Operand op3 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    char name2[32];
                    char name3[32];
                    sscanf(line, "%s := %s + %s", name1, name2, name3);
                    fillOperand(name1, op1);
                    fillOperand(name2, op2);
                    fillOperand(name3, op3);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    code->ops[2] = op3;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
                case SUB_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    Operand op3 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    char name2[32];
                    char name3[32];
                    sscanf(line, "%s := %s - %s", name1, name2, name3);
                    fillOperand(name1, op1);
                    fillOperand(name2, op2);
                    fillOperand(name3, op3);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    code->ops[2] = op3;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
                case MUL_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    Operand op3 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    char name2[32];
                    char name3[32];
                    sscanf(line, "%s := %s * %s", name1, name2, name3);
                    fillOperand(name1, op1);
                    fillOperand(name2, op2);
                    fillOperand(name3, op3);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    code->ops[2] = op3;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
                case DIV_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    Operand op3 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    char name2[32];
                    char name3[32];
                    sscanf(line, "%s := %s / %s", name1, name2, name3);
                    fillOperand(name1, op1);
                    fillOperand(name2, op2);
                    fillOperand(name3, op3);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    code->ops[2] = op3;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
                // TODO：暂时不清楚CALL_IR是否需要做基本块划分
                case CALL_IR: {
                    Operand op1 = (Operand)malloc(sizeof(Operand_));
                    Operand op2 = (Operand)malloc(sizeof(Operand_));
                    char name1[32];
                    op2->kind = FUNCTION_OP;
                    sscanf(line, "%s := CALL %s", name1, op2->name);
                    fillOperand(name1, op1);
                    code->ops[0] = op1;
                    code->ops[1] = op2;
                    insertInterCode(code, currBlock->interCodes);
                    break;
                }
            }
        }
    }
}

// 比较两个操作数是否等价
// flag为0，里外层都不比较版本号；flag为1，里外层都比较版本号；flag为2，外层不比较版本号，里层比较版本号
int opEqual(Operand op1, Operand op2, int flag) {
    if (op1 == NULL && op2 == NULL)
        return 1;
    else if (op1 == NULL || op2 == NULL)
        return 0;
    else if (flag == 1 && op1->ver != op2->ver)
        return 0;
    else if (op1->kind == op2->kind) {
        switch (op1->kind) {
            case FUNCTION_OP:
                return strcmp(op1->name, op2->name) == 0;
            case VARIABLE_OP:
            case TEMP_VAR_OP:
            case LABEL_OP:
                return op1->no == op2->no;
            case CONSTANT_OP:
                return op1->value == op2->value;
            case GET_ADDR_OP:
            case GET_VAL_OP:
                if (flag != 2)
                    return opEqual(op1->opr, op2->opr, flag);
                else
                    return opEqual(op1->opr, op2->opr, 1);
        }
    }
    return 0;
}

// 在DAG图数组中搜寻有没有已经代表该操作数（且ver最大）的节点
DAGNode searchDAGNode(Operand op, DAGNode* nodes, int nodesSize, int flag) {
    DAGNode res = NULL;
    int ver = 0;
    for (int i = 0; i < nodesSize; i++)
        // 找外层版本号最高的节点，因为提供的op一般里层版本号为0，所以比较里层版本号也没有意义
        if (opEqual(op, nodes[i]->op, flag) && nodes[i]->op->ver >= ver) {
            res = nodes[i];
            ver = res->op->ver;
        }
    return res;
}

// 将DAG图节点加入DAG图节点数组
void addDAGNode(DAGNode node, DAGNode** nodes, int* nodesCapacity, int* nodesSize) {
    // 尚未给数组分配空间
    if (*nodes == NULL) {
        *nodesCapacity = 32;
        *nodesSize = 0;
        *nodes = (DAGNode*)malloc(sizeof(DAGNode)*(*nodesCapacity));
    }
    // 若当前数组没有空位，则开辟一个两倍大的新数组，将原数组的内容复制到新数组
    else if (*nodesSize == *nodesCapacity) {
        DAGNode* tmp = (DAGNode*)malloc(sizeof(DAGNode)*2*(*nodesCapacity));
        for (int i = 0; i < *nodesSize; i++)
            tmp[i] = (*nodes)[i];
        *nodes = tmp;
        *nodesCapacity *= 2;
    }
    (*nodes)[*nodesSize] = node;
    *nodesSize += 1;
    return;
}

// 新建DAG图叶子节点
DAGNode createDAGLeafNode(Operand op) {
    DAGNode new = (DAGNode)malloc(sizeof(DAGNode_));
    new->kind = LEAF_NODE;
    new->op = op;
    op->ver = 0;
    return new;
}

// 新建DAG图节点
void createDAGNode(InterCode code, DAGNode** nodes, int* nodesCapacity, int* nodesSize) {
    DAGNode new = (DAGNode)malloc(sizeof(DAGNode_));
    if (code->kind == ASSIGN_IR) {
        new->kind = ASSIGN_NODE;
        Operand op1 = code->ops[0];
        Operand op2 = code->ops[1];
        DAGNode node1 = NULL;
        // 如果op1是取值操作数，那么要先搜索op1引用的操作数的最新版本，更新op1的内层版本
        if (op1->kind == GET_VAL_OP) {
            node1 = searchDAGNode(op1->opr, *nodes, *nodesSize, 0);
            // TODO: 这里可能是错误的
            // 似乎不可能找不到引用操作数的对应节点
            if (node1 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            // 找到之后，修改op1里层操作数的版本
            else
                op1->opr->ver = node1->op->ver;
        }
        node1 = NULL;
        // 确定op1的外层版本，采用外层宽松搜索，内层严格搜索的方式
        node1 = searchDAGNode(op1, *nodes, *nodesSize, 2);
        new->op = op1;
        // 确定外层版本号（取最新加一）
        if (node1 == NULL)
            op1->ver = 0;
        else
            op1->ver = node1->op->ver + 1;
        DAGNode node2 = NULL;
        if (op2->kind == GET_ADDR_OP || op2->kind == GET_VAL_OP) {
            node2 = searchDAGNode(op2->opr, *nodes, *nodesSize, 0);
            if (node2 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op2->opr->ver = node2->op->ver;
        }
        node2 = NULL;
        node2 = searchDAGNode(op2, *nodes, *nodesSize, 2);
        // 需要为op2建立一个节点
        if (node2 == NULL) {
            node2 = createDAGLeafNode(op2);
            addDAGNode(node2, nodes, nodesCapacity, nodesSize);
        }
        // 已存在对应节点，更新版本号
        else
            op2->ver = node2->op->ver;
        new->children[0] = node2;
        addDAGNode(new, nodes, nodesCapacity, nodesSize);
    }
    else if (code->kind == PLUS_IR || code->kind == SUB_IR || code->kind == MUL_IR || code->kind == DIV_IR) {
        if (code->kind == PLUS_IR)
            new->kind = PLUS_NODE;
        else if (code->kind == SUB_IR)
            new->kind = SUB_NODE;
        else if (code->kind == MUL_IR)
            new->kind = MUL_NODE;
        else
            new->kind = DIV_NODE;
        Operand op1 = code->ops[0];
        Operand op2 = code->ops[1];
        Operand op3 = code->ops[2];
        DAGNode node1 = NULL;
        if (op1->kind == GET_VAL_OP) {
            node1 = searchDAGNode(op1->opr, *nodes, *nodesSize, 0);
            if (node1 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op1->opr->ver = node1->op->ver;
        }
        node1 = NULL;
        node1 = searchDAGNode(op1, *nodes, *nodesSize, 2);
        new->op = op1;
        if (node1 == NULL)
            op1->ver = 0;
        else
            op1->ver = node1->op->ver + 1;
        DAGNode node2 = NULL;
        if (op2->kind == GET_ADDR_OP || op2->kind == GET_VAL_OP) {
            node2 = searchDAGNode(op2->opr, *nodes, *nodesSize, 0);
            if (node2 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op2->opr->ver = node2->op->ver;
        }
        node2 = NULL;
        node2 = searchDAGNode(op2, *nodes, *nodesSize, 2);
        if (node2 == NULL) {
            node2 = createDAGLeafNode(op2);
            addDAGNode(node2, nodes, nodesCapacity, nodesSize);
        }
        else
            op2->ver = node2->op->ver;
        new->children[0] = node2;
        DAGNode node3 = NULL;
        if (op3->kind == GET_ADDR_OP || op3->kind == GET_VAL_OP) {
            node3 = searchDAGNode(op3->opr, *nodes, *nodesSize, 0);
            if (node3 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op3->opr->ver = node3->op->ver;
        }
        node3 = NULL;
        node3 = searchDAGNode(op3, *nodes, *nodesSize, 2);
        if (node3 == NULL) {
            node3 = createDAGLeafNode(op3);
            addDAGNode(node3, nodes, nodesCapacity, nodesSize);
        }
        else
            op3->ver = node3->op->ver;
        new->children[1] = node3;
        addDAGNode(new, nodes, nodesCapacity, nodesSize);
    }
    // 建立叶子节点
    else if (code->kind == CALL_IR || code->kind == PARAM_IR || code->kind == READ_IR || code->kind == DEC_IR) {
        new->kind = LEAF_NODE;
        Operand op1 = code->ops[0];
        DAGNode node1 = NULL;
        if (op1->kind == GET_VAL_OP || op1->kind == GET_ADDR_OP) {
            node1 = searchDAGNode(op1->opr, *nodes, *nodesSize, 0);
            if (node1 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op1->opr->ver = node1->op->ver;
        }
        node1 = NULL;
        node1 = searchDAGNode(op1, *nodes, *nodesSize, 2);
        new->op = op1;
        if (node1 == NULL)
            op1->ver = 0;
        else
            op1->ver = node1->op->ver + 1;
        addDAGNode(new, nodes, nodesCapacity, nodesSize);
    }
    // 不会为以下指令中的操作数加入节点，只是更新一下操作数对应的版本号
    else if (code->kind == IF_GOTO_IR) {
        Operand op1 = code->ops[0];
        Operand op2 = code->ops[1];
        DAGNode node1 = NULL;
        if (op1->kind == GET_VAL_OP || op1->kind == GET_ADDR_OP) {
            node1 = searchDAGNode(op1->opr, *nodes, *nodesSize, 0);
            if (node1 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op1->opr->ver = node1->op->ver;
        }
        node1 = NULL;
        node1 = searchDAGNode(op1, *nodes, *nodesSize, 2);
        if (node1 == NULL)
            op1->ver = 0;
        else
            op1->ver = node1->op->ver;
        DAGNode node2 = NULL;
        if (op2->kind == GET_VAL_OP || op2->kind == GET_ADDR_OP) {
            node2 = searchDAGNode(op2->opr, *nodes, *nodesSize, 0);
            if (node2 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op2->opr->ver = node2->op->ver;
        }
        node2 = NULL;
        node2 = searchDAGNode(op2, *nodes, *nodesSize, 2);
        if (node2 == NULL)
            op2->ver = 0;
        else
            op2->ver = node2->op->ver;
    }
    else if (code->kind == ARG_IR || code->kind == WRITE_IR || code->kind == RETURN_IR) {
        Operand op1 = code->ops[0];
        DAGNode node1 = NULL;
        if (op1->kind == GET_VAL_OP || op1->kind == GET_ADDR_OP) {
            node1 = searchDAGNode(op1->opr, *nodes, *nodesSize, 0);
            if (node1 == NULL) {
                printf("Error: cannot find node in DAG!\n");
                return;
            }
            else
                op1->opr->ver = node1->op->ver;
        }
        node1 = NULL;
        node1 = searchDAGNode(op1, *nodes, *nodesSize, 2);
        if (node1 == NULL)
            op1->ver = 0;
        else
            op1->ver = node1->op->ver;
    }
}

// 创建常量
Operand getValue(int num) {
    Operand cons = (Operand)malloc(sizeof(Operand_));
    cons->kind = CONSTANT_OP;
    cons->value = num;
    return cons;
}

// 优化加法
void optimizePLUSIR(InterCode code) {
    Operand src1 = code->ops[1];
    Operand src2 = code->ops[2];
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(src1->value + src2->value);
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 0) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src2;
        code->ops[2] = NULL;
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 0) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src1;
        code->ops[2] = NULL;
    }
}

// 优化减法
void optimizeSUBIR(InterCode code) {
    Operand src1 = code->ops[1];
    Operand src2 = code->ops[2];
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(src1->value - src2->value);
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 0) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src1;
        code->ops[2] = NULL;
    }
}

// 优化乘法
void optimizeMULIR(InterCode code) {
    Operand src1 = code->ops[1];
    Operand src2 = code->ops[2];
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(src1->value * src2->value);
    }
    else if ((src1->kind == CONSTANT_OP && src1->value == 0) ||
             (src2->kind == CONSTANT_OP && src2->value == 0)) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(0);
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 1) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src2;
        code->ops[2] = NULL;
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 1) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src1;
        code->ops[2] = NULL;
    }
}

// 优化除法
void optimizeDIVIR(InterCode code) {
    Operand src1 = code->ops[1];
    Operand src2 = code->ops[2];
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(src1->value / src2->value);
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 0) {
        code->kind = ASSIGN_IR;
        code->ops[1] = getValue(0);
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 1) {
        code->kind = ASSIGN_IR;
        code->ops[1] = src1;
        code->ops[2] = NULL;
    }
}

// 优化有条件跳转
void optimizeIFGOTOIR(InterCode code) {
    if (code->ops[0]->kind == CONSTANT_OP && code->ops[1]->kind == CONSTANT_OP) {
        if ((strcmp(code->relop, "==") == 0 && code->ops[0]->value == code->ops[1]->value) ||
            (strcmp(code->relop, ">=") == 0 && code->ops[0]->value >= code->ops[1]->value) ||
            (strcmp(code->relop, "<=") == 0 && code->ops[0]->value <= code->ops[1]->value) ||
            (strcmp(code->relop, "!=") == 0 && code->ops[0]->value != code->ops[1]->value) ||
            (strcmp(code->relop, ">") == 0 && code->ops[0]->value > code->ops[1]->value) ||
            (strcmp(code->relop, "<") == 0 && code->ops[0]->value < code->ops[1]->value))
            deleteInterCode(code);
    }
}

// 对中间代码进行操作数代换
void replaceOp(Operand src, Operand dest, InterCode codes) {
    InterCode code = codes->next;
    while (code != codes) {
        switch(code->kind) {
            case ASSIGN_IR:
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR: {
                for (int i = 1; i < 3; i++) {
                    if (opEqual(code->ops[i], src, 1))
                        code->ops[i] = dest;
                    // 替换内层
                    else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && opEqual(code->ops[i]->opr, src, 1))
                        code->ops[i]->opr = dest;
                }
                // 完成替换之后对基本运算表达式进行一些化简
                if (code->kind == PLUS_IR)
                    optimizePLUSIR(code);
                else if (code->kind == SUB_IR)
                    optimizeSUBIR(code);
                else if (code->kind == MUL_IR)
                    optimizeMULIR(code);
                else if (code->kind == DIV_IR)
                    optimizeDIVIR(code);
                break;
            }
            case RETURN_IR:
            case WRITE_IR:
            case ARG_IR:
            case IF_GOTO_IR: {
                for (int i = 0; i < 3; i++) {
                    if (opEqual(code->ops[i], src, 1))
                        code->ops[i] = dest;
                    else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && opEqual(code->ops[i]->opr, src, 1))
                        code->ops[i]->opr = dest;
                }
                // 对IF_GOTO_IR进行一些化简
                if (code->kind == IF_GOTO_IR)
                    optimizeIFGOTOIR(code);
                break;
            }
            default:
                break;
        }
        code = code->next;
    }
}

// 删除对特定操作数赋值的中间代码
void deleteOp(Operand src, InterCode codes) {
    InterCode code = codes->next;
    while (code != codes) {
        switch (code->kind) {
            case ASSIGN_IR:
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR:
            case DEC_IR: {
                if (opEqual(code->ops[0], src, 1))
                    deleteInterCode(code);
            }
        }
        code = code->next;
    }
}

// 公共子表达式消除
void eliminateSubExp(InterCode codes, DAGNode** nodes, int* nodesCapacity, int* nodesSize) {
    int flag = 1;
    while (flag == 1 && *nodesSize != 0) {
        for (int i = *nodesSize - 1; i >= 0; i--) {
            DAGNode node = (*nodes)[i];
            if (node->kind == PLUS_NODE || node->kind == SUB_NODE || node->kind == MUL_NODE || node->kind == DIV_NODE) {
                for (int j = 0; j < i; j++) {
                    if ((*nodes)[j]->kind == node->kind && opEqual((*nodes)[j]->children[0]->op, node->children[0]->op, 1) && 
                        opEqual((*nodes)[j]->children[1]->op, node->children[1]->op, 1)) {
                            node->kind = ASSIGN_NODE;
                            node->children[0] = (*nodes)[j];
                            node->children[1] = NULL;
                            replaceOp(node->op, (*nodes)[j]->op, codes);
                            break;
                        }
                }
                if (node->kind == ASSIGN_NODE)
                    break;
            }
            if (i == 0)
                flag = 0;
        }
    }
}

// 无用代码消除
void eliminateDeadCode(InterCode codes, DAGNode** nodes, int* nodesCapacity, int* nodesSize) {
    int flag = 1;
    while (flag == 1 && *nodesSize != 0) {
        for (int i = 0; i < *nodesSize; i++) {
            DAGNode node = (*nodes)[i];
            // 检查该操作数是否未被使用且在该基本块内有下一次定义
            int use = 0;
            int define = 0;
            if (node->op->kind == VARIABLE_OP || node->op->kind == TEMP_VAR_OP || node->op->kind == GET_ADDR_OP || node->op->kind == GET_VAL_OP) {
                // 是否被用到通过遍历codes确定
                InterCode code = codes->next;
                while (code != codes) {
                    switch (code->kind) {
                        // 这些会带来新增节点的语句不比较第一个操作数（CALL，READ，PARAM虽然也会新增节点，但它们影响虚拟机的行为，所以不能修改）
                        case ASSIGN_IR:
                        case PLUS_IR:
                        case SUB_IR:
                        case MUL_IR:
                        case DIV_IR:
                        case DEC_IR: {
                            for (int i = 1; i < 3; i++) {
                                // 外层用到
                                if (opEqual(code->ops[i], node->op, 1))
                                    use = 1;
                                // 内层用到
                                else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && opEqual(code->ops[i]->opr, node->op, 1))
                                    use = 1;
                            }
                            break;
                        }
                        default: {
                            for (int i = 0; i < 3; i++) {
                                if (opEqual(code->ops[i], node->op, 1))
                                    use = 1;
                                else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && opEqual(code->ops[i]->opr, node->op, 1))
                                    use = 1;
                            }
                            break;
                        }
                    }
                    code = code->next;
                }
                for (int j = 0; j < *nodesSize; j++)
                    if (opEqual((*nodes)[j]->op, node->op, 0) && (*nodes)[j]->op->ver > node->op->ver)
                        define = 1;
                // 因为临时变量不跨基本块（？），所以临时变量只要不被用到就可以擦除
                // 因为任何变量都不会跨函数，所以只要基本块最后一句是RETURN，任何变量只要不被用到都可以擦除
                if (use == 0 && (codes->pre->kind == RETURN_IR || node->op->kind == TEMP_VAR_OP || (node->op->kind != TEMP_VAR_OP && define == 1))) {
                    deleteOp(node->op, codes);
                    node->kind = LEAF_NODE;
                    node->op = getValue(0);
                    node->children[0] = NULL;
                    node->children[1] = NULL;
                    break;
                }
            }
            if (i == *nodesSize - 1)
                flag = 0;
        }
    }
}

// 常量折叠
void foldConstant(InterCode codes, DAGNode** nodes, int* nodesCapacity, int* nodesSize) {
    int flag = 1;
    while (flag == 1 && *nodesSize != 0) {
        for (int i = 0; i < *nodesSize; i++) {
            // 赋值语句并且赋给变量的是一个常量
            if ((*nodes)[i]->kind == ASSIGN_NODE && (*nodes)[i]->children[0]->op->kind == CONSTANT_OP) {
                // 替换
                replaceOp((*nodes)[i]->op, (*nodes)[i]->children[0]->op, codes);
                // 修改原节点
                (*nodes)[i]->kind = LEAF_NODE;
                (*nodes)[i]->op = (*nodes)[i]->children[0]->op;
                (*nodes)[i]->children[0] = NULL;
                (*nodes)[i]->children[1] = NULL;
                break;
            }
            // 运算语句并且两个被操作数都是常量
            if ((*nodes)[i]->kind == PLUS_NODE && (*nodes)[i]->children[0]->op->kind == CONSTANT_OP && (*nodes)[i]->children[1]->op->kind == CONSTANT_OP) {
                Operand cons = getValue((*nodes)[i]->children[0]->op->value + (*nodes)[i]->children[1]->op->value);
                replaceOp((*nodes)[i]->op, cons, codes);
                (*nodes)[i]->kind = LEAF_NODE;
                (*nodes)[i]->op = cons;
                (*nodes)[i]->children[0] = NULL;
                (*nodes)[i]->children[1] = NULL;
                break;
            }
            if ((*nodes)[i]->kind == SUB_NODE && (*nodes)[i]->children[0]->op->kind == CONSTANT_OP && (*nodes)[i]->children[1]->op->kind == CONSTANT_OP) {
                Operand cons = getValue((*nodes)[i]->children[0]->op->value - (*nodes)[i]->children[1]->op->value);
                replaceOp((*nodes)[i]->op, cons, codes);
                (*nodes)[i]->kind = LEAF_NODE;
                (*nodes)[i]->op = cons;
                (*nodes)[i]->children[0] = NULL;
                (*nodes)[i]->children[1] = NULL;
                break;
            }
            if ((*nodes)[i]->kind == MUL_NODE && (*nodes)[i]->children[0]->op->kind == CONSTANT_OP && (*nodes)[i]->children[1]->op->kind == CONSTANT_OP) {
                Operand cons = getValue((*nodes)[i]->children[0]->op->value * (*nodes)[i]->children[1]->op->value);
                replaceOp((*nodes)[i]->op, cons, codes);
                (*nodes)[i]->kind = LEAF_NODE;
                (*nodes)[i]->op = cons;
                (*nodes)[i]->children[0] = NULL;
                (*nodes)[i]->children[1] = NULL;
                break;
            }
            if ((*nodes)[i]->kind == DIV_NODE && (*nodes)[i]->children[0]->op->kind == CONSTANT_OP && (*nodes)[i]->children[1]->op->kind == CONSTANT_OP) {
                Operand cons = getValue((*nodes)[i]->children[0]->op->value / (*nodes)[i]->children[1]->op->value);
                replaceOp((*nodes)[i]->op, cons, codes);
                (*nodes)[i]->kind = LEAF_NODE;
                (*nodes)[i]->op = cons;
                (*nodes)[i]->children[0] = NULL;
                (*nodes)[i]->children[1] = NULL;
                break;
            }
            // 没有可折叠常量
            if (i == *nodesSize - 1)
                flag = 0;
        }
    }
}

// 打印出DAG图节点数组以便debug
void debugNodes(DAGNode** nodes, int* nodesSize) {
    for (int i = 0; i < *nodesSize; i++) {
        printf("%d: ", i);
        printOperand((*nodes)[i]->op, stdout);
        if ((*nodes)[i]->op->kind == GET_ADDR_OP || (*nodes)[i]->op->kind == GET_VAL_OP)
            printf(".%d", (*nodes)[i]->op->opr->ver);
        printf(".%d\n", (*nodes)[i]->op->ver);
        // if (i == 15 || i == 5) {
        //     printOperand(nodes[i]->children[0]->op, stdout);
        //     printf(".%d\n", nodes[i]->children[0]->op->ver);
        //     printOperand(nodes[i]->children[1]->op, stdout);
        //     printf(".%d\n", nodes[i]->children[1]->op->ver);
        // }
    }
}

// 基本块内局部优化
void optimizeBlock(BasicBlock block) {
    InterCode code = block->interCodes->next;
    // 根节点数组
    DAGNode* nodes = (DAGNode*)malloc(sizeof(DAGNode)*32);
    int nodesCapacity = 32, nodesSize = 0;
    while (code != block->interCodes && code != NULL) {
        createDAGNode(code, &nodes, &nodesCapacity, &nodesSize);
        code = code->next;
    }
    // debugNodes(&nodes, &nodesSize);
    // 常量折叠
    foldConstant(block->interCodes, &nodes, &nodesCapacity, &nodesSize);
    // 重新生成DAG图，效率低但保证了各项优化之间的相互独立
    nodes = (DAGNode*)malloc(sizeof(DAGNode)*32);
    nodesCapacity = 32;
    nodesSize = 0;
    code = block->interCodes->next;
    while (code != block->interCodes && code != NULL) {
        createDAGNode(code, &nodes, &nodesCapacity, &nodesSize);
        code = code->next;
    }
    // debugNodes(&nodes, &nodesSize);
    eliminateSubExp(block->interCodes, &nodes, &nodesCapacity, &nodesSize);
    nodes = (DAGNode*)malloc(sizeof(DAGNode)*32);
    nodesCapacity = 32;
    nodesSize = 0;
    code = block->interCodes->next;
    while (code != block->interCodes && code != NULL) {
        createDAGNode(code, &nodes, &nodesCapacity, &nodesSize);
        code = code->next;
    }
    eliminateDeadCode(block->interCodes, &nodes, &nodesCapacity, &nodesSize);
}

// 获取函数对应基本块的起止范围
void getBeignEnd(char* func, int* begin, int* end) {
    for (int i = 0; i < blocksSize; i++)
        if (strcmp(blocks[i]->func, func) == 0) {
            *begin = i;
            break;
        }
    for (int i = blocksSize-1; i >= 0; i--)
        if (strcmp(blocks[i]->func, func) == 0) {
            *end = i;
            break;
        }
}

// 确定特定函数内部基本块之间的前驱后继关系
void genDataFlow(char* func) {
    // 初始化entry和exit
    // entryBlock = (BasicBlock)malloc(sizeof(BasicBlock_));
    // exitBlock = (BasicBlock)malloc(sizeof(BasicBlock_));
    // 确定该函数基本块的起止范围
    int begin = 0;
    int end = -1;
    getBeignEnd(func, &begin, &end);
    // 初始化
    for (int i = begin; i <= end; i++) {
        blocks[i]->dead = 0;
        blocks[i]->preNum = 0;
        blocks[i]->next[0] = NULL;
        blocks[i]->next[1] = NULL;
    }
    // 确定后继关系和前驱数量
    for (int i = begin; i <= end; i++) {
        // 基本块内第一条和最后一条语句
        InterCode first = blocks[i]->interCodes->next;
        InterCode last = blocks[i]->interCodes->pre;
        // 函数头所在基本块
        // if (first->kind == FUNC_IR)
        //     entryBlock->next[0] = blocks[i];
        // 函数return语句所在基本块
        if (last->kind == RETURN_IR) { }
        else if (last->kind == IF_GOTO_IR) {
            blocks[i]->next[0] = blocks[i+1];
            blocks[i+1]->preNum++;
            Operand label = last->ops[2];
            for (int j = begin; j <= end; j++)
                if (blocks[j]->interCodes->next->kind == LABEL_IR && opEqual(blocks[j]->interCodes->next->ops[0], label, 0)) {
                    blocks[i]->next[1] = blocks[j];
                    blocks[j]->preNum++;
                }
            if (blocks[i]->next[1] == NULL)
                printf("Error: cannot find matching label!\n");
        }
        else if (last->kind == GOTO_IR) {
            Operand label = last->ops[0];
            for (int j = begin; j <= end; j++)
                if (blocks[j]->interCodes->next->kind == LABEL_IR && opEqual(blocks[j]->interCodes->next->ops[0], label, 0)) {
                    blocks[i]->next[0] = blocks[j];
                    blocks[j]->preNum++;
                }
            if (blocks[i]->next[0] == NULL)
                printf("Error: cannot find matching label!\n");
        }
        // 基本块最后一句是一个普通指令
        else {
            blocks[i]->next[0] = blocks[i+1];
            blocks[i+1]->preNum++;
        }
    }
    // 确定前驱关系
    for (int i = begin; i <= end; i++) {
        // 判定基本块不可达
        if (blocks[i]->preNum == 0 && blocks[i]->interCodes->next->kind != FUNC_IR)
            blocks[i]->dead = 1;
        // 根据前驱数量开相应大小的数组
        blocks[i]->pre = (BasicBlock*)malloc(sizeof(BasicBlock)*blocks[i]->preNum);
        int size = 0;
        for (int j = begin; j <= end; j++)
            if (blocks[j]->next[0] == blocks[i] || blocks[j]->next[1] == blocks[i]) {
                blocks[i]->pre[size] = blocks[j];
                size++;
            }
    }
    // entryBlock->preNum = 0;
    // exitBlock->pre = (BasicBlock*)malloc(sizeof(BasicBlock)*exitBlock->preNum);
    // int size = 0;
    // for (int j = begin; j <= end; j++)
    //     if (blocks[j]->next[0] == exitBlock || blocks[j]->next[1] == exitBlock) {
    //         exitBlock->pre[size] = blocks[j];
    //         size++;
    //     }
    return;
}

// 常量传播的控制流约束函数
// 根据block->pre的out信息计算state->in
// i为基本块的序号
void propagateConstantg(int num) {
    if (blocks[num]->dead == 1)
        return;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    int* transValue = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < block->preNum; i++) {
        BasicBlock preBlock = block->pre[i];
        BlockState preState = NULL;
        for (int j = 0; j < blocksSize; j++)
            if (blocks[j] == preBlock) {
                preState = states[j];
                break;
            }
        if (preState == NULL) {
            // if (preBlock == entryBlock)
            //     preState = states[blocksSize];
            printf("Error: preState should not be null!\n");
            continue;
        }
        // 将state中的out和outValue与preState中的out和outValue合并
        for (int j = 0; j < state->size; j++) {
            if (preState->out[j] == 2 || trans[j] == 2)
                trans[j] = 2;
            else if (preState->out[j] == 1 && trans[j] == 1 && preState->outValue[j] != transValue[j])
                trans[j] = 2;
            else if (preState->out[j] == 0 && trans[j] == 0)
                trans[j] = 0;
            else if (preState->out[j] == 1 && trans[j] == 0) {
                trans[j] = 1;
                transValue[j] = preState->outValue[j];
            }
        }
    }
    // 将trans和transValue赋给in和inValue
    for (int i = 0; i < state->size; i++) {
        state->in[i] = trans[i];
        state->inValue[i] = transValue[i];
    }
}

// 常量传播的传递函数
// 根据state->in和block->interCodes填充state->out
// i为基本块的序号
// 返回out值是否改变
int propagateConstantf(int num) {
    if (blocks[num]->dead == 1)
        return 0;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    int* transValue = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < state->size; i++) {
        trans[i] = state->in[i];
        transValue[i] = state->inValue[i];
    }
    InterCode code = block->interCodes->next;
    while (code != block->interCodes) {
        // 只需要分析为变量赋值的语句
        if (code->ops[0] != NULL && code->ops[0]->kind == VARIABLE_OP) {
            switch (code->kind) {
                case ASSIGN_IR: {
                    if (code->ops[1]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = 1;
                        transValue[code->ops[0]->no] = code->ops[1]->value;
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[1]->no];
                        // 是定值才传值
                        if (trans[code->ops[0]->no] == 1)
                            transValue[code->ops[0]->no] = transValue[code->ops[1]->no];
                    }
                    else
                        trans[code->ops[0]->no] = 2;
                    break;
                }
                case PLUS_IR:
                case SUB_IR:
                case MUL_IR:
                case DIV_IR: {
                    int a = 0;
                    int b = 0;
                    if (code->ops[1]->kind == CONSTANT_OP && code->ops[2]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = 1;
                        a = code->ops[1]->value;
                        b = code->ops[2]->value;
                    }
                    else if (code->ops[1]->kind == CONSTANT_OP && code->ops[2]->kind == VARIABLE_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[2]->no];
                        a = code->ops[1]->value;
                        b = transValue[code->ops[2]->no];
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP && code->ops[2]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[1]->no];
                        a = transValue[code->ops[1]->no];
                        b = code->ops[2]->value;
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP && code->ops[2]->kind == VARIABLE_OP) {
                        if (trans[code->ops[1]->no] == 0 || trans[code->ops[2]->no] == 0)
                            trans[code->ops[0]->no] = 0;
                        else if (trans[code->ops[1]->no] == 1 && trans[code->ops[2]->no] == 1)
                            trans[code->ops[0]->no] = 1;
                        else
                            trans[code->ops[0]->no] = 2;
                        a = transValue[code->ops[1]->no];
                        b = transValue[code->ops[2]->no];
                    }
                    else
                        trans[code->ops[0]->no] = 2;
                    if (trans[code->ops[0]->no] == 1) {
                        if (code->kind == PLUS_IR)
                            transValue[code->ops[0]->no] = a + b;
                        else if (code->kind == SUB_IR)
                            transValue[code->ops[0]->no] = a - b;
                        else if (code->kind == MUL_IR)
                            transValue[code->ops[0]->no] = a * b;
                        else if (code->kind == DIV_IR)
                            transValue[code->ops[0]->no] = a / b;
                    }
                }
                // 统一定成nac
                case READ_IR:
                case CALL_IR:
                case DEC_IR:
                case PARAM_IR: {
                    trans[code->ops[0]->no] = 2;
                    break;
                }
            }
        }
        code = code->next;
    }
    // 看out是否有改变
    int res = 0;
    for (int i = 0; i < state->size; i++) {
        if (state->out[i] != trans[i]) {
            res = 1;
            state->out[i] = trans[i];
        }
        state->outValue[i] = transValue[i];
    }
    return res;
}

// 根据基本块的in信息进行常量代换
void propagateConsantr(int num) {
    if (blocks[num]->dead == 1)
        return;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    int* transValue = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < state->size; i++) {
        trans[i] = state->in[i];
        transValue[i] = state->inValue[i];
    }
    InterCode code = block->interCodes->next;
    while (code != block->interCodes) {
        // 常量替换
        switch(code->kind) {
            case ASSIGN_IR:
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR: {
                for (int i = 1; i < 3; i++)
                    if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP && trans[code->ops[i]->no] == 1)
                        code->ops[i] = getValue(transValue[code->ops[i]->no]);
                // 完成替换之后对基本运算表达式进行一些化简
                if (code->kind == PLUS_IR)
                    optimizePLUSIR(code);
                else if (code->kind == SUB_IR)
                    optimizeSUBIR(code);
                else if (code->kind == MUL_IR)
                    optimizeMULIR(code);
                else if (code->kind == DIV_IR)
                    optimizeDIVIR(code);
                break;
            }
            case RETURN_IR:
            case WRITE_IR:
            case ARG_IR:
            case IF_GOTO_IR: {
                for (int i = 0; i < 3; i++)
                    if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP && trans[code->ops[i]->no] == 1)
                        code->ops[i] = getValue(transValue[code->ops[i]->no]);
                if (code->kind == IF_GOTO_IR)
                    optimizeIFGOTOIR(code);
                break;
            }
            default:
                break;
        }
        // 更新trans和transValue
        if (code->ops[0] != NULL && code->ops[0]->kind == VARIABLE_OP) {
            switch (code->kind) {
                case ASSIGN_IR: {
                    if (code->ops[1]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = 1;
                        transValue[code->ops[0]->no] = code->ops[1]->value;
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[1]->no];
                        // 是定值才传值
                        if (trans[code->ops[0]->no] == 1)
                            transValue[code->ops[0]->no] = transValue[code->ops[1]->no];
                    }
                    else
                        // 其他情况都是nac
                        trans[code->ops[0]->no] = 2;
                    break;
                }
                case PLUS_IR:
                case SUB_IR:
                case MUL_IR:
                case DIV_IR: {
                    int a = 0;
                    int b = 0;
                    if (code->ops[1]->kind == CONSTANT_OP && code->ops[2]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = 1;
                        a = code->ops[1]->value;
                        b = code->ops[2]->value;
                    }
                    else if (code->ops[1]->kind == CONSTANT_OP && code->ops[2]->kind == VARIABLE_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[2]->no];
                        a = code->ops[1]->value;
                        b = transValue[code->ops[2]->no];
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP && code->ops[2]->kind == CONSTANT_OP) {
                        trans[code->ops[0]->no] = trans[code->ops[1]->no];
                        a = transValue[code->ops[1]->no];
                        b = code->ops[2]->value;
                    }
                    else if (code->ops[1]->kind == VARIABLE_OP && code->ops[2]->kind == VARIABLE_OP) {
                        if (trans[code->ops[1]->no] == 0 || trans[code->ops[2]->no] == 0)
                            trans[code->ops[0]->no] = 0;
                        else if (trans[code->ops[1]->no] == 1 && trans[code->ops[2]->no] == 1)
                            trans[code->ops[0]->no] = 1;
                        else
                            trans[code->ops[0]->no] = 2;
                        a = transValue[code->ops[1]->no];
                        b = transValue[code->ops[2]->no];
                    }
                    else
                        trans[code->ops[0]->no] = 2;
                    if (trans[code->ops[0]->no] == 1) {
                        if (code->kind == PLUS_IR)
                            transValue[code->ops[0]->no] = a + b;
                        else if (code->kind == SUB_IR)
                            transValue[code->ops[0]->no] = a - b;
                        else if (code->kind == MUL_IR)
                            transValue[code->ops[0]->no] = a * b;
                        else if (code->kind == DIV_IR)
                            transValue[code->ops[0]->no] = a / b;
                    }
                }
                // 统一定成nac
                case READ_IR:
                case CALL_IR:
                case DEC_IR:
                case PARAM_IR: {
                    trans[code->ops[0]->no] = 2;
                    break;
                }
            }
        }
        code = code->next;
    }
}

// 常量传播
// 思路是先把基本块的in和out分析出来，然后用基本块的in结合具体语句去分析
void propagateConsant(char* func) {
    // 因为临时变量不跨基本块（？），所以只需要统计所有变量的常量传播情况（暂时只涉及外层）
    int num = 0;
    int begin = 0;
    int end = -1;
    getBeignEnd(func, &begin, &end);
    for (int i = begin; i <= end; i++) {
        if (blocks[i]->dead != 1) {
            InterCode code = blocks[i]->interCodes->next;
            while (code != blocks[i]->interCodes) {
                for (int j = 0; j < 3; j++)
                    if (code->ops[j] != NULL && code->ops[j]->kind == VARIABLE_OP && code->ops[j]->no > num)
                        num = code->ops[j]->no;
                code = code->next;
            }
        }
    }
    states = (BlockState*)malloc(sizeof(BlockState)*(blocksSize));
    // 常量传播中，用整型表示变量的状态，0为未定义，1为定值，定值记录在value数组中，2为有多个可能的值
    for (int i = 0; i < blocksSize; i++) {
        states[i] = (BlockState)malloc(sizeof(BlockState_));
        states[i]->size = num + 1;
        states[i]->in = (int*)malloc(sizeof(int)*(states[i]->size));
        states[i]->out = (int*)malloc(sizeof(int)*(states[i]->size));
        states[i]->inValue = (int*)malloc(sizeof(int)*(states[i]->size));
        states[i]->outValue = (int*)malloc(sizeof(int)*(states[i]->size));
    }
    int flag = 1;
    while (flag == 1) {
        int res = 0;
        flag = 0;
        for (int i = begin; i <= end; i++) {
            if (blocks[i]->dead != 1) {
                propagateConstantg(i);
                res = propagateConstantf(i);
                if (res == 1)
                    flag = 1;
            }
        }
    }
    // 所有基本块的in和out信息都已经填充完毕，下面进行常量代换
    for (int i = begin; i <= end; i++)
        propagateConsantr(i);
}

// 公共子表达式消除

// 无用代码消除的控制流约束函数
void deleteDeadCodeg(int num) {
    if (blocks[num]->dead == 1)
        return;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < 2; i++) {
        if (block->next[i] == NULL)
            continue;
        BasicBlock nextBlock = block->next[i];
        BlockState nextState = NULL;
        for (int j = 0; j < blocksSize; j++)
            if (blocks[j] == nextBlock) {
                nextState = states[j];
                break;
            }
        if (nextState == NULL) {
            printf("Error: preState should not be null!\n");
            continue;
        }
        for (int j = 0; j < state->size; j++)
            if (nextState->in[j] == 1)
                trans[j] = 1;
    }
    for (int i = 0; i < state->size; i++)
        state->out[i] = trans[i];
}

// 无用代码消除的传递函数
int deleteDeadCodef(int num) {
    if (blocks[num]->dead == 1)
        return 0;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < state->size; i++)
        trans[i] = state->out[i];
    InterCode code = block->interCodes->pre;
    while (code != block->interCodes) {
        switch (code->kind) {
            // 定值语句
            case ASSIGN_IR:
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR:
            case READ_IR:
            case CALL_IR:
            case DEC_IR:
            case PARAM_IR: {
                if (code->ops[0]->kind == VARIABLE_OP)
                    trans[code->ops[0]->no] = 0;
                else if ((code->ops[0]->kind == GET_ADDR_OP || code->ops[0]->kind == GET_VAL_OP) && code->ops[0]->opr->kind == VARIABLE_OP)
                    trans[code->ops[0]->opr->no] = 1;
                for (int i = 1; i < 3; i++) {
                    // 有可能在内层使用到该变量
                    if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP)
                        trans[code->ops[i]->no] = 1;
                    else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && code->ops[i]->opr->kind == VARIABLE_OP)
                        trans[code->ops[i]->opr->no] = 1;
                }
                break;
            }
            default: {
                for (int i = 0; i < 3; i++) {
                    // 有可能在内层使用到该变量
                    if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP)
                        trans[code->ops[i]->no] = 1;
                    else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && code->ops[i]->opr->kind == VARIABLE_OP)
                        trans[code->ops[i]->opr->no] = 1;
                }
            }
        }
        code = code->pre;
    }
    int res = 0;
    for (int i = 0; i < state->size; i++)
        if (state->in[i] != trans[i]) {
            res = 1;
            state->in[i] = trans[i];
        }
    return res;
}

// 根据基本块的out信息进行死代码消除
int deleteDeadCoder(int num) {
    if (blocks[num]->dead == 1)
        return 0;
    BasicBlock block = blocks[num];
    BlockState state = states[num];
    int* trans = (int*)malloc(sizeof(int)*state->size);
    for (int i = 0; i < state->size; i++)
        trans[i] = state->out[i];
    InterCode code = block->interCodes->pre;
    while (code != block->interCodes) {
        int delete = 0;
        // 定值语句（并且不影响程序IO行为）可删
        switch(code->kind) {
            case ASSIGN_IR:
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR: {
                if (code->ops[0]->kind == VARIABLE_OP && trans[code->ops[0]->no] == 0) {
                    deleteInterCode(code);
                    delete = 1;
                }
                break;
            }
        }
        if (delete == 0) {
            switch (code->kind) {
                case ASSIGN_IR:
                case PLUS_IR:
                case SUB_IR:
                case MUL_IR:
                case DIV_IR:
                case READ_IR:
                case CALL_IR:
                case DEC_IR:
                case PARAM_IR: {
                    if (code->ops[0]->kind == VARIABLE_OP)
                        trans[code->ops[0]->no] = 0;
                    else if ((code->ops[0]->kind == GET_ADDR_OP || code->ops[0]->kind == GET_VAL_OP) && code->ops[0]->opr->kind == VARIABLE_OP)
                        trans[code->ops[0]->opr->no] = 1;
                    for (int i = 1; i < 3; i++) {
                        // 有可能在内层使用到该变量
                        if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP)
                            trans[code->ops[i]->no] = 1;
                        else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && code->ops[i]->opr->kind == VARIABLE_OP)
                            trans[code->ops[i]->opr->no] = 1;
                    }
                    break;
                }
                default: {
                    for (int i = 0; i < 3; i++) {
                        // 有可能在内层使用到该变量
                        if (code->ops[i] != NULL && code->ops[i]->kind == VARIABLE_OP)
                            trans[code->ops[i]->no] = 1;
                        else if (code->ops[i] != NULL && (code->ops[i]->kind == GET_ADDR_OP || code->ops[i]->kind == GET_VAL_OP) && code->ops[i]->opr->kind == VARIABLE_OP)
                            trans[code->ops[i]->opr->no] = 1;
                    }
                }
            }
        }
        code = code->pre;
    }
    // 删除行为可能会带来新的可消除的死代码
    int res = 0;
    for (int i = 0; i < state->size; i++)
        if (state->in[i] != trans[i]) {
            res = 1;
            state->in[i] = trans[i];
        }
    return res;
}

// 无用代码消除
void deleteDeadCode(char* func) {
    int num = 0;
    int begin = 0;
    int end = -1;
    getBeignEnd(func, &begin, &end);
    // 同样暂时只考虑外层变量
    for (int i = begin; i <= end; i++) {
        if (blocks[i]->dead != 1) {
            InterCode code = blocks[i]->interCodes->next;
            while (code != blocks[i]->interCodes) {
                for (int j = 0; j < 3; j++)
                    if (code->ops[j] != NULL && code->ops[j]->kind == VARIABLE_OP && code->ops[j]->no > num)
                        num = code->ops[j]->no;
                code = code->next;
            }
        }
    }
    states = (BlockState*)malloc(sizeof(BlockState)*(blocksSize));
    // 活跃变量分析中，用整型表示变量的状态，0为不活跃，1为活跃
    for (int i = 0; i < blocksSize; i++) {
        states[i] = (BlockState)malloc(sizeof(BlockState_));
        states[i]->size = num + 1;
        states[i]->in = (int*)malloc(sizeof(int)*(states[i]->size));
        states[i]->out = (int*)malloc(sizeof(int)*(states[i]->size));
    }
    int outFlag = 1;
    while (outFlag == 1) {
        int flag = 1;
        while (flag == 1) {
            int res = 0;
            flag = 0;
            for (int i = begin; i <= end; i++) {
                if (blocks[i]->dead != 1) {
                    deleteDeadCodeg(i);
                    res = deleteDeadCodef(i);
                    if (res == 1)
                        flag = 1;
                }
            }
        }
        int res = 0;
        outFlag = 0;
        for (int i = begin; i <= end; i++) {
            res = deleteDeadCoder(i);
            if (res == 1)
                outFlag = 1;
        }
    }
    // 如果基本块内某个被ASSIGN语句赋值的变量在出口处不活跃，那么可以使用复制传播
    for (int i = begin; i <= end; i++) {
        if (blocks[i]->dead != 1) {
            BasicBlock block = blocks[i];
            BlockState state = states[i];
            InterCode code = block->interCodes->next;
            while (code != block->interCodes) {
                if (code->kind == ASSIGN_IR && code->ops[0]->kind == VARIABLE_OP && state->out[code->ops[0]->no] == 0 && code->ops[1]->kind == TEMP_VAR_OP) {
                    Operand src = code->ops[0];
                    Operand dest = code->ops[1];
                    InterCode tmp = code;
                    while (tmp != block->interCodes) {
                        switch(tmp->kind) {
                            case ASSIGN_IR:
                            case PLUS_IR:
                            case SUB_IR:
                            case MUL_IR:
                            case DIV_IR: {
                                for (int i = 1; i < 3; i++) {
                                    if (opEqual(tmp->ops[i], src, 1))
                                        tmp->ops[i] = dest;
                                    else if (tmp->ops[i] != NULL && (tmp->ops[i]->kind == GET_ADDR_OP || tmp->ops[i]->kind == GET_VAL_OP) && opEqual(tmp->ops[i]->opr, src, 1))
                                        tmp->ops[i]->opr = dest;
                                }
                                break;
                            }
                            case RETURN_IR:
                            case WRITE_IR:
                            case ARG_IR:
                            case IF_GOTO_IR: {
                                for (int i = 0; i < 3; i++) {
                                    if (opEqual(tmp->ops[i], src, 1))
                                        tmp->ops[i] = dest;
                                    else if (tmp->ops[i] != NULL && (tmp->ops[i]->kind == GET_ADDR_OP || tmp->ops[i]->kind == GET_VAL_OP) && opEqual(tmp->ops[i]->opr, src, 1))
                                        tmp->ops[i]->opr = dest;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        tmp = tmp->next;
                    }
                    deleteInterCode(code);
                }
                code = code->next;
            }
        }
    }
}

// 循环不变代码外提
void loopCodeMotion(char* func) {
    int begin = 0;
    int end = -1;
    getBeignEnd(func, &begin, &end);
}

// 归纳变量强度削减


// 函数内全局优化
void optimizeFunc(char* func) {
    // 将该函数的基本块链接为数据流
    genDataFlow(func);
    // 常量传播
    propagateConsant(func);
    // 公共子表达式消除
    // 无用代码消除
    deleteDeadCode(func);
    // 常量折叠和控制流优化
    for (int i = 0; i < blocksSize; i++)
        optimizeBlock(blocks[i]);
    genDataFlow(func);
    // 循环不变代码外提
    loopCodeMotion(func);
    // 归纳变量强度削减
    // 控制流优化
    genDataFlow(func);
}

// 优化管道
void optimize() {
    // 对每个基本块做局部优化
    for (int i = 0; i < blocksSize; i++)
        optimizeBlock(blocks[i]);
    char func[32];
    for (int i = 0; i < blocksSize; i++) {
        // 新的函数
        if (strcmp(func, blocks[i]->func) != 0) {
            strcpy(func, blocks[i]->func);
            // 在函数内部进行全局优化
            optimizeFunc(func);
        }
    }
}

// 向指定文件中打印操作数
void printOperand(Operand op, FILE* fp) {
    if (op == NULL) {
        fputs("null", fp);
        return;
    }
    char out[32];
    switch (op->kind) {
        case VARIABLE_OP:
            sprintf(out, "v%d", op->no);
            fputs(out, fp);
            break;
        case TEMP_VAR_OP:
            sprintf(out, "t%d", op->no);
            fputs(out, fp);
            break;
        case CONSTANT_OP:
            sprintf(out, "#%d", op->value);
            fputs(out, fp);
            break;
        case LABEL_OP:
            sprintf(out, "label%d", op->no);
            fputs(out, fp);
            break;
        case FUNCTION_OP:
            sprintf(out, "%s", op->name);
            fputs(out, fp);
            break;
        case GET_ADDR_OP:
            fputs("&", fp);
            printOperand(op->opr, fp);
            break;
        case GET_VAL_OP:
            fputs("*", fp);
            printOperand(op->opr, fp);
            break;
        default:
            break;
    }
    return;
}

// 向指定文件中打印中间代码
void printInterCodes(InterCode interCodes, FILE* fp) {
    InterCode curr = interCodes->next;
    int flag = 1;
    while (flag == 1 || curr != interCodes) {
        if (flag == 0 && curr->kind != NULL_IR)
            fputs("\n", fp);
        flag = 0;
        switch(curr->kind) {
            case LABEL_IR:
                fputs("LABEL ", fp);
                printOperand(curr->ops[0], fp);
                fputs(" :", fp);
                break;
            case FUNC_IR:
                fputs("FUNCTION ", fp);
                printOperand(curr->ops[0], fp);
                fputs(" :", fp);
                break;
            case ASSIGN_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                break;
            case PLUS_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                fputs(" + ", fp);
                printOperand(curr->ops[2], fp);
                break;
            case SUB_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                fputs(" - ", fp);
                printOperand(curr->ops[2], fp);
                break;
            case MUL_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                fputs(" * ", fp);
                printOperand(curr->ops[2], fp);
                break;
            case DIV_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                fputs(" / ", fp);
                printOperand(curr->ops[2], fp);
                break;
            case TO_MEM_IR:
                fputs("*", fp);
                printOperand(curr->ops[0], fp);
                fputs(" := ", fp);
                printOperand(curr->ops[1], fp);
                break;
            case GOTO_IR:
                fputs("GOTO ", fp);
                printOperand(curr->ops[0], fp);
                break;
            case IF_GOTO_IR:
                fputs("IF ", fp);
                printOperand(curr->ops[0], fp);
                fputs(" ", fp);
                fputs(curr->relop, fp);
                fputs(" ", fp);
                printOperand(curr->ops[1], fp);
                fputs(" GOTO ", fp);
                printOperand(curr->ops[2], fp);
                break;
            case RETURN_IR:
                fputs("RETURN ", fp);
                printOperand(curr->ops[0], fp);
                break;
            case DEC_IR:
                fputs("DEC ", fp);
                printOperand(curr->ops[0], fp);
                char str[32];
                sprintf(str, " %d", curr->size);
                fputs(str, fp);
                break;
            case ARG_IR:
                fputs("ARG ", fp);
                printOperand(curr->ops[0], fp);
                break;
            case CALL_IR:
                printOperand(curr->ops[0], fp);
                fputs(" := CALL ", fp);
                printOperand(curr->ops[1], fp);
                break;
            case PARAM_IR:
                fputs("PARAM ", fp);
                printOperand(curr->ops[0], fp);
                break;
            case READ_IR:
                fputs("READ ", fp);
                printOperand(curr->ops[0], fp);
                break;
            case WRITE_IR:
                fputs("WRITE ", fp);
                printOperand(curr->ops[0], fp);
                break;
            default:
                break;
        }
        fflush(fp);
        curr = curr->next;
    }
}

// 打印输出
void writeOutput(char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror(filename);
        exit(1);
    }
    for (int i = 0; i < blocksSize; i++)
        if (blocks[i] != NULL && blocks[i]->interCodes != NULL && blocks[i]->dead != 1) {
            printInterCodes(blocks[i]->interCodes, f);
            if (i != blocksSize - 1)
                fputs("\n", f);
        }
    fclose(f);
}