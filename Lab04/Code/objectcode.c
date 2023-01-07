#include "objectcode.h"

extern InterCode interCodes;

RegDes regs[32];    // 寄存器描述符数组
FrameDes frames;    // 栈帧描述符链表
char currFuncName[32];  // 当前翻译到的函数的名字

// 初始化寄存器描述符数组
void initRegs() {
    for (int i = 0; i < 32; i++) {
        regs[i] = (RegDes)malloc(sizeof(RegDes_));
        regs[i]->free = 1;
        regs[i]->interval = 0;
        // 填写寄存器别名
        // 不使用，值永远为0
        if (i == 0)
            sprintf(regs[i]->name, "$zero");
        // 不使用，预留给汇编器
        else if (i == 1)
            sprintf(regs[i]->name, "$at");
        // 可使用，存放函数的返回值
        else if (i >= 2 && i <= 3)
            sprintf(regs[i]->name, "$v%d", i-2);
        // 可使用，用于存放函数的（前四个）参数
        else if (i >= 4 && i <= 7)
            sprintf(regs[i]->name, "$a%d", i-4);
        // 可任意使用，属于调用者保存的寄存器
        else if (i >= 8 && i <= 15)
            sprintf(regs[i]->name, "$t%d", i-8);
        // 可任意使用，属于被调用者保存的寄存器
        else if (i >= 16 && i <= 23)
            sprintf(regs[i]->name, "$s%d", i-16);
        // 可任意使用，属于调用者保存的寄存器
        else if (i >= 24 && i <= 25)
            sprintf(regs[i]->name, "$t%d", i-16);
        // 不使用，预留给汇编器
        else if (i >= 26 && i <= 27)
            sprintf(regs[i]->name, "$k%d", i-26);
        // 不使用，固定指向64K静态数据区的中央
        else if (i == 28)
            sprintf(regs[i]->name, "$gp");
        // 可使用，指向栈顶
        else if (i == 29)
            sprintf(regs[i]->name, "$sp");
        // 可使用，指向栈帧的底部
        else if (i == 30)
            sprintf(regs[i]->name, "$fp");
        // 可使用，用来保存函数的返回地址
        else if (i == 31)
            sprintf(regs[i]->name, "$ra");
    }
}

// 将所有可操作寄存器的free标记置1，表示不需要阻止抢占
void freeRegs() {
    for (int i = 8; i < 26; i++)
        regs[i]->free = 1;
}

// 将所有可操作寄存器清空
void clearRegs() {
    for (int i = 8; i < 26; i++) {
        regs[i]->free = 1;
        regs[i]->var = NULL;
        regs[i]->interval = 0;
    }
}

// 将所有可操作寄存器压栈
void pushAllRegs(FILE* fp) {
    fprintf(fp, "  addi $sp, $sp, -72\n");
    for (int i = 25; i >= 8; i--)
        fprintf(fp, "  sw %s, %d($sp)\n", regs[i]->name, 4*(i-8));
}

// 恢复可操作寄存器中的值
void popAllRegs(FILE* fp) {
    for (int i = 8; i < 26; i++)
        fprintf(fp, "  lw %s, %d($sp)\n", regs[i]->name, 4*(i-8));
    fprintf(fp, "  addi $sp, $sp, 72\n");
}

// 比较两个操作数是否等价
int opEqual(Operand op1, Operand op2) {
    if (op1 == NULL && op2 == NULL)
        return 1;
    else if (op1 == NULL || op2 == NULL)
        return 0;
    if (op1->kind == op2->kind) {
        switch (op1->kind) {
            case VARIABLE_OP:
            case FUNCTION_OP:
                return strcmp(op1->name, op2->name) == 0;
            case TEMP_VAR_OP:
            case LABEL_OP:
                return op1->no == op2->no;
            case CONSTANT_OP:
                return op1->value == op2->value;
            case GET_ADDR_OP:
            case GET_VAL_OP:
                return opEqual(op1->opr, op2->opr);
        }
    }
    return 0;
}

