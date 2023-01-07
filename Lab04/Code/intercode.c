#include "intercode.h"

extern Entry symbolTable[HASH_SIZE];
extern Entry layersHead;

// 存储指令的双向链表的链表头节点
InterCode interCodes;

// 临时变量，标记的全局编号
int tmpVarNo = 1;
int labelNo = 1;

// 初始化双向链表
void initInterCodes() {}

// 向指令链表尾部插入多条指令组成的双向链表
void insertInterCode(InterCode code, InterCode interCodes) {
    if (interCodes == NULL) {
        printf("Cannot insert code to a null interCodes!\n");
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

// 向指定文件中打印中间代码
void printInterCodes(char* name) {
    FILE* fp = fopen(name, "w");
    if (fp == NULL) {
        printf("Cannot open file %s", name);
        return;
    }
    InterCode curr = interCodes;
    int flag = 1;
    while (flag == 1 || curr != interCodes) {
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
        // 空指令什么也不输出，也不需要换行
        if (curr->kind != NULL_IR)
            fputs("\n", fp);
        fflush(fp);
        curr = curr->next;
    }
    fclose(fp);
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
            sprintf(out, "%s", op->name);
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

// 创建临时变量
Operand newTemp() {
    Operand tmpVar = (Operand)malloc(sizeof(Operand_));
    tmpVar->kind = TEMP_VAR_OP;
    tmpVar->no = tmpVarNo;
    tmpVarNo++;
    return tmpVar;
}

// 创建临时标记
Operand newLabel() {
    Operand label = (Operand)malloc(sizeof(Operand_));
    label->kind = LABEL_OP;
    label->no = labelNo;
    labelNo++;
    return label;
}

// 创建常量
Operand getValue(int num) {
    Operand cons = (Operand)malloc(sizeof(Operand_));
    cons->kind = CONSTANT_OP;
    cons->value = num;
    return cons;
}

// 创建变量操作数
Operand getVar(char* name) {
    Operand var = (Operand)malloc(sizeof(Operand_));
    var->kind = VARIABLE_OP;
    //在变量操作数名前面加上一个v，防止某些名字和临时变量名重名
    sprintf(var->name, "v%s", name);
    return var;
}

// 创建函数操作数
Operand getFunc(char* name) {
    Operand func = (Operand)malloc(sizeof(Operand_));
    func->kind = FUNCTION_OP;
    strcpy(func->name, name);
    return func;
}

// 对某个操作数取地址
Operand getAddr(Operand op) {
    Operand addr = (Operand)malloc(sizeof(Operand_));
    addr->kind = GET_ADDR_OP;
    addr->opr = op;
    return addr;
}

// 对某个操作数解引用
Operand getVal(Operand op) {
    Operand val = (Operand)malloc(sizeof(Operand_));
    val->kind = GET_VAL_OP;
    val->opr = op;
    return val;
}

// 将src操作数拷贝给dest操作数
void operandCpy(Operand dest, Operand src) {
    dest->kind = src->kind;
    if (dest->kind == TEMP_VAR_OP || dest->kind == LABEL_OP)
        dest->no = src->no;
    else if (dest->kind == CONSTANT_OP)
        dest->value = src->value;
    else if (dest->kind == VARIABLE_OP || dest->kind == FUNCTION_OP)
        strcpy(dest->name, src->name);
    else
        dest->opr = src->opr;
    dest->type = src->type;
    dest->next = src->next;
    return;
}

// 计算结构体或数组变量的大小（字节数）
int getSize(Type type) {
    if (type == NULL)
        return 4;
    // 基本类型int，整型占据4个字节
    else if (type->kind == ENUM_BASIC && type->basic == INT_TYPE)
        return 4;
    else if (type->kind == ENUM_ARRAY)
        return type->array.size * getSize(type->array.elem);
    else if (type->kind == ENUM_STRUCT) {
        FieldList head = type->structure->head;
        int sum = 0;
        while (head != NULL) {
            int tmp = getSize(head->type);
            // 按四字节对齐
            if (tmp % 4 != 0)
                tmp = ((tmp / 4) + 1) * 4;
            sum += tmp;
            head = head->next;
        }
        return sum;
    }
}

// 获取一条空指令
InterCode getNullInterCode() {
    InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
    code1->kind = NULL_IR;
    return code1;
}

// 优化加法
InterCode optimizePLUSIR(Operand dest, Operand src1, Operand src2) {
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        operandCpy(dest, getValue(src1->value + src2->value));
        return getNullInterCode();
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 0 && 
             src2->kind != GET_ADDR_OP && src2->kind != GET_VAL_OP) {
        operandCpy(dest, src2);
        return getNullInterCode();
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 0 && 
             src1->kind != GET_ADDR_OP && src1->kind != GET_VAL_OP) {
        operandCpy(dest, src1);
        return getNullInterCode();
    }
    else {
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = PLUS_IR;
        code1->ops[0] = dest;
        code1->ops[1] = src1;
        code1->ops[2] = src2;
        return code1;
    }
}

// 优化减法
InterCode optimizeSUBIR(Operand dest, Operand src1, Operand src2) {
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        operandCpy(dest, getValue(src1->value - src2->value));
        return getNullInterCode();
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 0 &&
             src1->kind != GET_ADDR_OP && src1->kind != GET_VAL_OP) {
        operandCpy(dest, src1);
        return getNullInterCode();
    }
    else {
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = SUB_IR;
        code1->ops[0] = dest;
        code1->ops[1] = src1;
        code1->ops[2] = src2;
        return code1;
    }
}

// 优化乘法
InterCode optimizeMULIR(Operand dest, Operand src1, Operand src2) {
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        operandCpy(dest, getValue(src1->value * src2->value));
        return getNullInterCode();
    }
    else if ((src1->kind == CONSTANT_OP && src1->value == 0) ||
             (src2->kind == CONSTANT_OP && src2->value == 0)) {
        operandCpy(dest, getValue(0));
        return getNullInterCode();
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 1 && 
             src2->kind != GET_ADDR_OP && src2->kind != GET_VAL_OP) {
        operandCpy(dest, src2);
        return getNullInterCode();
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 1 && 
             src1->kind != GET_ADDR_OP && src1->kind != GET_VAL_OP) {
        operandCpy(dest, src1);
        return getNullInterCode();
    }
    else {
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = MUL_IR;
        code1->ops[0] = dest;
        code1->ops[1] = src1;
        code1->ops[2] = src2;
        return code1;
    }
}

