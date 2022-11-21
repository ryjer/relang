#include "compiler.h"

/******************************************************************************
 *                                   语义分析
 * 符号表管理、静态分析（类型检查）、目标代码生成
 *****************************************************************************/
void 符号表_不扩容插入节点(符号表* 符号表指针, 符号节点 * 待插入节点指针);
void 释放符号节点链表(符号节点* 首节点);


static uint32_t 容量质数表[] = { //采用小于 2^n 的最大质数，构成容量质数表
    31, 61, 127,
    251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521,
    131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213,
    33554393, 67108859, 134217689, 268435399, 536870909, 1073741789, 2147483647, 4294967291
};

#define 符号表初始容量 (31)
#define 符号表装载因子(符号表指针) ((float)((符号表指针)->用量)/((符号表指针)->容量))
#define 符号表最大装载因子 (0.75)


/******************************************************************************
 *                            散列符号表 数据结构
 *****************************************************************************/
// 创建 一个空符号表 ，初始化后返回 符号表的指针
符号表 * 创建符号表(符号表* 上一符号表指针, int 嵌套等级, int 期望容量) {
    符号表 * 符号表指针 = (符号表*)malloc(sizeof(*符号表指针));
    符号表指针->上一符号表 = 上一符号表指针;
    符号表指针->嵌套等级 = 嵌套等级;
    符号表指针->用量 = 0;
    if (期望容量==0) {
        符号表指针->容量 = 符号表初始容量;
    }else{
        符号表指针->容量 = 期望容量;
    }
    符号表指针->散列区 = (符号节点**)malloc(期望容量*sizeof(符号节点));
    return 符号表指针;
}

// 扩容 符号表到下一级容量，接近翻倍扩容
void 扩容符号表(符号表* 符号表指针) {
    // 查找容量质数表，确定 新散列表 容量
    int 新容量;
    int 期望容量 = 符号表指针->容量 + 1;  //如果为0，则自动扩容到容量表下一项
    int i=0; // 从头到尾遍历容量表，直到找到足够容纳“期望容量”的最小质数
    while (容量质数表[i] < 期望容量) {
        i++;
    }
    新容量 = 容量质数表[i];
    // 创建一个更大的 临时符号表，用于扩容并转移散列区内的节点
    符号表 * 临时符号表指针 = 创建符号表(符号表指针->上一符号表, 符号表指针->嵌套等级, 新容量);
    // 遍历旧符号表表首节点指针数组，将各节点 移入"临时符号表"
    符号节点 * 当前节点;
    符号节点 * 上一节点;
    for (int i=0; i<(符号表指针->容量); i++) {
        if (符号表指针->散列区[i] != NULL) { //处理非空表项
            // 遍历单向链表，将原有各个节点插入新散列表
            当前节点 = 符号表指针->散列区[i];
            while(当前节点->下一节点 != NULL) {
                上一节点 = 当前节点;
                当前节点 = 当前节点->下一节点;
                符号表_不扩容插入节点(临时符号表指针, 上一节点);
            }
            // 插入尾节点
            符号表_不扩容插入节点(临时符号表指针, 当前节点);
        }
    }
    // 将数据从 临时符号表 移回 本符号表
    符号表指针->用量 = 临时符号表指针->用量;
    符号表指针->容量 = 临时符号表指针->容量;
    free(符号表指针->散列区); // 释放 原符号表 散列区，注意其中的节点已经全部移交给新符号表，其实际上已空
    符号表指针->散列区 = 临时符号表指针->散列区; //将新散列区 转移给 原符号表
    free(临时符号表指针);
}

// 用于散列表变容后的‘链表节点’重新插入，假定散列表剩余空间充足，‘不需要扩容’
void 符号表_不扩容插入节点(符号表* 符号表指针, 符号节点 * 待插入节点指针) {
    待插入节点指针->下一节点 = NULL; // 将 下一节点指针 置空，防止干扰
    // 计算 散列值
    uint32_t 键散列值 = unicode切片_计算散列值(待插入节点指针->符号.名字, 符号表指针->容量);
    // 插入节点检查 链表是否存在
    if (符号表指针->散列区[键散列值] == NULL) { //首节点不存在，作为首节点插入
        符号表指针->散列区[键散列值] = 待插入节点指针;
    } else { //首节点存在，作为新首节点插入
        待插入节点指针->下一节点 = 符号表指针->散列区[键散列值]; //待插入节点指向原头部节点
        符号表指针->散列区[键散列值] = 待插入节点指针; //散列表项 指向待插入节点
    }
    // 更新用量
    符号表指针->用量 ++;
}