// 为操作数在栈帧描述符中生成变量描述符
VarDes createVarDes(Operand op, FrameDes frame) {
    // 首先查找是否已存在对应的变量描述符
    VarDes var = frame->vars;
    while (var != NULL) {
        if (opEqual(op, var->op))
            return var;
        var = var->next;
    }
    // 创建新的变量描述符
    var = (VarDes)malloc(sizeof(VarDes_));
    // var->regNo = -1;
    int extraOffset = 0;
    // main函数以外的其他函数要现在栈帧中保存全部可操作寄存器的旧值，所以会多出72个字节
    if (strcmp(frame->name, "main") != 0)
        extraOffset = 72;
    if (frame->vars == NULL)
        var->offset = getSize(op->type) + extraOffset;
    else
        var->offset = frame->vars->offset + getSize(op->type);
    var->op = op;
    var->next = frame->vars;
    frame->vars = var;
    return var;
}

// 从头到尾扫描一遍中间代码，然后初始化栈帧描述符表
void initFrames() {
    InterCode curr = interCodes;
    int flag = 1;
    while (flag == 1 || curr != interCodes) {
        flag = 0;
        switch (curr->kind) {
            case FUNC_IR: {
                // 创建一个对应该函数的新栈帧描述符并插入到链表首部
                FrameDes frame = (FrameDes)malloc(sizeof(FrameDes_));
                strcpy(frame->name, curr->ops[0]->name);
                frame->next = frames;
                frames = frame;
                break;
            }
            case ASSIGN_IR:
            case TO_MEM_IR:
            case IF_GOTO_IR: {
                Operand left = curr->ops[0];
                Operand right = curr->ops[1];
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP)
                    createVarDes(left, frames);
                if (right->kind == VARIABLE_OP || right->kind == TEMP_VAR_OP)
                    createVarDes(right, frames);
                break;
            }
            case PLUS_IR:
            case SUB_IR:
            case MUL_IR:
            case DIV_IR: {
                Operand left = curr->ops[0];
                Operand right1 = curr->ops[1];
                Operand right2 = curr->ops[2];
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP)
                    createVarDes(left, frames);
                if (right1->kind == VARIABLE_OP || right1->kind == TEMP_VAR_OP)
                    createVarDes(right1, frames);
                if (right2->kind == VARIABLE_OP || right2->kind == TEMP_VAR_OP)
                    createVarDes(right2, frames);
                break;
            }
            case RETURN_IR:
            case DEC_IR:
            case ARG_IR:
            case CALL_IR:
            case PARAM_IR:
            case READ_IR:
            case WRITE_IR: {
                if (curr->ops[0]->kind == VARIABLE_OP || curr->ops[0]->kind == TEMP_VAR_OP)
                    createVarDes(curr->ops[0], frames);
                break;
            }
            default:
                break;
        }
        curr = curr->next;
    }
}

FrameDes findCurrFrame() {
    FrameDes frame = frames;
    while (frame != NULL) {
        if (strcmp(frame->name, currFuncName) == 0)
            return frame;
        frame = frame->next;
    }
}

// 将一些必需的目标代码输入到文件
void initObjectCode(FILE* fp) {
    // 数据段标记
    fputs(".data\n", fp);
    // 输入提示语
    fputs("_prompt: .asciiz \"Enter an integer:\"\n", fp);
    // 换行符
    fputs("_ret: .asciiz \"\\n\"\n", fp);
    fputs(".globl main\n", fp);
    // 代码段标记
    fputs(".text\n", fp);
    // read函数目标代码
    fputs("read:\n", fp);
    // 打印输入提示语
    fputs("  li $v0, 4\n", fp);
    fputs("  la $a0, _prompt\n", fp);
    fputs("  syscall\n", fp);
    // 读入一个整型
    fputs("  li $v0, 5\n", fp);
    fputs("  syscall\n", fp);
    // 跳转返回地址
    fputs("  jr $ra\n", fp);
    fputs("\n", fp);
    // write函数目标代码
    fputs("write:\n", fp);
    fputs("  li $v0, 1\n", fp);
    fputs("  syscall\n", fp);
    fputs("  li $v0, 4\n", fp);
    fputs("  la $a0, _ret\n", fp);
    fputs("  syscall\n", fp);
    fputs("  move $v0, $0\n", fp);
    fputs("  jr $ra\n", fp);
}