// 优化除法
InterCode optimizeDIVIR(Operand dest, Operand src1, Operand src2) {
    if (src1->kind == CONSTANT_OP && src2->kind == CONSTANT_OP) {
        operandCpy(dest, getValue(src1->value / src2->value));
        return getNullInterCode();
    }
    else if (src1->kind == CONSTANT_OP && src1->value == 0) {
        operandCpy(dest, getValue(0));
        return getNullInterCode();
    }
    else if (src2->kind == CONSTANT_OP && src2->value == 1 && 
             src1->kind != GET_ADDR_OP && src1->kind != GET_VAL_OP) {
        operandCpy(dest, src1);
        return getNullInterCode();
    }
    else {
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = DIV_IR;
        code1->ops[0] = dest;
        code1->ops[1] = src1;
        code1->ops[2] = src2;
        return code1;
    }
}

// 找到语句链表中的最后一条非空语句或者空语句（如果没有非空语句的话）   
InterCode findLastInterCode(InterCode code) {
    InterCode last = code->pre;
    while (last->kind == NULL_IR && last != code)
        last = last->pre;
    return last;
}

// 优化后面跟着GOTO或LABEL的LABEL语句
void optimizeLABELBeforeGOTO(InterCode code, Operand label) {
    InterCode last = findLastInterCode(code);
    if (last->kind == LABEL_IR) {
        int no = last->ops[0]->no;
        last->kind = NULL_IR;
        InterCode curr = last;
        while (curr != code) {
            if (curr->kind == GOTO_IR && curr->ops[0]->no == no)
                curr->ops[0]->no = label->no;
            else if (curr->kind == IF_GOTO_IR && curr->ops[2]->no == no)
                curr->ops[2]->no = label->no;
            curr = curr->pre;
        }
    }
}