// 释放 符号表
void 释放符号表(符号表* 符号表) {
    // 清空散列区，逐个清空散列桶
    for (int i=0; i<符号表->容量; i++) {
        if (符号表->散列区[i] != NULL) {
            释放符号节点链表(符号表->散列区[i]);
        }
    }
    // 释放 符号表节点
    free(符号表);
}

// 根据首节点指针，释放整个散列桶
void 释放符号节点链表(符号节点* 首节点) {
    符号节点* 当前节点;
    for (当前节点=首节点; 当前节点!=NULL; 当前节点=当前节点->下一节点) {
        free(当前节点);
    }
}

// 单表查询 符号，返回符号节点指针，失败返回NULL
符号节点 * 查询符号节点(符号表 * 符号表指针, unicode切片 名字) {
    // 计算‘键’的散列值
    uint32_t 键散列值 = unicode切片_计算散列值(名字, 符号表指针->容量);
    // 表内查找 对应节点
    for (符号节点* 当前节点=符号表指针->散列区[键散列值]; 当前节点!=NULL; 当前节点=当前节点->下一节点){
        if (unicode切片_判等(名字, 当前节点->符号.名字)) {
            return 当前节点;
        }
    }
    // 查询失败，返回NULL
    return NULL;
}

// 查找 符号，返回符号节点指针，失败返回NULL
符号节点 * 向上查找符号节点(符号表 * 符号表指针, unicode切片 名字) {
    // 向上逐表查询
    do {
        // 计算‘键’的散列值，每个表都要重新计算
        uint32_t 键散列值 = unicode切片_计算散列值(名字, 符号表指针->容量);
        // 表内，查找散列桶内 符号节点拉链
        for (符号节点* 当前节点=符号表指针->散列区[键散列值]; 当前节点!=NULL; 当前节点=当前节点->下一节点){
            if (unicode切片_判等(名字, 当前节点->符号.名字)) {
                return 当前节点;
            }
        }
    } while((符号表指针 = 符号表指针->上一符号表) != NULL);
    // 查找失败，返回空NULL
    return NULL;
}

// 插入符号，但是只初始化名字。如果对应名字已经存在，则插入失败返回NULL
符号节点 * 插入符号(符号表* 符号表指针, unicode切片 名字) {
    // 检查该 键 是否已存在
    符号节点 * 目标节点 = 查询符号节点(符号表指针, 名字);
    if (目标节点 != NULL) { // 键 已存在，插入失败，返回NULL
        return NULL;
    }
    // 该名字不存在，可以插入

    // 检查扩容，大于最大装载因子触发扩容
    if (符号表装载因子(符号表指针) >= 符号表最大装载因子) {
        扩容符号表(符号表指针);
    }
    // 创建新节点
    符号节点* 新节点 = (符号节点*)malloc(sizeof(*新节点));
    // 初始化节点
    新节点->符号.名字 = unicode切片_复制(名字);

    // 将符号插入表中
    uint32_t 键散列值 = unicode切片_计算散列值(新节点->符号.名字, 符号表指针->容量);
    if (符号表指针->散列区[键散列值] == NULL) { //键 不存在，作为唯一节点插入
        符号表指针->散列区[键散列值] = 新节点;
    } else { //散列碰撞，作为新首节点插入
        新节点->下一节点 = 符号表指针->散列区[键散列值];
        符号表指针->散列区[键散列值] = 新节点;
    }
    // 成功，返回新节点指针
    符号表指针->用量++;
    return 新节点;
}