// 将可操作寄存器中的变量保存到栈上
void spillReg(RegDes reg, FILE* fp) {
    // 寄存器中的变量已经存储在栈上的某个位置（在预先扫描时已经被安排好了）
    if (reg->var != NULL && (reg->var->op->kind == VARIABLE_OP || reg->var->op->kind == TEMP_VAR_OP))
        fprintf(fp, "  sw %s, %d($fp)\n", reg->name, -reg->var->offset);
}

// 更新寄存器的使用间隔
void updateInterval(RegDes reg) {
    for (int i = 8; i < 26; i++)
        regs[i]->interval++;
    reg->interval = 0;
}

// 为变量描述符分配寄存器, load用于指示是否需要装载寄存器，形如 x = y op z 的表达式中，为x分配寄存器就不需要装载，而为y和z分配时都需要
int allocateReg(VarDes var, FILE* fp, int load) {
    // 查找是否有空闲寄存器
    int i = 8;
    for (; i < 26; i++)
        if (regs[i]->var == NULL)
            break;
    // 存在空闲寄存器
    if (i >= 8 && i < 26) {
        regs[i]->var = var;
        updateInterval(regs[i]);
        // var->regNo = i;
        if (load == 1) {
            // 常量装载到寄存器中
            if (var->op->kind == CONSTANT_OP)
                fprintf(fp, "  li %s, %d\n", regs[i]->name, var->op->value);
            // 将栈中存储的变量的值装载到寄存器中
            else if (var->op->kind == VARIABLE_OP || var->op->kind == TEMP_VAR_OP)
                fprintf(fp, "  lw %s, %d($fp)\n", regs[i]->name, -var->offset);
        }
        return i;
    }
    // 不存在空闲寄存器
    else if (i == 26) {
        // 最长时间未使用算法
        // 先尝试找到一个free并且存放常量的寄存器
        for (i = 8; i < 26; i++)
            if (regs[i]->free == 1 && regs[i]->var->op->kind == CONSTANT_OP)
                break;
        // 然后找interval最大的那个寄存器
        if (i == 26) {
            int max = 0;
            int res = 8;
            for (i = 8; i < 26; i++)
                if (regs[i]->free == 1 && regs[i]->interval >= max) {
                    max = regs[i]->interval;
                    res = i;
                }
            i = res;
        }
        // 这里是个很隐蔽的错误，因为对编译器的翻译而言，寄存器中的变量变化是线性的
        // 而对于实际的机器执行而言，寄存器中的变量变化存在很多可能的分支，编译器仅能确保在一条语句的翻译过程中的变量正确
        // 所以这里并不能保存寄存器中的旧值，因为我们不知道在实际运行过程中到达这条语句的该寄存器中存放的是否还是那个变量
        // spillReg(regs[i], fp);
        // regs[i]->var->regNo = -1;
        regs[i]->var = var;
        updateInterval(regs[i]);
        // var->regNo = i;
        if (load == 1) {
            // 常量装载到寄存器中
            if (var->op->kind == CONSTANT_OP)
                fprintf(fp, "  li %s, %d\n", regs[i]->name, var->op->value);
            // 将栈中存储的变量的值装载到寄存器中
            else if (var->op->kind == VARIABLE_OP || var->op->kind == TEMP_VAR_OP)
                fprintf(fp, "  lw %s, %d($fp)\n", regs[i]->name, -var->offset);
        }
        return i;
    }
}

