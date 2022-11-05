#ifndef COMPILER_H
#define COMPILER_H
    #include <stdint.h>
    #include <stdio.h>
    typedef enum 单词类型{ // 大类: 保留词，标识符, 字面量, 界符, 无效词法记号
        文件结束=(-1), 空单词=0, 异常单词, 注释, 标识符, 空标识符, 
        续行符,
        // 字面量： 数字 字符 字符串
        数字字面量, 字符字面量, 字符串字面量, 
        /* ********************************************************************
         *                             运算符 界符
         *********************************************************************/
        // 算符型关键字
        求大小, 求类型, 
        // 特殊单词符号
        双冒号, 冒等于,       // :: :=
        // 界符，结尾有 “号” 的表示 单字符界符，或者是相同单字符的重复
        等于号, 加号, 减号, 星号, 点乘号, 叉乘号, 斜杠, 除号, 百分号, 双加号, 双减号,  //算术运算符 = + - * · × / ÷ % ++ --
        加等于, 减等于, 星等于, 点乘等于, 叉乘等于, 斜杠等于, 除等于, 百分等于,     //自算术运算符 += -= *= ·=  ÷= /= %=
        感叹等于, 双等于, 小于号, 小于等于, 大于号, 大于等于, 不等于号, 小于等于号, 大于等于号, //比较运算符 !=、==、<、<=、>、>=、≠、≤、≥
        感叹号, 逻辑非, 与号, 竖线, 逻辑与, 逻辑或, 异或, 同或, 波浪号, 杨抑号, 双小于, 双大于, //逻辑运算符 ! ￢ ¬ & | ∧ ∨ ⊕ ⊙ ~ ^ << >>
        双与号, 双竖线, 与等于, 竖线等于, 或等于,            //短路逻辑运算符 && || &= |= 
        异或等于, 同或等于, 杨抑等于, 双小于等于, 双大于等于, //自逻辑运算符⊕= ⊙= ^= <<= >>=
        交, 并,                                            //关系运算符 ∩ ∪
        属于, 不属于, 空集, 包含于, 不包含与, 真包含于,       //集合运算符 ∅ Ø ∈∊ ∉、⊂ ⊆、⊄ ⊈、⊊ ⫋、
        逗号, 冒号, 分号, 实心句号, 问号, 顿号, 空心句号, 换行,           //中间界符 , : ; . ? ，：、；？ '\n'
        左圆括号, 右圆括号, 左方括号, 右方括号, 左花括号, 右花括号,    //边缘界符 （）()[]{}
        单引号, 反引号, 双引号, 左单引号, 右单引号, 左双引号, 右双引号,//引号' ` " ‘ ’ “ ” 
        // 方向符号<- <-> -> <=> => <| |> 单字符版本：←:U+2190 ↔:U+2194 →:U+2192 ⇒:U+21D2 ⇔:U+21D4
        单线左单箭头, 单线双向单箭头, 单线右单箭头, 双线双向双箭头, 双线右双箭头, 小于竖线, 竖线大于, 
        // 介词，待用保留词：一些当前不使用，留待未来使用的保留字。为防止误用，这里会识别然后锁定
        介词, 待用保留词,
        /* ********************************************************************
         *                                 声明 
         *********************************************************************/
        包, 模块, 命名域, 链接, 导入, 导出, 接口,
        变量, 常量, // var const
        宏, 过程, 类, 方法, 函数, 算符, // proc func
        // 保留词 控制语句
        跳转, 调用, 善后, 返回, //goto call defer return
        分支, 如果, 则, 否则,  //分支 branch if then else
        循环, 对于, 直到, 当, 中断, 继续, //循环 loop for until while break continue
        // 基本数据类型 类型定义
        逻辑真, 逻辑假, 逻辑型, 数据型,  // true false date bool
        自然型, 自然型8, 自然型16, 自然型32, 自然型64, //无符号整型 uint uint8 uint16 uint32 uint64
        整型, 整型8, 整型16, 整型32, 整型64, //整型 int int8 int16 int32 int64
        浮点, 浮点32, 浮点64,       //浮点型 float float32 float64
        字符, 字符8, 字符16, 字符32, ascii字符, //字符类型 char char8 char16 char32 指定编码类型ascii
        // 构造数据类型
        类型,   // type
        结构体, 共用体, 枚举体, //构造类型 struct union enum
        字符串, // string
    } 单词类型;

    typedef enum 错误码{
        没有错误,
        // 词法分析错误
        数字字面量没有实体数据,
        字符字面量孤立左单引号, 字符字面量为空, 字符字面量不明转义字符, 字符字面量无右单引号,
        字符串字面量无右双引号, 字符串字面量存在非法字符, 字符串字面量不明转义字符,
        块注释未正常结束,
        不明转义符, 
        无效单词,
        // 语法分析错误
    }错误码;

/******************************************************************************
 *                                   词法分析
 * 输入文件 >> posix文件缓冲 >> 编码转换器 >> 字符扫描器 >> 预处理器 >> 词法分析器
 *****************************************************************************/
    #include "lexer.h"

/******************************************************************************
 *                                   语法分析
 * 词法分析 >> 单词信息体 》》  语法分析器  》》 语法分析树 -> 抽象语法树
 * 原理：LL(1)右读向下分析文法 + 表达式算符优先文法
 *****************************************************************************/
    extern FILE * 输入文件_语法分析;
    void   文件_解析();
#endif