// 基本表达式的翻译
InterCode translateExp(Node* root, Operand place) {
    // 赋值表达式
    if (root->childNum == 3 && strcmp(root->children[1]->name, "ASSIGNOP") == 0) {
        // 单个变量作为左值
        if (root->children[0]->childNum == 1 && 
            strcmp(root->children[0]->children[0]->name, "ID") == 0) {
            Entry sym = findSymbolAll(root->children[0]->children[0]->strVal);
            Operand var = getVar(sym->name);
            Operand tmp1 = newTemp();
            // 右侧exp的运算结果存储在t1中
            InterCode code1 = translateExp(root->children[2], tmp1);
            // 把t1的值赋给左侧的左值
            InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
            code2->kind = ASSIGN_IR;
            code2->ops[0] = var;
            code2->ops[1] = tmp1;
            insertInterCode(code2, code1);
            // 将运算结果存回place
            if (place != NULL)
                operandCpy(place, var);
            return code1;
        }
        // 数组元素作为左值
        else if (root->children[0]->childNum == 4 && 
            strcmp(root->children[0]->children[1]->name, "LB") == 0) {
            // tmp1应当是数组的地址，tmp2应当是一个整型，tmp3是所取元素的偏移量
            Operand tmp1 = newTemp();
            Operand tmp2 = newTemp();
            Operand tmp3 = newTemp();
            InterCode code1 = translateExp(root->children[0]->children[0], tmp1);
            InterCode code2 = translateExp(root->children[0]->children[2], tmp2);
            int size = getSize(tmp1->type->array.elem);
            InterCode code3 = optimizeMULIR(tmp3, tmp2, getValue(size));
            insertInterCode(code2, code1);
            insertInterCode(code3, code1);
            // tmp4存储所取元素的首地址
            Operand tmp4 = newTemp();
            InterCode code4 = optimizePLUSIR(tmp4, tmp1, tmp3);
            insertInterCode(code4, code1);
            // tmp5存储的是右侧表达式的运算结果
            Operand tmp5 = newTemp();
            InterCode code5 = translateExp(root->children[2], tmp5);
            InterCode code6 = (InterCode)malloc(sizeof(InterCode_));
            code6->kind = TO_MEM_IR;
            code6->ops[0] = tmp4;
            code6->ops[1] = tmp5;
            insertInterCode(code5, code1);
            insertInterCode(code6, code1);
            // 将运算结果存回place
            if (place != NULL)
                operandCpy(place, getVal(tmp4));
            return code1;
        }
        // 结构体特定域作为左值
        else if (root->children[0]->childNum == 3 &&
            strcmp(root->children[0]->children[1]->name, "DOT") == 0) {
            // 获取域名
            char name[32];
            strcpy(name, root->children[0]->children[2]->strVal);
            Operand tmp1 = newTemp();
            // tmp1返回的是一个结构体的首地址，并且带有type属性
            InterCode code1 = translateExp(root->children[0]->children[0], tmp1);
            // 获取域的偏移量和类型
            int offset = 0;
            FieldList head = tmp1->type->structure->head;
            while (strcmp(head->name, name) != 0) {
                int tmp = getSize(head->type);
                // 按四字节对齐
                if (tmp % 4 != 0)
                    tmp = ((tmp / 4) + 1) * 4;
                offset += tmp;
                head = head->next;
            }
            // tmp2中存储域的首地址
            Operand tmp2 = newTemp();
            InterCode code2 = optimizePLUSIR(tmp2, tmp1, getValue(offset));
            insertInterCode(code2, code1);
            // tmp3存储的是右侧表达式的运算结果
            Operand tmp3 = newTemp();
            InterCode code3 = translateExp(root->children[2], tmp3);
            InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
            code4->kind = TO_MEM_IR;
            code4->ops[0] = tmp2;
            code4->ops[1] = tmp3;
            insertInterCode(code3, code1);
            insertInterCode(code4, code1);
            // 将运算结果存回place
            if (place != NULL)
                operandCpy(place, getVal(tmp2));
            return code1;
        }
    }
    // 加减乘除表达式
    else if (root->childNum == 3 && (
             strcmp(root->children[1]->name, "PLUS") == 0 ||
             strcmp(root->children[1]->name, "MINUS") == 0 ||
             strcmp(root->children[1]->name, "STAR") == 0 ||
             strcmp(root->children[1]->name, "DIV") == 0)) {
        Operand tmp1 = newTemp();
        Operand tmp2 = newTemp();
        InterCode code1 = translateExp(root->children[0], tmp1);
        InterCode code2 = translateExp(root->children[2], tmp2);
        insertInterCode(code2, code1);
        InterCode code3 = getNullInterCode();
        switch (root->children[1]->name[0]) {
            case 'P': code3 = optimizePLUSIR(place, tmp1, tmp2); break;
            case 'M': code3 = optimizeSUBIR(place, tmp1, tmp2); break;
            case 'S': code3 = optimizeMULIR(place, tmp1, tmp2); break;
            case 'D': code3 = optimizeDIVIR(place, tmp1, tmp2); break;
        }
        insertInterCode(code3, code1);
        return code1;
    }
    // 取负表达式
    else if (strcmp(root->children[0]->name, "MINUS") == 0) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[1], tmp1);
        InterCode code2 = optimizeSUBIR(place, getValue(0), tmp1);
        insertInterCode(code2, code1);
        return code1;
    }
    // 括号表达式
    else if (strcmp(root->children[0]->name, "LP") == 0) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[1], tmp1);
        // 优化：直接把place修改为tmp1
        if (place != NULL)
            operandCpy(place, tmp1);
        return code1;
    }
    // 条件表达式
    else if (root->childNum >= 2 && (
             strcmp(root->children[0]->name, "NOT") == 0 ||
             strcmp(root->children[1]->name, "RELOP") == 0 ||
             strcmp(root->children[1]->name, "AND") == 0 ||
             strcmp(root->children[1]->name, "OR") == 0)) {
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = ASSIGN_IR;
        code1->ops[0] = place;
        code1->ops[1] = getValue(0);
        InterCode code2 = translateCond(root, label1, label2);
        optimizeLABELBeforeGOTO(code2, label1);
        InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
        code3->kind = LABEL_IR;
        code3->ops[0] = label1;
        InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
        code4->kind = ASSIGN_IR;
        code4->ops[0] = place;
        code4->ops[1] = getValue(1);
        InterCode code5 = (InterCode)malloc(sizeof(InterCode_));
        code5->kind = LABEL_IR;
        code5->ops[0] = label2;
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        insertInterCode(code4, code1);
        insertInterCode(code5, code1);
        return code1;
    }
    else if (strcmp(root->children[0]->name, "ID") == 0) {
        // 单变量表达式
        if (root->childNum == 1) {
            Entry sym = findSymbolAll(root->children[0]->strVal);
            Operand var = getVar(sym->name);
            // 数组类型和结构体类型并且不是函数参数是局部变量——需要取地址指令
            if (sym->type->kind != ENUM_BASIC && sym->isArg == 0) {
                operandCpy(place, getAddr(var));
                place->type = sym->type;
                return getNullInterCode();
            }
            // 优化：不需要取地址指令，直接修改place
            else {
                operandCpy(place, var);
                place->type = sym->type;
                return getNullInterCode();
            }
        }
        // 函数调用表达式
        else {
            Entry sym = findSymbolFunc(root->children[0]->strVal);
            Operand func = getFunc(sym->name);
            // 无参函数
            if (root->childNum == 3) {
                // read函数
                if (strcmp(func->name, "read") == 0) {
                    InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
                    code1->kind = READ_IR;
                    code1->ops[0] = place;
                    return code1;
                }
                InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
                code1->kind = CALL_IR;
                code1->ops[0] = place;
                code1->ops[1] = func;
                return code1;
            }
            // 带参函数
            else if (root->childNum == 4) {
                Operand argList = (Operand)malloc(sizeof(Operand_));
                InterCode code1 = translateArgs(root->children[2], argList);
                // write函数
                if (strcmp(func->name, "write") == 0) {
                    InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
                    code2->kind = WRITE_IR;
                    code2->ops[0] = argList->next;
                    insertInterCode(code2, code1);
                    if (place != NULL)
                        operandCpy(place, getValue(0));
                    return code1;
                }
                Operand curr = argList->next;
                while (curr != NULL) {
                    InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
                    code2->kind = ARG_IR;
                    code2->ops[0] = curr;
                    insertInterCode(code2, code1);
                    curr = curr->next;
                }
                InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
                code3->kind = CALL_IR;
                code3->ops[0] = place;
                code3->ops[1] = func;
                insertInterCode(code3, code1);
                return code1;
            }
        }
    }
    // 整型表达式
    else if (strcmp(root->children[0]->name, "INT") == 0) {
        // 优化：直接把place改成一个常量操作数，不需要多加一条赋值指令
        Operand value = getValue(root->children[0]->intVal);
        operandCpy(place, value);
        // 输出一条空指令来避免空指针错误
        return getNullInterCode();
    }
    // 数组元素表达式
    else if (root->childNum == 4 && strcmp(root->children[1]->name, "LB") == 0) {
        // tmp1应当是地址，tmp2应当是一个整型
        Operand tmp1 = newTemp();
        Operand tmp2 = newTemp();
        Operand tmp3 = newTemp();
        InterCode code1 = translateExp(root->children[0], tmp1);
        InterCode code2 = translateExp(root->children[2], tmp2);
        insertInterCode(code2, code1);
        // place为NULL没必要继续算
        if (place != NULL) {
            int size = getSize(tmp1->type->array.elem);
            InterCode code3 = optimizeMULIR(tmp3, tmp2, getValue(size));
            insertInterCode(code3, code1);
            // 如果取出的数组元素还是一个数组，或者是一个结构体，则返回地址
            if (tmp1->type->array.elem->kind == ENUM_ARRAY || 
                tmp1->type->array.elem->kind == ENUM_STRUCT) {
                // 优化加法
                InterCode code4 = optimizePLUSIR(place, tmp1, tmp3);
                insertInterCode(code4, code1);
                place->type = tmp1->type->array.elem;
            }
            // 取出的数组元素是一个基本类型，则返回值
            else {
                Operand tmp4 = newTemp();
                InterCode code4 = optimizePLUSIR(tmp4, tmp1, tmp3);
                insertInterCode(code4, code1);
                operandCpy(place, getVal(tmp4));
            }
            return code1;
        }
        return code1;
    }
    // 取结构体域
    else if (root->childNum == 3 && strcmp(root->children[1]->name, "DOT") == 0) {
        // 获取域名
        char name[32];
        strcpy(name, root->children[2]->strVal);
        Operand tmp1 = newTemp();
        // tmp1返回的是一个结构体的首地址，并且带有type属性
        InterCode code1 = translateExp(root->children[0], tmp1);
        // 获取域的偏移量和类型
        int offset = 0;
        FieldList head = tmp1->type->structure->head;
        while (strcmp(head->name, name) != 0) {
            int tmp = getSize(head->type);
            // 按四字节对齐
            if (tmp % 4 != 0)
                tmp = ((tmp / 4) + 1) * 4;
            offset += tmp;
            head = head->next;
        }
        // place为NULL没必要算
        if (place != NULL) {
            InterCode code2 = getNullInterCode();
            if (head->type->kind == ENUM_BASIC) {
                Operand tmp2 = newTemp();
                code2 = optimizePLUSIR(tmp2, tmp1, getValue(offset));
                insertInterCode(code2, code1);
                operandCpy(place, getVal(tmp2));
            }
            else {
                code2 = optimizePLUSIR(place, tmp1, getValue(offset));
                place->type = head->type;
                insertInterCode(code2, code1);
            }
        }
        return code1;
    }
    return getNullInterCode();
}

