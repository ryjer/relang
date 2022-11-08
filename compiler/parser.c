#include "compiler.h"
#include "lexer.h"
#include <stdio.h>
#include "../散列表_unicode切片_编码/散列表_unicode切片_编码.h"

#define 解析器调试

#ifdef 解析器调试
        extern 单词信息体 当前单词;
        void 打印缩进();
        int 文法等级 = 0;
        FILE * 解析器调试输出文件;
        void 打印缩进() {
            for (int i=0; i<文法等级; i++) {
                fprintf(解析器调试输出文件, "  ");
            }
        }
        void 打印文法符号(char * 当前文法符号) {
            打印缩进();
            fprintf(解析器调试输出文件, "|-%s ", 当前文法符号);
            if (当前单词.类型 != 换行) {
                unicode切片_文件打印(当前单词.单词切片, 解析器调试输出文件);
            }else {
                fprintf(解析器调试输出文件, "\\n");
            }
            fputc('\n', 解析器调试输出文件);
        }
#endif
/******************************************************************************
 *                                   语法分析
 * 词法分析 >> 单词信息体 》》  语法分析器  》》 语法分析树 -> 抽象语法树
 * 原理：LL(1)右读向下分析文法 + 表达式算符优先文法
 *****************************************************************************/
// 维护函数
单词信息体 读入下一单词();

// LL(1) 产生式对应函数;
表达式节点 * 表达式_解析();
// void 表达式_算符优先解析();
表达式节点 * 基本表达式_解析();
表达式节点 * 一元表达式_解析();


// 基础结构解析
void 标识符列表_解析();
    void 标识符列表尾_解析();
void 表达式列表_解析();
    void 表达式列表尾_解析();
void 参数列表_解析();
    void 参数列表尾_解析();

void 常数表达式列表_解析(); //常数表达式，结果为数字
    void 常数表达式列表尾_解析();
    void 常数表达式_解析();

// 复杂结构 解析
void 包声明行_解析();
void 导入声明_解析();
    void 导入声明尾_解析();
    void 导入描述组_解析();
    void 导入描述行_解析();

void 声明区_解析();
void 访问修饰声明_解析();
void 声明_解析();
void 类型声明_解析();
    void 类型声明尾_解析();
    void 类型描述组_解析();
    void 类型描述行_解析();
        void 类型描述_解析();
        void 基本类型名_解析();
        void 构造类型描述_解析();
            void 地址类型描述_解析();
            void 数组类型描述_解析();
            void 结构体描述_解析();
            void 共用体描述_解析();
                void 字段组_解析();
                void 访问修饰字段行_解析();
                void 字段行_解析();
            void 枚举体描述_解析();
                void 枚举成员组_解析();
                void 枚举成员项_解析();

    void 常量声明_解析();
        void 常量声明尾_解析();
        void 常量描述组_解析();
        void 常量描述行_解析();
        void 常数表达式_解析();
    void 变量声明_解析();
        void 变量声明尾_解析();
        void 变量描述组_解析();
        void 变量描述行_解析();
    // void 宏声明_解析();
    void 过程声明_解析();
    // void 类声明_解析();
    // void 函数声明_解析();
    // void 算符声明_解析();

void 语句块_解析();
void 裸语句块_解析();
void 语句组_解析();
void 语句行_解析();
void 语句_解析();
    void 简单语句_解析();
    void 中断语句_解析();
    void 继续语句_解析();
    void 分支语句_解析();

static uint8_t 二元中缀优先级表[] = {
    // 2 赋值运算符 := += -= *= ·= ×= /= ÷= %=    &= ^= |= <<= >>= 右结合
    [冒等于]=2, [加等于]=2, [减等于]=2, [星等于]=2, [点乘等于]=2, [叉乘等于]=2,
    [斜杠等于]=2, [除等于]=2, [百分等于]=2,
    [与等于]=2, [杨抑等于]=2, [竖线等于]=2, [双小于等于]=2, [双大于等于]=2,
    // 3 条件运算符 ?:  右结合
    // 4 逻辑OR：|| ∨ 
    [双竖线]=4, [逻辑或]=4,
    // 5 逻辑AND：&& ∧
    [双与号]=5, [逻辑与]=5,
    // 6 位或OR：|
    [竖线]=6,
    // 7 位异或XOR：^
    [杨抑号]=7,
    // 8 位与AND：&
    [与号]=8,
    // 9 判等 = != ≠
    [等于号]=9, [感叹等于]=9, [不等于号]=9,
    // 10 比较 < <= ≤ > >= ≥
    [小于号]=10, [小于等于]=10, [小于等于号]=10, [大于号]=10, [大于等于]=10, [大于等于号]=10,
    // 11 移位运算符：<<  >>
    [双小于]=11, [双大于]=11,
    // 12 加减法运算符：+ -
    [加号]=12, [减号]=12,
    // 13 乘除模运算符：* · ×  / ÷  %
    [星号]=13, [点乘号]=13, [叉乘号]=13, [斜杠]=13, [除号]=13, [百分号]=13,
};
static uint8_t 一元前缀优先级表[] = {
    // 14 一元前缀运算符：! ￢ ¬ 按位取反~ + - 解引用* 取地址& sizeof _Alignof 右结合
    [感叹号]=14, [逻辑非]=14, [波浪号]=14, /* [加号]=14, [减号]=14, [星号]=14, */ [与号]=14, 
    [求大小]=14,
};
static uint8_t 一元后缀优先级表[] = {
    // 15 后缀运算符：[] () . -> (类型名称){列表}  左结合
    [左方括号]=15, [右方括号]=15, [左圆括号]=15, [右圆括号]=15, [实心句号]=15, [单线右单箭头]=15,
};
/******************************************************************************
 * 语法分析器：每次从“词法分析器”获取一个“单词”，解析构建 语法树 和 抽象语法树
 * 
 *****************************************************************************/
FILE * 输入文件_语法分析;
单词信息体 当前单词;
// 单词信息体 下一单词; LL(2)文法改造使用