// 调试用，散列表序列表打印
void 符号表_文件序列化打印(符号表* 符号表, FILE * 输出文件) {
    // 先打印结构体信息 {3 31 [
    fprintf(输出文件, "{%d %d/%d [\n",符号表->嵌套等级, 符号表->用量, 符号表->容量);
    // 再打印散列表信息，只打印非空元素及后链表
    符号节点 * 当前节点;
    符号节点 * 上一节点;
    for (int i=0; i<(符号表->容量); i++) {
        if (符号表->散列区[i] != NULL) {
            // 遍历单向链表，逐个打印 {3 31 [0:["
            fprintf(输出文件, "%d: [", i); 
            当前节点 = 符号表->散列区[i];
            while(当前节点->下一节点 != NULL){
                上一节点 = 当前节点;
                当前节点 = 当前节点->下一节点;
                // 打印上一节点 "{3 31 哈希 [0:[k:v "
                fputc('"', 输出文件);
                unicode切片_文件打印(上一节点->符号.名字, 输出文件);
                fprintf(输出文件, "\" ");
                // fprintf(输出文件, "\":%d ", 上一节点->符号.名字);
            }
            // 打印当前接节点 "{3 31 哈希 [0:[k:v k:v k:v] "
            fputc('"', 输出文件);
            unicode切片_文件打印(当前节点->符号.名字, 输出文件);
            fprintf(输出文件, "\"]\n");
            // fprintf(输出文件, "\":%d] ", 当前节点->符号.名字);
        }
    }
    if (符号表->用量 > 0) {
        fputc('\b', 输出文件); //抹去最后的空格
    }
    fprintf(输出文件, "]}");
}

/******************************************************************************
 *                                语义分析
 *****************************************************************************/




// gcc -g -Wall semantic.c parser.c lexer.c ../散列表_unicode切片_编码/散列表_unicode切片_编码.c ../unicode切片/unicode切片.c ../utf8_unicode/utf8_unicode.c && ./a.out 
// 符号表调试
int main() {
    符号表 * 表0 = 创建符号表(NULL, 0, 31);
    FILE * 输出文件 = stdout;
    // 插入符号
    fprintf(输出文件, "#### 表0 插入 ####\n");
    插入符号(表0, 原始utf8字符串转unicode切片("A"));
    插入符号(表0, 原始utf8字符串转unicode切片("BB"));
    插入符号(表0, 原始utf8字符串转unicode切片("abc"));
    符号表_文件序列化打印(表0, 输出文件);
    fputc('\n', 输出文件);
    // 查询
    fprintf(输出文件, "#### 表0 查询 ####\n");
    符号节点 * 节点指针;
    节点指针 = 查询符号节点(表0, 原始utf8字符串转unicode切片("abc"));
    fprintf(输出文件, "\"abc\": ");
    unicode切片_文件打印(节点指针->符号.名字, 输出文件); fputc('\n', 输出文件);
    // 释放节点，正常会发生段错误
    // fprintf(输出文件, "#### 释放 散列桶 ####\n");
    // 节点指针 = 查询符号节点(表0, 原始utf8字符串转unicode切片("abc"));
    // 释放符号节点链表(节点指针);
    // unicode切片_文件打印(节点指针->符号.名字, 输出文件); fputc('\n', 输出文件);
    // #################### 嵌套表 ####################
    fprintf(输出文件, "################ 嵌套表 ################\n");
    符号表 * 表1 = 创建符号表(表0, 1, 31);
    // 嵌套插入
    fprintf(输出文件, "#### 表1 插入 ####\n");
    插入符号(表1, 原始utf8字符串转unicode切片("王"));
    插入符号(表1, 原始utf8字符串转unicode切片("ikun"));
    插入符号(表1, 原始utf8字符串转unicode切片("甲乙丙"));
    fprintf(输出文件, "上级表: ");
    符号表_文件序列化打印(表1->上一符号表, 输出文件);
    fprintf(输出文件, "\n当前表: ");
    符号表_文件序列化打印(表1, 输出文件);
    // 向上查找
    fprintf(输出文件, "#### 表1 向上查询 ####\n");
    节点指针 = 向上查找符号节点(表1, 原始utf8字符串转unicode切片("ikun"));
    fprintf(输出文件, "\"ikun\": ");
    unicode切片_文件打印(节点指针->符号.名字, 输出文件); fputc('\n', 输出文件);
    节点指针 = 向上查找符号节点(表1, 原始utf8字符串转unicode切片("BB"));
    fprintf(输出文件, "\"BB\": ");
    unicode切片_文件打印(节点指针->符号.名字, 输出文件); fputc('\n', 输出文件);
    // 表释放，会报段错误
    fprintf(输出文件, "#### 表0 释放 ####\n");
    释放符号表(表0);
    fprintf(输出文件, "#### 表1 释放 ####\n");
    释放符号表(表1);
    // 符号表_文件序列化打印(表0, 输出文件);


    fputc('\n', 输出文件);
}