// 函数参数的翻译模式
InterCode translateArgs(Node* root, Operand argList) {
    if (root->childNum == 1) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[0], tmp1);
        tmp1->next = argList->next;
        argList->next = tmp1;
        return code1;
    }
    else if (root->childNum == 3) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[0], tmp1);
        tmp1->next = argList->next;
        argList->next = tmp1;
        InterCode code2 = translateArgs(root->children[2], argList);
        insertInterCode(code2, code1);
        return code1;
    }
}

// 语句的翻译模式
InterCode translateStmt(Node* root) {
    if (strcmp(root->children[0]->name, "Exp") == 0) {
        return translateExp(root->children[0], NULL);
    }
    else if (strcmp(root->children[0]->name, "CompSt") == 0) {
        pushLayer();
        InterCode code1 = translateCompSt(root->children[0], NULL);
        popLayer();
        return code1;
    }
    else if (strcmp(root->children[0]->name, "RETURN") == 0) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[1], tmp1);
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = RETURN_IR;
        code2->ops[0] = tmp1;
        insertInterCode(code2, code1);
        return code1;
    }
    else if (strcmp(root->children[0]->name, "IF") == 0 && root->childNum == 5) {
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        InterCode code1 = translateCond(root->children[2], label1, label2);
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = LABEL_IR;
        code2->ops[0] = label1;
        InterCode code3 = translateStmt(root->children[4]);
        InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
        code4->kind = LABEL_IR;
        code4->ops[0] = label2;
        insertInterCode(code2, code1);
        if (code3 != NULL)
            insertInterCode(code3, code1);
        insertInterCode(code4, code1);
        return code1;
    }
    else if (strcmp(root->children[0]->name, "IF") == 0 && root->childNum == 7) {
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        Operand label3 = newLabel();
        InterCode code1 = translateCond(root->children[2], label1, label2);
        // code1的最后一条语句为goto labelfalse并且code1中只有这一条向labelflase的跳转语句，此时可以优化
        InterCode last = findLastInterCode(code1);
        if (last->kind == GOTO_IR && last->ops[0]->no == label2->no) {
            int count = 0;
            if (code1->kind == GOTO_IR && code1->ops[0]->no == label2->no)
                count++;
            InterCode curr = code1->pre;
            while (curr != code1) {
                if (curr->kind == GOTO_IR && curr->ops[0]->no == label2->no)
                    count++;
                curr = curr->pre;
            }
            // 优化：把labelFalse对应的语句提到条件表达式的IFGOTO语句后面，可以消除一个冗余的label
            if (count == 1) {
                last->kind = NULL_IR;
                InterCode code2 = translateStmt(root->children[6]);
                // 优化：如果code2的最后一句是LABEL语句，那么将code2中的所有GOTO语句中的该LABEL替换为LABEL3
                optimizeLABELBeforeGOTO(code2, label3);
                InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
                code3->kind = GOTO_IR;
                code3->ops[0] = label3;
                InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
                code4->kind = LABEL_IR;
                code4->ops[0] = label1;
                InterCode code5 = translateStmt(root->children[4]);
                optimizeLABELBeforeGOTO(code5, label3);
                InterCode code6 = (InterCode)malloc(sizeof(InterCode_));
                code6->kind = LABEL_IR;
                code6->ops[0] = label3;
                insertInterCode(code2, code1);
                insertInterCode(code3, code1);
                insertInterCode(code4, code1);
                insertInterCode(code5, code1);
                insertInterCode(code6, code1);
                return code1;
            }
        }
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = LABEL_IR;
        code2->ops[0] = label1;
        InterCode code3 = translateStmt(root->children[4]);
        optimizeLABELBeforeGOTO(code3, label3);
        InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
        code4->kind = GOTO_IR;
        code4->ops[0] = label3;
        InterCode code5 = (InterCode)malloc(sizeof(InterCode_));
        code5->kind = LABEL_IR;
        code5->ops[0] = label2;
        InterCode code6 = translateStmt(root->children[6]);
        optimizeLABELBeforeGOTO(code6, label3);
        InterCode code7 = (InterCode)malloc(sizeof(InterCode_));
        code7->kind = LABEL_IR;
        code7->ops[0] = label3;
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        insertInterCode(code4, code1);
        insertInterCode(code5, code1);
        insertInterCode(code6, code1);
        insertInterCode(code7, code1);
        return code1;
    }
    else if (strcmp(root->children[0]->name, "WHILE") == 0) {
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        Operand label3 = newLabel();
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = LABEL_IR;
        code1->ops[0] = label1;
        InterCode code2 = translateCond(root->children[2], label2, label3);
        InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
        code3->kind = LABEL_IR;
        code3->ops[0] = label2;
        InterCode code4 = translateStmt(root->children[4]);
        optimizeLABELBeforeGOTO(code4, label1);
        InterCode code5 = (InterCode)malloc(sizeof(InterCode_));
        code5->kind = GOTO_IR;
        code5->ops[0] = label1;
        InterCode code6 = (InterCode)malloc(sizeof(InterCode_));
        code6->kind = LABEL_IR;
        code6->ops[0] = label3;
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        insertInterCode(code4, code1);
        insertInterCode(code5, code1);
        insertInterCode(code6, code1);
        return code1;
    }
    return getNullInterCode();
}