// 过滤 无效单词，更新 “当前单词”信息体
单词信息体 读入下一单词() {
    单词信息体 新单词信息体;
    // 掠过无用的 注释、空单词、续行符
    do{
        新单词信息体 = 文件_扫描单词(输入文件_语法分析);
    }while(新单词信息体.类型==注释 || 新单词信息体.类型==空单词 || 新单词信息体.类型==续行符);
    当前单词 = 新单词信息体;
    return 新单词信息体;
}
// 错误处理：当遇到错误时，跳过部分语句的解析，而不是全部停止

/******************************************************************************
 *                                 文件解析
 *****************************************************************************/
//《文件》： 《包声明行》 "\n"* 《导入声明》 "\n"* 《声明区》 "\n"*
void 文件_解析() {
#ifdef 解析器调试
    打印文法符号("《文件》");
#endif
    // 读入第一个单词
    读入下一单词();
    包声明行_解析(); //
    if (当前单词.类型==换行) { //略过空行
        do{
            读入下一单词();
        }while(当前单词.类型==换行);
    }
    导入声明_解析();
    if (当前单词.类型==换行) { //略过空行
        do{
            读入下一单词();
        }while(当前单词.类型==换行);
    }
    声明区_解析();
    if (当前单词.类型==换行) { //略过空行
        do{
            读入下一单词();
        }while(当前单词.类型==换行);
    }
    if (当前单词.类型==文件结束) {
        fprintf(stdout, "文件结束！\n");
    } 
#ifdef 解析器调试
    文法等级--;
#endif
}
/*《包声明行》： package保留词 标识符 \n */
void 包声明行_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《包声明行》");
#endif
    if (当前单词.类型 == 包) {
        // 动作
        读入下一单词();
        if (当前单词.类型==标识符) {
            // 动作
            读入下一单词();
        } else {
            // 报错
        }
        if (当前单词.类型==换行) {
            读入下一单词();
        }else {
            // 报错
        }
    } else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
/*《导入声明》： import保留词 《导入声明尾》 */
void 导入声明_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《导入声明》");
#endif
    if (当前单词.类型 == 导入) {
        读入下一单词();
        // 动作
        导入声明尾_解析();
    } else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
/*《导入声明尾》：《导入描述行》 | "(" \n 《导入描述组》 ")" \n  //别名可选，行任意重复 */
void 导入声明尾_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《导入声明尾》");
#endif
    //多行 导入声明
    if(当前单词.类型==标识符 || 当前单词.类型==字符串字面量) { //《导入描述行》首符集
        // 动作
        导入描述行_解析();
    }else if (当前单词.类型 == 左圆括号) { // "("
        // 动作
        读入下一单词();
        if (当前单词.类型 == 换行) { // \n
            读入下一单词();
        } else {/* 报错 */}
        导入描述组_解析();      // 《导入描述组》
        if (当前单词.类型 == 右圆括号) { // )
            读入下一单词();
        } else {/* 报错 */}
        if (当前单词.类型 == 换行) { // \n
            读入下一单词();
        } else {/* 报错 */}
    }else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《导入描述组》：《导入描述行》《导入描述组》|\n 《导入描述组》| ε
void 导入描述组_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《导入描述组》");
#endif
    if(当前单词.类型==标识符 || 当前单词.类型==字符串字面量) { //《导入描述行》首符集
        导入描述行_解析();
        导入描述组_解析();
    }else if(当前单词.类型==换行){
        // 空行
        读入下一单词();
        导入描述组_解析();
    }else if(当前单词.类型==右圆括号){ //《导入描述组》FOLLOW集 {")"}
        // 动作
    }else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《导入描述行》：标识符:别名 字符串:路径 \n | 字符串:路径 \n
void 导入描述行_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《导入描述行》");
#endif
    if (当前单词.类型==标识符) { // 标识符:别名 字符串:路径 \n
        // 动作
        读入下一单词();
        if (当前单词.类型==字符串字面量){
            读入下一单词();
        }else{/*报错*/}
        if (当前单词.类型==换行){
            读入下一单词();
        }else {/*报错*/}
    }else if (当前单词.类型==字符串字面量) { // 字符串:路径 \n
        // 动作
        读入下一单词();
        if (当前单词.类型==换行){
            读入下一单词();
        }else {/*报错*/}
    }else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}

// 《声明区》：《访问修饰声明》《声明区》 | \n 《声明区》| ε
void 声明区_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《声明区》");
#endif
    switch (当前单词.类型){ //《访问修饰声明》首符集: + - macro type const var proc class func oper
    case 加号: //访问控制 public
    case 减号: //访问控制 private
    case 宏: case 类型: case 常量: case 变量:
    case 过程: case 类: case 函数: case 算符:
        访问修饰声明_解析();
        声明区_解析();
        break;
    case 换行:
        读入下一单词();
        声明区_解析();
        break;
    case 文件结束:
        break;
    default:
        // 报错
        break;
    }
#ifdef 解析器调试
    文法等级--;
#endif
}

/*《访问修饰声明》：《声明》 | "+" 《声明》 | "-" 《声明》| 空 \n */
void 访问修饰声明_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《访问修饰声明》");
#endif
    switch (当前单词.类型){
    case 加号: //访问控制 public
        // 动作
        读入下一单词(); 
        声明_解析();
        break;
    case 减号: //访问控制 private
        // 动作
        读入下一单词(); 
        声明_解析();
        break;
    // 《声明》首符集: const var type proc class func
    case 宏: case 类型: case 常量: case 变量:
    case 过程: case 类: case 函数: case 算符: 
        声明_解析();
        break;
    default:
        // 报错
        break;
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《默认声明》：《宏声明》|《类型声明》|《常量声明》|《变量声明》|《过程声明》|《类声明》|《函数声明》|《算符声明》
void 声明_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《声明》");
#endif
    switch (当前单词.类型) {
    // 《默认声明》 首符集
    // case 宏:   宏声明_解析();   break;
    case 类型: 类型声明_解析(); break;
    case 常量: 常量声明_解析(); break;
    case 变量: 变量声明_解析(); break;
    case 过程: 过程声明_解析(); break;
    // case 类:    类声明_解析(); break;
    // case 函数: 函数声明_解析(); break;
    // case 算符: 算符声明_解析(); break;
    default:
        // 报错
        break;
    }
#ifdef 解析器调试
    文法等级--;
#endif
}