// 获取存放操作数的寄存器编号
int getReg(Operand op, FILE* fp, int load) {
    if (op->kind == VARIABLE_OP || op->kind == TEMP_VAR_OP) {
        FrameDes frame = findCurrFrame();
        // 在变量描述符链表中搜索该变量对应的描述符（一定存在）
        VarDes var = createVarDes(op, frame);
        if (var == NULL) {
            printf("Error: Var should exist.\n");
            exit(1);
        }
        // 这里同样是个很隐蔽的错误，正如上面所说，编译器仅能确保在一条语句的翻译过程的变量正确（至少在朴素寄存器分配算法中是这样）
        // 所以我们必须为每个变量分配新的寄存器而不是使用编译器中保存的信息来推断之前该变量存储在哪个寄存器中，这是绝对不行的
        // 记住编译器中保存的信息不等于机器运行过程中的实际信息，变量描述符里的这个regNo属性毫无用处
        // if (var->regNo == -1) {
        //     int res = allocateReg(var, fp, load);
        //     regs[res]->free = 0;
        //     return res;
        // }
        // else if (var->regNo >= 0) {
        //     regs[var->regNo]->free = 0;
        //     updateInterval(regs[var->regNo]);
        //     return var->regNo;
        // }
        int res = allocateReg(var, fp, load);
        regs[res]->free = 0;
        return res;
    }
    else if (op->kind == CONSTANT_OP) {
        // 如果常量为0直接返回0
        if (op->value == 0)
            return 0;
        // 不能去搜索是否已有存储该常量的寄存器，因为目标代码执行的顺序和现在翻译的顺序是不同的，要以机器执行的角度思考
        // 给该常量分配一个变量描述符，但这个描述符不会加入变量描述符链表
        VarDes var = (VarDes)malloc(sizeof(VarDes_));
        // var->regNo = -1;
        var->offset = -1;
        var->op = op;
        int res = allocateReg(var, fp, load);
        regs[res]->free = 0;
        return res;
    }
    return 0;
}

// 根据操作数的类型完成装载
int handleOp(Operand op, FILE* fp, int load) {
    if (op->kind == VARIABLE_OP || op->kind == TEMP_VAR_OP || op->kind == CONSTANT_OP)
        return getReg(op, fp, load);
    else if (op->kind == GET_VAL_OP) {
        int reg = getReg(op->opr, fp, load);
        fprintf(fp, "  lw %s, 0(%s)\n", regs[reg]->name, regs[reg]->name);
        return reg;
    }
    else if (op->kind == GET_ADDR_OP) {
        int reg = getReg(op->opr, fp, load);
        FrameDes frame = findCurrFrame();
        VarDes var = createVarDes(op->opr, frame);
        fprintf(fp, "  addi %s, $fp, %d\n", regs[reg]->name, -var->offset);
        return reg;
    }
}