// 条件表达式的翻译模式
InterCode translateCond(Node* root, Operand labelTrue, Operand labelFalse) {
    if (root->childNum >= 2 && strcmp(root->children[1]->name, "RELOP") == 0) {
        Operand tmp1 = newTemp();
        Operand tmp2 = newTemp();
        InterCode code1 = translateExp(root->children[0], tmp1);
        InterCode code2 = translateExp(root->children[2], tmp2);
        InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
        code3->kind = IF_GOTO_IR;
        code3->ops[0] = tmp1;
        code3->ops[1] = tmp2;
        code3->ops[2] = labelTrue;
        strcpy(code3->relop, root->children[1]->strVal);
        InterCode code4 = (InterCode)malloc(sizeof(InterCode_));
        code4->kind = GOTO_IR;
        code4->ops[0] = labelFalse;
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        insertInterCode(code4, code1);
        return code1;
    }
    else if (strcmp(root->children[0]->name, "NOT") == 0) {
        return translateCond(root->children[1], labelFalse, labelTrue);
    }
    else if (root->childNum >= 2 && (
             strcmp(root->children[1]->name, "AND") == 0 ||
             strcmp(root->children[1]->name, "OR") == 0)) {
        Operand label1 = newLabel();
        InterCode code1;
        switch (root->children[1]->name[0]) {
            case 'A': code1 = translateCond(root->children[0], label1, labelFalse); break;
            case 'O': code1 = translateCond(root->children[0], labelTrue, label1); break;
        }
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = LABEL_IR;
        code2->ops[0] = label1;
        InterCode code3 = translateCond(root->children[2], labelTrue, labelFalse);
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        return code1;
    }
    else {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root, tmp1);
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = IF_GOTO_IR;
        code2->ops[0] = tmp1;
        code2->ops[1] = getValue(0);
        code2->ops[2] = labelTrue;
        strcpy(code2->relop, "!=");
        InterCode code3 = (InterCode)malloc(sizeof(InterCode_));
        code3->kind = GOTO_IR;
        code3->ops[0] = labelFalse;
        insertInterCode(code2, code1);
        insertInterCode(code3, code1);
        return code1;
    }
}