/******************************************************************************
 *                                 类型声明
 *****************************************************************************/
//《类型声明》：type保留词 《类型声明尾》
void 类型声明_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《类型声明》");
#endif
    if (当前单词.类型==类型) {
        // 动作
        读入下一单词();
        类型声明尾_解析();
    } else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《类型声明尾》： 《类型描述行》 | "("  \n  《类型描述组》  ")"  \n
void 类型声明尾_解析() {
#ifdef 解析器调试
    文法等级++;
    打印文法符号("《类型声明尾》");
#endif
    if (当前单词.类型==标识符) {
        // 动作
        类型描述行_解析();
    }else if(当前单词.类型==左圆括号) {
        // 动作
        读入下一单词();
        if (当前单词.类型==换行) {
            读入下一单词();
        }else {/* 报错 */}
        类型描述组_解析();
        if (当前单词.类型==右圆括号) { 读入下一单词(); }
            else {/* 报错 */}
        if (当前单词.类型==换行) { 读入下一单词(); }
            else {/* 报错 */}
    }else{
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《类型描述组》： 《类型描述行》《类型描述组》 | \n 《类型描述组》 | ε
void 类型描述组_解析() {
    if (当前单词.类型==标识符) {
        // 动作
        类型描述行_解析();
        类型描述组_解析();
    }else if(当前单词.类型==换行) { // 空行
        读入下一单词();
        类型描述组_解析();
    }else if(当前单词.类型==右圆括号) { //《类型描述组》 FOLLOW集{")"}
        return;
    }else {
        // 报错
    }
}
//《类型描述行》： 标识符:类型名 《类型描述》 \n
void 类型描述行_解析() {
#ifdef 解析器调试
    文法等级++;  打印文法符号("《类型描述行》");
#endif
    if (当前单词.类型==标识符) {
        // 动作
        读入下一单词();
        类型描述_解析();
        if (当前单词.类型==换行) {
            读入下一单词();
        }else{/* 报错 */}
    }else {
        // 报错
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
//《类型描述》： 《基本类型名》|《构造类型描述》
void 类型描述_解析() {
    switch(当前单词.类型) {
    case 数据型: case 逻辑型:
    case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
    case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
    case 浮点: case 浮点32: case 浮点64:
    case 字符: case 字符8: case 字符16: case 字符32: case ascii字符:
        基本类型名_解析();
        break;
    case 单线右单箭头:
    case 左方括号:
    case 结构体: case 共用体: case 枚举体:
        构造类型描述_解析();
        break;
    default:
        // 报错
        break;
    }
}
/*《基本类型名》: 逻辑型 数据型|自然型|自然型8|自然型16|自然型32|自然型64
                |整型|整型8|整型16|整型32|整型64
				| 浮点 | 浮点32 | 浮点64 
				| 字符 | 字符8 | 字符16 | 字符32 | ascii字符  */ 
void 基本类型名_解析() {
#ifdef 解析器调试
    文法等级++;  打印文法符号("《基本类型名》");
#endif
    switch (当前单词.类型){
    case 数据型: case 逻辑型:
    case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
    case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
    case 浮点: case 浮点32: case 浮点64:
    case 字符: case 字符8: case 字符16: case 字符32:
    case ascii字符:
        // 动作
        读入下一单词();
        break;
    default:
        // 报错
        break;
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
// 《构造类型描述》： 《地址类型描述》|《数组类型描述》|《结构体描述》|《共用体描述》|《枚举体描述》
void 构造类型描述_解析() {
#ifdef 解析器调试
    文法等级++;  打印文法符号("《构造类型描述》");
#endif
    switch (当前单词.类型) {
    case 单线右单箭头:
        // 动作
        地址类型描述_解析();
        break;
    case 左方括号:
        // 动作
        数组类型描述_解析();
        break;
    case 结构体:
        // 动作
        结构体描述_解析();
        break;
    case 共用体:
        // 动作
        共用体描述_解析();
    case 枚举体:
        // 动作
        枚举体描述_解析();
        break;
    default:
        // 报错
        break;
    }
#ifdef 解析器调试
    文法等级--;
#endif
}
// 《地址类型描述》： "->" 《类型描述》
void 地址类型描述_解析() {
    if (当前单词.类型==单线右单箭头) {
        // 动作
        读入下一单词();
        类型描述_解析();
    }else {
        // 报错
    }
}
// 《数组类型描述》： "["  《常数表达式》:容量  "]"  《类型描述》
void 数组类型描述_解析() {
    if (当前单词.类型==左方括号) {
        // 动作
        读入下一单词();
        常数表达式_解析();
        if (当前单词.类型==右方括号) {
            // 动作
            读入下一单词();
            类型描述_解析();
        }else{
            // 报错 右方括号缺失
        }
    }else {
        // 报错
    }
}
// 《结构体描述》： struct保留词  "{" \n 《字段组》  "}"  \n
void 结构体描述_解析() {
    if (当前单词.类型==结构体) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==左花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
    字段组_解析();
    if (当前单词.类型==右花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
}
//《字段组》： 《访问修饰字段行》 《字段组》 | \n 《字段组》| ε
void 字段组_解析() {
    // 《访问修饰字段行》 FIRST首符集{"+"、"-"、标识符}
    if(当前单词.类型==标识符 || 当前单词.类型==加号 || 当前单词.类型==减号) {
        // 动作
        访问修饰字段行_解析();
    }else if(当前单词.类型==换行) {
        // 空行
        读入下一单词();
    }else if (当前单词.类型==右花括号) { //《字段组》 FOLLOW随符集{"}"}
        // 动作
        读入下一单词();
    }else {
        // 报错
    }
}
// 《访问修饰字段行》： 《字段行》 | "+" 《字段行》 | "-" 《字段行》
void 访问修饰字段行_解析() {
    if (当前单词.类型==标识符) {
        字段行_解析();
    }else if(当前单词.类型==加号 || 当前单词.类型==减号) {
        // 动作
        读入下一单词();
        字段行_解析();
    }else {
        // 报错
    }
}
// 《字段行》： 标识符:字段名 《类型描述》 \n ;字段行暂不支持逗号","分隔
void 字段行_解析() {
    if (当前单词.类型==标识符) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    类型描述_解析();
    if (当前单词.类型==换行) {
        读入下一单词();
    }else {/* 报错 */}
}
// 《共用体描述》： union保留词   "{" \n 《字段组》  "}"  \n
void 共用体描述_解析() {
    if (当前单词.类型==共用体) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==左花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
    字段组_解析();
    if (当前单词.类型==右花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
}
// 《枚举体描述》： enum保留词  "{" \n 《枚举成员组》 "}"  "\n"
void 枚举体描述_解析() {
    if(当前单词.类型==枚举体) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==左花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
    枚举成员组_解析();
    if (当前单词.类型==右花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        读入下一单词();
    }else{/* 报错 */}
}
// 《枚举成员组》： 《枚举成员项》 (","|"、"|"\n") 《枚举成员组》| \n 《枚举成员组》| ε
void 枚举成员组_解析() {
    if(当前单词.类型==标识符) {
        // 动作
        枚举成员项_解析();
        if(当前单词.类型==逗号 || 当前单词.类型==冒号 || 当前单词.类型==换行) {
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        枚举成员组_解析();
    }else if(当前单词.类型==换行) {
        读入下一单词();
    }else if(当前单词.类型==右花括号) { // 《枚举成员列表》 FOLLOW随符集{"}"}
        // 动作
        读入下一单词();
    }else{  
        // 报错
    }
}
// 《枚举成员项》： 标识符:成员名 (赋值号 常数:枚举值)? 
void 枚举成员项_解析() {
    if(当前单词.类型==标识符) {
        // 动作
        读入下一单词();
        if(当前单词.类型==等于号) {  // 成员名=3
            // 动作
            读入下一单词();
            if(当前单词.类型==整数字面量) {
                // 动作
                读入下一单词();
            }else {/* 报错 */}
        }else {
            // 报错
        }
    }else{
        // 报错
    }
}


/******************************************************************************
 *                                 标识符列表
 *****************************************************************************/
// 《标识符列表》： 标识符 《标识符列表尾》
void 标识符列表_解析() {
    // 首标识符
    if(当前单词.类型==标识符) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    // 剩余标识符
    if (当前单词.类型==逗号 || 当前单词.类型==顿号) { //《标识符列表尾》 FIRST首符集{"," "、"}
        // 动作
        标识符列表尾_解析();
    }else { // 《标识符列表》 FOLLOW随符集{}
        switch(当前单词.类型) { //
        case 数据型: case 逻辑型:
        case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
        case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
        case 浮点: case 浮点32: case 浮点64:
        case 字符: case 字符8: case 字符16: case 字符32:
        case ascii字符:
        case 单线右单箭头:
        case 左方括号:
        case 结构体:
        case 共用体:
        case 枚举体:
            // 动作
            break;
        case 等于号: case 冒等于:
            // 动作
            break;
        default:
            // 报错
            return;
        }
    }
}
// 《标识符列表尾》： (","|"、") 标识符 《标识符列表尾》  |  ε
void 标识符列表尾_解析() {
    if(当前单词.类型==逗号 || 当前单词.类型==顿号) {
        // 动作
        读入下一单词();
        if(当前单词.类型==标识符) {
            // 动作
            读入下一单词();
        } else {
            // 报错
        }
        标识符列表尾_解析();
    }else { // 《标识符列表尾》 FOLLOW随符集{}
        switch(当前单词.类型) {
        case 数据型: case 逻辑型:
        case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
        case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
        case 浮点: case 浮点32: case 浮点64:
        case 字符: case 字符8: case 字符16: case 字符32: case ascii字符:
        case 单线右单箭头:
        case 左方括号:
        case 结构体: case 共用体: case 枚举体: // 完整声明格式，后随《类型描述》
            // 动作
            break;
        case 等于号: case 冒等于: // 短声明格式，直接 赋值
            // 动作
            break;
        default:
            // 报错
            return;
        }
    }
}


/******************************************************************************
 *                                 表达式列表
 *****************************************************************************/
// 《表达式列表》： 《表达式》 《表达式列表尾》
void 表达式列表_解析() {
    表达式_解析();
    表达式列表尾_解析();
}
// 《表达式列表尾》： (","|"、") 《表达式》 《表达式列表尾》 | ε  
void 表达式列表尾_解析() {
    if(当前单词.类型==逗号 || 当前单词.类型==顿号)  {
        // 动作
        读入下一单词();
        表达式_解析();
        表达式列表尾_解析();
    }else { // 《表达式列表尾》 FOLLOW随符集: \n ; 赋值号
        switch(当前单词.类型) { 
        case 换行: case 分号:
        case 冒等于: case 加等于: case 减等于: case 星等于:
        case 点乘等于: case 叉乘等于: case 除等于: case 斜杠等于: case 百分等于:
        case 同或等于:  case 异或等于: case 杨抑等于:
        case 双小于等于: case 双大于等于:
            break;
        default:
            // 报错
            return;
        }
    }
}


/******************************************************************************
 *                                 常数表达式列表
 *****************************************************************************/
// 《常量表达式列表》: 《常数表达式》《常数表达式列表尾》
void 常数表达式列表_解析() {
    常数表达式_解析();
    常数表达式列表尾_解析();
}
// 《常数表达式列表尾》：(","|"、") 《常数表达式》 《常数表达式列表尾》 | ε
void 常数表达式列表尾_解析() {
    if(当前单词.类型==逗号 || 当前单词.类型==顿号)  {
        // 动作
        读入下一单词();
        常数表达式_解析();
        常数表达式列表尾_解析();
    }else if (当前单词.类型==换行 || 当前单词.类型==分号){ //《常数表达式列表尾》FOLLOW随符集: : \n
        // 空字，结束
        return;
    } else {
        // 报错
    }
}
// 《常量表达式》：《表达式》
void 常数表达式_解析() {
    表达式_解析();
}

/******************************************************************************
 *                                 常量声明
 *****************************************************************************/
// 《常量声明》： const保留词 《常量声明尾》
void 常量声明_解析() {
    if (当前单词.类型==常量) {
        // 动作
        读入下一单词();
        常量声明尾_解析();
    } else {
        // 报错
    }
}
// 《常量声明尾》：《常量描述行》 | "("  \n  《常量描述组》  ")"  \n
void 常量声明尾_解析() {
    if (当前单词.类型==标识符) {
        // 动作
        常量描述行_解析();
    }else if(当前单词.类型==左圆括号){
        // 动作
        读入下一单词();
        if (当前单词.类型==换行){ //换行没有动作
            读入下一单词();
        }else{/* 报错 */}
        常量描述组_解析();
        if (当前单词.类型==右圆括号){
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        if (当前单词.类型==换行) { //换行没有动作
            读入下一单词();
        }else {/* 报错 */}
    }else{
        // 报错
    }
}
// 《常量描述组》： 《常量描述行》 《常量描述组》 | \n 《常量描述组》| ε
void 常量描述组_解析() {
    if(当前单词.类型==标识符) { //《常量描述组》：《常量描述行》《常量描述组》
        常量描述行_解析();
        常量描述组_解析();
    }else if(当前单词.类型==换行){ //《常量描述组》：\n 《常量描述组》
        // 空行
        读入下一单词();
        常量描述组_解析();
    }else if(当前单词.类型==右圆括号) { //《常量描述组》: ε   FOLLOW集: ")"
        // 动作
        return;
    }else{
        // 报错
    }
}
// 《常量描述行》： 《标识符列表》 (《类型描述》? 赋值号 《常数表达式列表》)? (";"|"\n")
void 常量描述行_解析() {
    if(当前单词.类型==标识符) {
        // 动作
        标识符列表_解析();
    }else {
        // 报错
    }
    switch(当前单词.类型) { // 《类型描述》?
        case 数据型: case 逻辑型:
        case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
        case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
        case 浮点: case 浮点32: case 浮点64:
        case 字符: case 字符8: case 字符16: case 字符32: case ascii字符:
        case 单线右单箭头:
        case 左方括号:
        case 结构体: case 共用体: case 枚举体:
        //《常量描述行》： 《标识符列表》 《类型描述》赋值号《常数表达式列表》 (";"|"\n")
            // 动作
            类型描述_解析(); //有 类型描述
        case 等于号: case 冒等于: //《常量描述行》：《标识符列表》赋值号《常数表达式列表》 (";"|"\n")
            // 动作
            读入下一单词();
            常数表达式列表_解析();
        case 分号: case 换行: //《常量描述行》：《标识符列表》 (";"|"\n")
            读入下一单词();
            break;
        default:
            // 报错
            return;
    }
    if(当前单词.类型==换行 || 当前单词.类型==分号) {
        // 结束
        读入下一单词();
    }else {
        // 报错
    }
}

/******************************************************************************
 *                                 变量声明
 *****************************************************************************/
// 《变量声明》： var保留词 《变量声明尾》
void 变量声明_解析() {
        if (当前单词.类型==变量) {
        // 动作
        读入下一单词();
        变量声明尾_解析();
    } else {
        // 报错
    }
}
// 《变量声明尾》： 《变量描述行》  |  "(" \n 《变量描述组》 ")" \n
void 变量声明尾_解析() {
    if(当前单词.类型==标识符) { // 《变量描述行》 FIRST首符集{ 标识符 }
        // 动作
        变量描述行_解析();
    }else if(当前单词.类型==左圆括号) {
        // 动作
        读入下一单词();
        if(当前单词.类型==换行) {
            // 动作
            读入下一单词();
        }else{/* 报错 */}
        变量描述组_解析();
        if(当前单词.类型==右圆括号) {
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        if(当前单词.类型==换行) {
            // 动作
            读入下一单词();
        }else {/* 报错 */}
    }else{
        // 报错
    }
}
// 《变量描述组》： 《变量描述行》 《变量描述组》  |  \n 《变量描述组》  |  ε
void 变量描述组_解析() {
    if(当前单词.类型==标识符) { //《变量描述组》：《变量描述行》《变量描述组》
        变量描述行_解析();
        变量描述组_解析();
    }else if(当前单词.类型==换行){ //《变量描述组》：\n 《变量描述组》
        读入下一单词();
        变量描述组_解析();
    }else if(当前单词.类型==右圆括号) { //《变量描述组》: ε  FOLLOW随符集: ")"
        // 动作
        return;
    }else{
        // 报错
    }
}
// 《变量描述行》:《标识符列表》 (《类型描述》(赋值号《表达式列表》)? | 赋值号《表达式列表》) (";"|"\n")
void 变量描述行_解析() {
    if(当前单词.类型==标识符) {
        // 动作
        标识符列表_解析();
    }else {
        // 报错
    }
    switch(当前单词.类型) { // 《类型描述》?
        case 数据型: case 逻辑型:
        case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
        case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
        case 浮点: case 浮点32: case 浮点64:
        case 字符: case 字符8: case 字符16: case 字符32: case ascii字符:
        case 单线右单箭头:
        case 左方括号:
        case 结构体: case 共用体: case 枚举体:
            // 动作
            //《变量描述行》：《标识符列表》《类型描述》(赋值号 《表达式列表》)? (";"|"\n")
            类型描述_解析(); 
            //《变量描述行》：《标识符列表》《类型描述》 赋值号《表达式列表》) (";"|"\n")
            if (当前单词.类型==等于号||当前单词.类型==冒等于) { 
                // 动作
                读入下一单词();
                表达式列表_解析();
                if(当前单词.类型==换行||当前单词.类型==分号) {
                    读入下一单词();
                }else {
                    // 报错
                }
            }//《变量描述行》：《标识符列表》《类型描述》(";"|"\n")
            else if(当前单词.类型==换行||当前单词.类型==分号) {
                读入下一单词();
            }else {
                // 报错
            }
            break;
        case 等于号: case 冒等于: //FOLLOW集，没有 《类型描述》
            读入下一单词();
            表达式列表_解析();
        case 分号: case 换行: //《变量描述行》： 《标识符列表》 《类型描述》? (赋值号 《表达式列表》)? (";"|"\n")
            读入下一单词();
            break;
        default:
            // 报错
            return;
    }
    if(当前单词.类型==换行 || 当前单词.类型==冒号) {
        // 结束
        读入下一单词();
    }else {
        // 报错
    }
}

/******************************************************************************
 *                                 过程、方法、函数 声明
 *****************************************************************************/
// 《参数列表》： 标识符:命名? 《类型描述》 《参数列表尾》
void 参数列表_解析() {
    if (当前单词.类型==标识符) { // 有 标识符
        // 动作
        读入下一单词();
    }else {  // 无 标识符
        switch(当前单词.类型) { // 《类型描述》?
        case 数据型: case 逻辑型:
        case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
        case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
        case 浮点: case 浮点32: case 浮点64:
        case 字符: case 字符8: case 字符16: case 字符32:
        case ascii字符:
        case 单线右单箭头:
        case 左方括号:
        case 结构体:
        case 共用体:
        case 枚举体:
            break;    //《类型描述》 FIRST首符集，继续
        default:
            // 报错
            return;
        }
    }
    类型描述_解析();
    if (当前单词.类型==逗号 || 当前单词.类型==顿号) {
        // 动作
        参数列表尾_解析();
    }else if(当前单词.类型==右圆括号) { //FOLLOW随符集：右圆括号 )
        // 结束
        return;
    }else {
        // 报错
    }
}
// 《参数列表尾》：(","|"、") 标识符:命名? 《类型描述》 《参数列表尾》 | ε  
void 参数列表尾_解析() {
    if (当前单词.类型==逗号) {
        // 动作
        读入下一单词();
    }else if(当前单词.类型==右圆括号) { //FOLLOW随符集：右圆括号
        // 结束
        return;
    }else {
        // 报错
    }
}

// 《过程声明》：proc保留词 标识符 "(" 《参数列表》? ")"  ("(" 《参数列表》 ")")? 《语句块》 
void 过程声明_解析() {
    if(当前单词.类型==过程) {
        // 动作
        读入下一单词();
        if(当前单词.类型==标识符) { //过程名
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        if(当前单词.类型==左圆括号) { //输入列表左括号
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        switch(当前单词.类型) { // 《参数列表》 FIRST首符集
            case 标识符: 
            case 数据型: case 逻辑型:
            case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
            case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
            case 浮点: case 浮点32: case 浮点64:
            case 字符: case 字符8: case 字符16: case 字符32:
            case ascii字符:
            case 单线右单箭头:
            case 左方括号:
            case 结构体: case 共用体: case 枚举体:
                // 动作
                参数列表_解析(); //有 类型描述
                break;
            case 右圆括号: break; //空 《参数列表》，正常退出
            default:
                // 报错
                return;
            }
        if(当前单词.类型==右圆括号) { //输入列表右括号
            // 动作
            读入下一单词();
        }else {/* 报错 */}
        // 返回值列表探测 ("(" 《参数列表》 ")")? 《语句块》 
        if(当前单词.类型==左圆括号) { //有 返回值
            // 动作
            读入下一单词();
            switch(当前单词.类型) { // 《参数列表》 FIRST首符集
                case 标识符: 
                case 数据型: case 逻辑型:
                case 自然型: case 自然型8: case 自然型16: case 自然型32: case 自然型64:
                case 整型: case 整型8: case 整型16: case 整型32: case 整型64:
                case 浮点: case 浮点32: case 浮点64:
                case 字符: case 字符8: case 字符16: case 字符32:
                case ascii字符:
                case 单线右单箭头:
                case 左方括号:
                case 结构体: case 共用体: case 枚举体:
                    // 动作
                    参数列表_解析(); //有 类型描述
                    break;
                case 右圆括号: break; //《参数列表》FOLLOW随符集，正常退出
                default:
                    // 报错
                    return;
                }
            if(当前单词.类型==右圆括号) {
                // 动作
                读入下一单词();
            } else {/* 报错 */}
        }else if (当前单词.类型==左花括号){ //没有 返回值，正常
            语句块_解析();
        }else{/*报错*/}
    }else {
        // 错误
    }
}

/******************************************************************************
 *                                   语句
 *****************************************************************************/
// 《语句块》：  《裸语句块》 ("\n" | EOF)
void 语句块_解析() {
    if (当前单词.类型==左花括号) { // 《裸语句块》FIRST首符集
        // 动作
        裸语句块_解析();
    }else {
        // 报错
    }
    if (当前单词.类型==换行||当前单词.类型==分号) {
        读入下一单词();
    }else if (当前单词.类型==文件结束){
    }else {
        // 报错
    }
}
// 《裸语句块》："{" \n 《语句组》 "}"
void 裸语句块_解析() {
    if (当前单词.类型==左花括号) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    if (当前单词.类型==换行) {
        // 动作
        读入下一单词();
    }else {/* 报错 */}
    switch(当前单词.类型) { // 《语句组》?
        case 宏: 
        case 类型: case 常量: case 变量: 
        case 标识符: case 整数字面量:
        case 跳转: case 中断: case 继续: 
        case 如果:
        case 循环: case 当: case 对于:
        case 善后: case 返回:
            语句组_解析();
            break;    //《语句组》 FIRST首符集，继续
        default:
            // 报错
            return;
        }
    if (当前单词.类型==右花括号) {
        // 动作
        读入下一单词();
    }else {
        // 报错
    }
}
// 《语句组》： 《语句行》《语句组》 | \n 《语句组》 | ε    
void 语句组_解析() {
    switch(当前单词.类型) { // 《语句行》 FIRST首符集？
        case 宏: 
        case 类型: case 常量: case 变量: 
        case 标识符: case 整数字面量:
        case 跳转: case 中断: case 继续: 
        case 如果:
        case 循环: case 当: case 对于:
        case 善后: case 返回:  //《语句行》 FIRST首符集，继续
            语句行_解析(); //《语句组》：《语句行》 《语句组》
            语句组_解析();
            break;   
        case 换行: //《语句组》：\n 《语句组》
            // 空行
            读入下一单词();
            语句组_解析();
            break;
        case 右花括号: //《语句组》： ε    《语句组》FOLLOW随符集："}"
            break;  
        default:
            // 报错
            return;
        }

}
// 《语句行》： 《语句》 ("\n"| ";")
void 语句行_解析() {
    switch(当前单词.类型) { // 《语句行》 FIRST首符集？
        case 宏: 
        case 类型: case 常量: case 变量: 
        case 标识符: case 整数字面量:
        case 跳转: case 中断: case 继续: 
        case 如果:
        case 循环: case 当: case 对于:
        case 善后: case 返回:  //《语句行》 FIRST首符集，继续
            语句_解析();
            break;
        default:
            // 报错
            return;
        }
}
/*《语句》：《声明》|《简单语句》|《标签语句》|《goto语句》|《break语句》|《continue语句》 
		|《branch语句》|《if语句》|《while语句》|《loop语句》|《for语句》
		|《defer语句》|《return语句》|《语句块》   */
void 语句_解析() {
    switch(当前单词.类型) { // 《语句行》 FIRST首符集？
        // case 宏:    宏声明_解析(); break;
        case 类型:       类型声明_解析(); break;
        case 常量:       常量声明_解析(); break;
        case 变量:       变量声明_解析(); break;
        case 标识符:     简单语句_解析(); break; //以标识符、算符开头的语句
        case 整数字面量: 表达式_解析();   break;
        // case 跳转:       跳转语句_解析(); break; //流程控制
        case 中断:       中断语句_解析(); break; 
        case 继续:       继续语句_解析(); break;
        case 分支:       分支语句_解析(); break; //分支 branch if
        // case 如果:       如果语句_解析(); break;
        // case 循环:       循环语句_解析(); break; //循环 loop for while 
        // case 当:         当循环_解析();   break;
        // case 对于:       对于循环_解析(); break;
        // case 善后:       善后语句_解析(); break; //过程 defer return
        // case 返回:       返回语句_解析(); break;   
        default:
            // 报错
            return;
        }
}
// 《简单语句》： 《赋值语句》 | 《表达式》  //以标识符开头的语句，如过程、方法 调用之类
void 简单语句_解析() {

}

/******************************************************************************
 *                                 桩语句
 *****************************************************************************/
// 
void 中断语句_解析() {

}
// 
void 继续语句_解析() {

}
// 
void 分支语句_解析() {

}


//############################### 表达式 LL(1)分析 ##################################

// 通用 《基本表达式》 节点
基本表达式节点 * 解析器_创建基本表达式节点(单词信息体 当前单词) {
    基本表达式节点 * 节点指针 = malloc(sizeof(*节点指针));
    节点指针->开始位置 = 当前单词.位置;
    节点指针->结束位置.行 = 当前单词.位置.行; 
    节点指针->结束位置.列 = 当前单词.位置.列 + 当前单词.单词切片.长度;
    switch (当前单词.类型) {
        case 标识符:
            节点指针->类型 = 标识符_节点;
            节点指针->值.标识符 = 当前单词.字面值.标识符;
            break;
        case 逻辑值字面量:
            节点指针->类型 = 逻辑值_节点;
            节点指针->值.逻辑值 = 当前单词.字面值.逻辑值;
            break;
        case 整数字面量:
            节点指针->类型 = 整数_节点;
            节点指针->值.整数 = 当前单词.字面值.整数;
            break;
        case 浮点数字面量:
            节点指针->类型 = 浮点数_节点;
            节点指针->值.浮点数 = 当前单词.字面值.浮点数;
            break;
        case 字符字面量:
            节点指针->类型 = 字符_节点;
            节点指针->值.字符 = 当前单词.字面值.字符;
            break;
        case 字符串字面量:
            节点指针->类型 = 字符串_节点;
            节点指针->值.字符串 = 当前单词.字面值.字符串;
            break;
        default:
            // 报错
            break;
        }
    return 节点指针;
}

// 《后缀表达式节点》
基本表达式节点 * 解析器_创建表达式节点() {

}

// 《表达式》：《数字值表达式》|《逻辑值表达式》|《集合值表达式》
表达式节点 * 表达式_解析() {
#ifdef 解析器调试
    文法等级++;  打印文法符号("《表达式》");
#endif
    // 赋值表达式_解析();
#ifdef 解析器调试
    文法等级--;
#endif
}


// 《前缀运算符》:= ++ -- & * + - ~ !
// 《一元表达式》:= 《前缀运算符》《一元表达式》|《后缀表达式》|"("《类型描述》")"《一元表达式》
//				  |"sizeof" 《一元表达式》 | "sizeof" "(" 《类型描述》 ")"
表达式节点 * 一元表达式_解析() {
    表达式节点 * 节点指针;
    switch (当前单词.类型) {
    // case 标识符: 后缀表达式_解析(); break; // 《后缀表达式》
    case 双加号: case 双减号: 
    case 与号: case 星号: 
    case 加号: case 减号:
    case 波浪号: case 感叹号: // 《前缀运算符》 《一元表达式》
        读入下一单词();
        一元表达式_解析();
        break;
    case 左圆括号:  // "("《类型描述》")"《一元表达式》
        读入下一单词();
        类型描述_解析();
        if (当前单词.类型==右圆括号) {
            读入下一单词();
            一元表达式_解析();
        }else {
            // 报错
        }
    case 求大小: // "sizeof" 《一元表达式》 | "sizeof" "(" 《类型描述》 ")"
        读入下一单词();
        if (当前单词.类型==左圆括号) { // "sizeof" "(" 《类型描述》 ")"
            读入下一单词();
            类型描述_解析();
            if (当前单词.类型==右圆括号) {
                读入下一单词();
            }else {
                // 报错
            }
        } else { // "sizeof" 《一元表达式》
            一元表达式_解析();
        }
        break;
    default:
        // 报错
        break;
    }
}

// 《后缀表达式》:= 《基本表达式》 (《后缀运算符》)*
表达式节点 * 后缀表达式_解析() {
    基本表达式节点 * 节点指针;
    // 《基本表达式》
    switch (当前单词.类型){
        case 标识符:
        case 整数字面量: case 浮点数字面量:
        case 字符字面量: case 字符串字面量:
            节点指针 = 基本表达式_解析();
            break;
        default:
            break;
        }
    // (《后缀运算符》)*
    while (当前单词.类型==左方括号 || 当前单词.类型==左圆括号
            || 当前单词.类型==实心句号 || 当前单词.类型==单线右单箭头
            || 当前单词.类型==双加号 || 当前单词.类型==双减号 ) 
    {
        // 节点指针 = 后缀运算符_解析(节点指针);
    }
}

/* 《后缀运算符》:= "[" 《表达式》 "]" | "(" (《赋值表达式》 ((","|"、") 《赋值表达式》)* )? ")"
				| "." 标识符 | "->" 标识符 | ++ | --
*/ 
// 表达式节点 * 后缀运算符_解析(基本表达式节点 * 基本表达式节点指针) {

//     switch (当前单词.类型) {
//     case /* constant-expression */:
//         /* code */
//         break;
    
//     default:
//         break;
//     }
// }

// 《基本表达式》:= 标识符 | 整数字面量 | 浮点数字面量 | 字符串字面量 | "(" 《表达式》 ")"
表达式节点 * 基本表达式_解析() {
    // 解析树
    基本表达式节点 * 节点指针;
    switch (当前单词.类型){
        case 标识符:
        case 逻辑值字面量:
        case 整数字面量:
        case 浮点数字面量:
        case 字符字面量:
        case 字符串字面量:
            节点指针 = 解析器_创建基本表达式节点(当前单词);
            break;
        case 左圆括号: // 《基本表达式》:= "(" 《表达式》 ")"
            读入下一单词();
            基本表达式_解析();  //临时调试用
            if (当前单词.类型==右圆括号) {
                读入下一单词();
            }else {
                perror("《基本表达式》 错误！\n");
                // 报错
            }
        default:
            perror("《基本表达式》 错误！\n");
            break;
        }
    读入下一单词();
    return 节点指针;
}

// 中缀美化打印 ( ) + ( )
void 解析器_中缀美化打印(FILE * 输出文件, 表达式节点 * 节点指针) {
    switch (节点指针->类型) {
    //《基本表达式》
    case 标识符_节点: 
        unicode切片_文件打印(((基本表达式节点*)节点指针)->值.标识符, 输出文件); 
        break;
    case 逻辑值_节点:
        fprintf(输出文件,"%s", ((基本表达式节点*)节点指针)->值.逻辑值==true ? "true" : "false");
        break;
    case 整数_节点: 
        fprintf(输出文件, "%d", ((基本表达式节点*)节点指针)->值.整数); 
        break;
    // case 浮点数_节点:
    case 字符_节点:
        fputc('\'', 输出文件);
        文件_写入utf8字符(输出文件, unicode转utf8(((基本表达式节点*)节点指针)->值.字符));
        fputc('\'', 输出文件);
        break;
    case 字符串_节点:
        fputc('"', 输出文件);
        unicode切片_文件打印(((基本表达式节点*)节点指针)->值.字符串, 输出文件); 
        fputc('"', 输出文件);
        break;
    case 加法_节点:
        fprintf(输出文件, "(");
        解析器_中缀美化打印(输出文件, 节点指针->左指针);
        fprintf(输出文件, ")");
        fprintf(输出文件, "+");
        fprintf(输出文件, "(");
        解析器_中缀美化打印(输出文件, 节点指针->右指针);
        fprintf(输出文件, ")");
        break;
    case 乘法_节点:
        fprintf(输出文件, "(");
        解析器_中缀美化打印(输出文件, 节点指针->左指针);
        fprintf(输出文件, ")");
        fprintf(输出文件, "×");
        fprintf(输出文件, "(");
        解析器_中缀美化打印(输出文件, 节点指针->右指针);
        fprintf(输出文件, ")");
        break;
    default:
        break;
    }
}
// 前缀美化打印 +( , )
void 解析器_前缀美化打印(FILE * 输出文件, 表达式节点 * 节点指针) {
    switch (节点指针->类型) {
    // 基本表达式
    case 标识符_节点: 
        unicode切片_文件打印(((基本表达式节点*)节点指针)->值.标识符, 输出文件); 
        break;
    case 逻辑值_节点:
        fprintf(输出文件,"%s", ((基本表达式节点*)节点指针)->值.逻辑值==true ? "true" : "false");
        break;
    case 整数_节点: 
        fprintf(输出文件, "%d", ((基本表达式节点*)节点指针)->值.整数); 
        break;
    // case 浮点数_节点:
    case 字符_节点:
        fputc('\'', 输出文件);
        文件_写入utf8字符(输出文件, unicode转utf8(((基本表达式节点*)节点指针)->值.字符));
        fputc('\'', 输出文件);
        break;
    case 字符串_节点:
        fputc('"', 输出文件);
        unicode切片_文件打印(((基本表达式节点*)节点指针)->值.字符串, 输出文件); 
        fputc('"', 输出文件);
        break;
    // 中间节点
    case 加法_节点:
        fprintf(输出文件, "+(");
        解析器_前缀美化打印(输出文件, 节点指针->左指针);
        fprintf(输出文件, ", ");
        解析器_前缀美化打印(输出文件, 节点指针->右指针);
        fprintf(输出文件, ")");
        break;
    case 乘法_节点:
        fprintf(输出文件, "×(");
        解析器_前缀美化打印(输出文件, 节点指针->左指针);
        fprintf(输出文件, ", ");
        解析器_前缀美化打印(输出文件, 节点指针->右指针);
        fprintf(输出文件, ")");
        break;
    default:
        break;
    }
}

//gcc -g -Wall parser.c lexer.c ../散列表_unicode切片_编码/散列表_unicode切片_编码.c ../unicode切片/unicode切片.c ../utf8_unicode/utf8_unicode.c && ./a.out
int main() {
    解析器调试输出文件 = stdout;
    输入文件_语法分析 = fopen("parser_test.go", "r");
    基本表达式节点 * 节点指针;
    // 部分测试
    读入下一单词();
    节点指针 = 基本表达式_解析();
    // 查看
    解析器_中缀美化打印(解析器调试输出文件, 节点指针);
    // 
    fputc('\n', 解析器调试输出文件);
}