// 将中间代码翻译为目标代码并向指定文件中打印
// 同时需要负责协调寄存器的分配，因为同一指令中各个变量分配的寄存器不能相互抢占（某些情况下是可以优化的，暂时不考虑）
void printObjectCodes(char* name) {
    FILE* fp = fopen(name, "w");
    if (fp == NULL) {
        printf("Cannot open file %s", name);
        return;
    }
    // 初始化
    initRegs();
    initFrames();
    initObjectCode(fp);
    // 将中间代码翻译成目标代码并输入到文件
    InterCode curr = interCodes;
    int flag = 1;
    while (flag == 1 || curr != interCodes) {
        flag = 0;
        switch (curr->kind) {
            case LABEL_IR: {
                fprintf(fp, "label%d:\n", curr->ops[0]->no);
                break;
            }
            case FUNC_IR: {
                fprintf(fp, "\n%s:\n", curr->ops[0]->name);
                // 将$fp的旧值压栈
                fprintf(fp, "  addi $sp, $sp, -4\n");
                fprintf(fp, "  sw $fp, 0($sp)\n");
                // 将$sp的值赋给$fp，该函数的栈帧从$fp开始
                fprintf(fp, "  move $fp, $sp\n");
                strcpy(currFuncName, curr->ops[0]->name);
                FrameDes frame = findCurrFrame();
                // 如果不是main函数，那么被调用函数需要将所有可操作寄存器保存到栈中并清空可操作寄存器
                if (strcmp(curr->ops[0]->name, "main") != 0) {
                    pushAllRegs(fp);
                    // 为变量和局部变量预留出空间
                    fprintf(fp, "  addi $sp, $sp, %d\n", -frame->vars->offset+72);
                }
                else
                    fprintf(fp, "  addi $sp, $sp, %d\n", -frame->vars->offset);
                clearRegs();
                // 处理函数的参数声明（即FUNC指令后的PARAM指令）
                int argCount = 0;
                while (curr->next != NULL && curr->next->kind == PARAM_IR) {
                    curr = curr->next;
                    argCount++;
                    int reg = handleOp(curr->ops[0], fp, 0);
                    // 将函数的前四个参数从特定寄存器装载到为形参分配的寄存器中
                    if (argCount <= 4)
                        fprintf(fp, "  move %s, %s\n", regs[reg]->name, regs[argCount+3]->name);
                    // 将函数的后几个参数从栈上装载到为形参分配的寄存器中
                    else
                        fprintf(fp, "  lw %s, %d($fp)\n", regs[reg]->name, 4*(argCount-4) + 4);
                    spillReg(regs[reg], fp);
                }
                break;
            }
            case ASSIGN_IR: {
                // 左侧操作数有三种可能的类型：variable_op, temp_var_op, get_val_op
                Operand left = curr->ops[0];
                // 右侧操作数有五种可能的类型：variable_op, temp_var_op, constant_op, get_addr_op, get_val_op
                Operand right = curr->ops[1];
                // 分类讨论
                int regRight = handleOp(right, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 0);
                    fprintf(fp, "  move %s, %s\n", regs[regLeft]->name, regs[regRight]->name);
                    spillReg(regs[regLeft], fp);
                }
                else if (left->kind == GET_VAL_OP) {
                    int regLeft = getReg(left->opr, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regRight]->name, regs[regLeft]->name);
                }
                break;
            }
            case PLUS_IR: {
                Operand left = curr->ops[0];
                Operand right1 = curr->ops[1];
                Operand right2 = curr->ops[2];
                int regRight1 = handleOp(right1, fp, 1);
                int regRight2 = handleOp(right2, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 0);
                    fprintf(fp, "  add %s, %s, %s\n", regs[regLeft]->name, regs[regRight1]->name, regs[regRight2]->name);
                    spillReg(regs[regLeft], fp);                
                }
                else if (left->kind == GET_VAL_OP) {
                    int regLeft1 = getReg(left->opr, fp, 0);
                    fprintf(fp, "  add %s, %s, %s\n", regs[regLeft1]->name, regs[regRight1]->name, regs[regRight2]->name);
                    int regLeft2 = getReg(left->opr, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regLeft1]->name, regs[regLeft2]->name);
                }
                break;
            }
            case SUB_IR: {
                Operand left = curr->ops[0];
                Operand right1 = curr->ops[1];
                Operand right2 = curr->ops[2];
                int regRight1 = handleOp(right1, fp, 1);
                int regRight2 = handleOp(right2, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 0);
                    fprintf(fp, "  sub %s, %s, %s\n", regs[regLeft]->name, regs[regRight1]->name, regs[regRight2]->name);
                    spillReg(regs[regLeft], fp);
                }
                else if (left->kind == GET_VAL_OP) {
                    int regLeft1 = getReg(left->opr, fp, 0);
                    fprintf(fp, "  sub %s, %s, %s\n", regs[regLeft1]->name, regs[regRight1]->name, regs[regRight2]->name);
                    int regLeft2 = getReg(left->opr, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regLeft1]->name, regs[regLeft2]->name);
                }
                break;
            }
            case MUL_IR: {
                Operand left = curr->ops[0];
                Operand right1 = curr->ops[1];
                Operand right2 = curr->ops[2];
                int regRight1 = handleOp(right1, fp, 1);
                int regRight2 = handleOp(right2, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 0);
                    fprintf(fp, "  mul %s, %s, %s\n", regs[regLeft]->name, regs[regRight1]->name, regs[regRight2]->name);
                    spillReg(regs[regLeft], fp);
                }
                else if (left->kind == GET_VAL_OP) {
                    int regLeft1 = getReg(left->opr, fp, 0);
                    fprintf(fp, "  mul %s, %s, %s\n", regs[regLeft1]->name, regs[regRight1]->name, regs[regRight2]->name);
                    int regLeft2 = getReg(left->opr, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regLeft1]->name, regs[regLeft2]->name);
                }
                break;
            }
            case DIV_IR: {
                Operand left = curr->ops[0];
                Operand right1 = curr->ops[1];
                Operand right2 = curr->ops[2];
                int regRight1 = handleOp(right1, fp, 1);
                int regRight2 = handleOp(right2, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 0);
                    fprintf(fp, "  div %s, %s\n", regs[regRight1]->name, regs[regRight2]->name);
                    fprintf(fp, "  mflo %s\n", regs[regLeft]->name);
                    spillReg(regs[regLeft], fp);
                }
                else if (left->kind == GET_VAL_OP) {
                    int regLeft1 = getReg(left->opr, fp, 0);
                    fprintf(fp, "  div %s, %s\n", regs[regRight1]->name, regs[regRight2]->name);
                    fprintf(fp, "  mflo %s\n", regs[regLeft1]->name);
                    int regLeft2 = getReg(left->opr, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regLeft1]->name, regs[regLeft2]->name);
                }
                break;
            }
            case TO_MEM_IR: {
                Operand left = curr->ops[0];
                Operand right = curr->ops[1];
                int regRight = handleOp(right, fp, 1);
                if (left->kind == VARIABLE_OP || left->kind == TEMP_VAR_OP) {
                    int regLeft = getReg(left, fp, 1);
                    fprintf(fp, "  sw %s, 0(%s)\n", regs[regRight]->name, regs[regLeft]->name);
                }
                break;
            }
            case GOTO_IR: {
                fprintf(fp, "  j label%d\n", curr->ops[0]->no);
                break;
            }
            case IF_GOTO_IR: {
                Operand left = curr->ops[0];
                Operand right = curr->ops[1];
                int regLeft = handleOp(left, fp, 1);
                int regRight = handleOp(right, fp, 1);
                char relop[4];
                if (strcmp(curr->relop, "==") == 0)
                    sprintf(relop, "beq");
                else if (strcmp(curr->relop, "!=") == 0)
                    sprintf(relop, "bne");
                else if (strcmp(curr->relop, ">") == 0)
                    sprintf(relop, "bgt");
                else if (strcmp(curr->relop, "<") == 0)
                    sprintf(relop, "blt");
                else if (strcmp(curr->relop, ">=") == 0)
                    sprintf(relop, "bge");
                else if (strcmp(curr->relop, "<=") == 0)
                    sprintf(relop, "ble");
                fprintf(fp, "  %s %s, %s, label%d\n", relop, regs[regLeft]->name, regs[regRight]->name, curr->ops[2]->no);
                break;
            }
            case RETURN_IR: {
                int reg = handleOp(curr->ops[0], fp, 1);
                fprintf(fp, "  move $v0, %s\n", regs[reg]->name);
                // 如果不是main函数，那么被调用函数需要将所有可操作寄存器保存到栈中并清空可操作寄存器
                if (strcmp(currFuncName, "main") != 0) {
                    // 弹出栈帧中的所有多余项并恢复寄存器的旧值
                    fprintf(fp, "  addi $sp, $fp, -72\n");
                    popAllRegs(fp);
                }
                else
                    fprintf(fp, "  move $sp, $fp\n");
                // 恢复$fp的旧值
                fprintf(fp, "  lw $fp, 0($sp)\n");
                fprintf(fp, "  addi $sp, $sp, 4\n");    
                fprintf(fp, "  jr $ra\n");
                break;
            }
            case DEC_IR:
                // DEC指令不需要翻译，因为在预先扫描的过程中已经为所有变量在栈中分配了空间
                break;
            case ARG_IR:
                // 传参代码一定是在CALL指令之前，所以不单独翻译，在CALL指令部分翻译
                break;
            case CALL_IR: {
                // 处理CALL指令之前的ARG指令
                InterCode preCode = curr->pre;
                int argCount = 0;
                // 为前四个之后的参数分配的寄存器编号
                // TODO: 这个数组的大小可能需要调整，如果存在一个参数特别多的函数的话
                int regNos[64];
                while (preCode != NULL && preCode->kind == ARG_IR) {
                    argCount++;
                    Operand arg = preCode->ops[0];
                    int reg = handleOp(arg, fp, 1);
                    // 函数的前四个存放在特定寄存器中
                    if (argCount <= 4)
                        fprintf(fp, "  move %s, %s\n", regs[argCount+3]->name, regs[reg]->name);
                    // 后面的参数存放在栈上
                    // 因为这些参数必须要连续存放，中间不能保存其他的东西，所以需要统一分配好寄存器再统一压栈
                    else
                        regNos[argCount-5] = reg;
                    preCode = preCode->pre;
                }
                // 将后面的参数连续压栈（参数压栈顺序为从后往前）
                for (int i = argCount - 5; i >= 0; i--) {
                    fprintf(fp, "  addi $sp, $sp, -4\n");
                    fprintf(fp, "  sw %s, 0($sp)\n", regs[regNos[i]]->name);
                }
                fputs("  addi $sp, $sp, -4\n", fp);
		        fputs("  sw $ra, 0($sp)\n", fp);
                fprintf(fp, "  jal %s\n", curr->ops[1]->name);
                fputs("  lw $ra, 0($sp)\n", fp);
		        fputs("  addi $sp, $sp, 4\n", fp);
                if (curr->ops[0]->kind == VARIABLE_OP || curr->ops[0]->kind == TEMP_VAR_OP) {
                    int regNo = getReg(curr->ops[0], fp, 0);
		            fprintf(fp, "  move %s, $v0\n", regs[regNo]->name);
                    spillReg(regs[regNo], fp);
                }
                else if (curr->ops[0]->kind == GET_VAL_OP) {
                    int regNo = getReg(curr->ops[0]->opr, fp, 1);
                    fprintf(fp, "  sw $v0, 0(%s)\n", regs[regNo]->name);
                }
                break;
            }
            case PARAM_IR:
                // 声明代码一定是在FUNC指令之后，所以不单独翻译，在FUNC指令部分翻译
                break;
            case READ_IR: {
                fputs("  addi $sp, $sp, -4\n", fp);
		        fputs("  sw $ra, 0($sp)\n", fp);
		        fputs("  jal read\n", fp);
		        fputs("  lw $ra, 0($sp)\n", fp);
		        fputs("  addi $sp, $sp, 4\n", fp);
		        if (curr->ops[0]->kind == VARIABLE_OP || curr->ops[0]->kind == TEMP_VAR_OP) {
                    int regNo = getReg(curr->ops[0], fp, 0);
		            fprintf(fp, "  move %s, $v0\n", regs[regNo]->name);
                    spillReg(regs[regNo], fp);
                }
                else if (curr->ops[0]->kind == GET_VAL_OP) {
                    int regNo = getReg(curr->ops[0]->opr, fp, 1);
                    fprintf(fp, "  sw $v0, 0(%s)\n", regs[regNo]->name);
                }
                break;
            }
            case WRITE_IR: {
                int regNo = handleOp(curr->ops[0], fp, 1);
                fprintf(fp, "  move $a0, %s\n", regs[regNo]->name);
                fputs("  addi $sp, $sp, -4\n", fp);
		        fputs("  sw $ra, 0($sp)\n", fp);
		        fputs("  jal write\n", fp);
		        fputs("  lw $ra, 0($sp)\n", fp);
		        fputs("  addi $sp, $sp, 4\n", fp);
                break;
            }
            default:
                break;
        }
        fflush(fp);
        // 多条指令之间应该不存在寄存器分配的抢占问题，所以在处理完一条指令后将所有寄存器的free标记置1
        freeRegs();
        curr = curr->next;
    }
    fclose(fp);
}