void translateProgram(Node* root) {
    initSymbolTable();
    initInterCodes();
    InterCode code = translateExtDefList(root->children[0]);
    interCodes = code;
}

InterCode translateExtDefList(Node* root) {
    if (root->childNum != 0) {
        InterCode code1 = translateExtDef(root->children[0]);
        InterCode code2 = translateExtDefList(root->children[1]);
        if (code2 != NULL && code1 != NULL)
            insertInterCode(code2, code1);
        if (code1 != NULL)
            return code1;
        else if (code2 != NULL)
            return code2;
    }
    return getNullInterCode();
}

InterCode translateExtDef(Node* root) {
    Type type = Specifier(root->children[0]);
    // 结构体定义，是结构体，不是匿名类型，域定义没有产生错误
    if (type->kind == ENUM_STRUCT && type->structure->name != "" && type->structure->head != NULL) {
        Entry res = (Entry)malloc(sizeof(Entry_));
        strcpy(res->name, type->structure->name);
        // 需要保证对res->type->kind的改动不会影响到type
        res->type = (Type)malloc(sizeof(Type_));
        res->type->structure = type->structure;
        res->type->kind = ENUM_STRUCT_DEF;
        insertSymbol(res);
    }
    // 函数定义
    if (strcmp(root->children[1]->name, "FunDec") == 0) {
        Function func = FunDec(root->children[1]);
        // 生成FUNCTION和PARAM中间代码
        InterCode code1 = (InterCode)malloc(sizeof(InterCode_));
        code1->kind = FUNC_IR;
        code1->ops[0] = getFunc(func->name);
        FieldList head = func->head;
        while (head != NULL) {
            InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
            code2->kind = PARAM_IR;
            code2->ops[0] = getVar(head->name);
            insertInterCode(code2, code1);
            head = head->next;
        }
        // 向符号表中添加符号
        func->returnType = type;
        func->hasDefined = 1;
        Type newType = (Type)malloc(sizeof(Type_));
        newType->kind = ENUM_FUNC;
        newType->func = func;
        Entry res = (Entry)malloc(sizeof(Entry_));
        strcpy(res->name, func->name);
        res->type = newType;
        insertSymbol(res);
        pushLayer();
        InterCode code3 = translateCompSt(root->children[2], func->name);
        popLayer();
        if (code3 != NULL)
            insertInterCode(code3, code1);
        return code1;
    }
    return getNullInterCode();
}

InterCode translateCompSt(Node* root, char* funcName) {
    // 先把函数参数存进符号表
    if (funcName != NULL) {
        Entry sym = findSymbolFunc(funcName);
        FieldList parms = sym->type->func->head;
        while (parms != NULL) {
            Entry parm = (Entry)malloc(sizeof(Entry_));
            strcpy(parm->name, parms->name);
            parm->type = parms->type;
            // 标记该符号表条目为函数传入的参数
            parm->isArg = 1;
            insertSymbol(parm);
            parms = parms->next;
        }
    }
    InterCode code1 = translateDefList(root->children[1], ENUM_VAR);
    InterCode code2 = translateStmtList(root->children[2]);
    insertInterCode(code2, code1);
    return code1;
}

// 语句列表
InterCode translateStmtList(Node* root) {
    if (root->childNum == 2) {
        InterCode code1 = translateStmt(root->children[0]);
        InterCode code2 = translateStmtList(root->children[1]);
        if (code2 != NULL && code1 != NULL)
            insertInterCode(code2, code1);
        if (code1 != NULL)
            return code1;
        else if (code2 != NULL)
            return code2;
    }
    return getNullInterCode();
}

// 函数体局部变量的翻译模式
InterCode translateDefList(Node* root, IdType class) {
    if (root->childNum == 0)
        return getNullInterCode();
    else {
        InterCode code1 = translateDef(root->children[0], class);
        InterCode code2 = translateDefList(root->children[1], class);
        insertInterCode(code2, code1);
        return code1;
    }
}

InterCode translateDef(Node* root, IdType class) {
    Type type = Specifier(root->children[0]);
    InterCode code1 = getNullInterCode();
    FieldList res = translateDecList(root->children[1], type, class, code1);
    // 如果type是结构体或者数组，那么对每个定义的局部变量都要申请空间
    FieldList curr = res;
    while (curr != NULL) {
        if (curr->type->kind == ENUM_ARRAY || curr->type->kind == ENUM_STRUCT) {
            InterCode code2 = getNullInterCode();
            code2->kind = DEC_IR;
            code2->ops[0] = getVar(curr->name);
            code2->ops[0]->type = curr->type;
            code2->size = getSize(curr->type);
            insertInterCode(code2, code1);
        }
        curr = curr->next;
    }
    return code1;
}

FieldList translateDecList(Node* root, Type type, IdType class, InterCode code) {
    FieldList res = translateDec(root->children[0], type, class, code);
    if (root->childNum == 3) {
        if (res == NULL)
            res = translateDecList(root->children[2], type, class, code);
        else {
            FieldList tmp = res;
            while (tmp->next != NULL) tmp = tmp->next;
            tmp->next = translateDecList(root->children[2], type, class, code);
        }
    }
    return res;
}

FieldList translateDec(Node* root, Type type, IdType class, InterCode code) {
    FieldList res = VarDec(root->children[0], type, class);
    // 局部变量在声明时初始化
    if (class == ENUM_VAR && res != NULL && root->childNum == 3) {
        Operand tmp1 = newTemp();
        InterCode code1 = translateExp(root->children[2], tmp1);
        insertInterCode(code1, code);
        InterCode code2 = (InterCode)malloc(sizeof(InterCode_));
        code2->kind = ASSIGN_IR;
        code2->ops[0] = getVar(res->name);
        code2->ops[1] = tmp1;
        insertInterCode(code2, code);
    }
    return res;
}