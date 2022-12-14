#ifndef COMPILER_H
#define COMPILER_H
    #include <stdint.h>
    #include <stdio.h>
    typedef enum 单词类型{ // 大类: 保留词，标识符, 字面量, 界符, 无效词法记号
        文件结束=(-1), 空单词=0, 异常单词, 注释, 空标识符, 续行符,
        /* ********************************************************************
         *                             运算符 界符
         *********************************************************************/
        // 特殊单词符号
        双冒号, 冒等于,       // :: :=
        // 界符，结尾有 “号” 的表示 单字符界符，或者是相同单字符的重复
        等于号, 加号, 减号, 星号, 中间点, 叉乘号, 斜杠, 除号, 百分号, 取模号, 双加号, 双减号,  //算术运算符 = + - * · × / ÷ % ++ --
        加等于, 减等于, 星等于, 中间点等于, 叉乘等于, 斜杠等于, 除等于, 百分等于,     //自算术运算符 += -= *= ·= ×= /= ÷= %=
        感叹等于, 双等于, 小于号, 小于等于, 大于号, 大于等于, 不等于号, 小于等于号, 大于等于号, //比较运算符 !=、==、<、<=、>、>=、≠、≤、≥
        感叹号, 逻辑非, 与号, 竖线, 逻辑与, 逻辑或, 逻辑异或, 同或, 波浪号, 杨抑号, 双小于, 双大于, //逻辑运算符 ! ￢ ¬ & | ∧ ∨ ⊕ ⊙ ~ ^ << >>
        双与号, 双竖线, 与等于, 逻辑与等于, 竖线等于, 逻辑或等于,            //短路逻辑运算符 && || &= ∧= |= ∨=
        异或等于, 同或等于, 杨抑等于, 双小于等于, 双大于等于, //自逻辑运算符⊕= ⊙= ^= <<= >>=
        交, 并,                                            //关系运算符 ∩ ∪
        属于, 不属于, 空集, 包含于, 不包含与, 真包含于,       //集合运算符 ∅ Ø ∈∊ ∉、⊂ ⊆、⊄ ⊈、⊊ ⫋、
        逗号, 冒号, 分号, 实心句号, 问号, 顿号, 空心句号, 换行,           //中间界符 , : ; . ? ，：、；？ '\n'
        左圆括号, 右圆括号, 左方括号, 右方括号, 左花括号, 右花括号,    //边缘界符 （）()[]{}
        单引号, 反引号, 双引号, 左单引号, 右单引号, 左双引号, 右双引号,//引号' ` " ‘ ’ “ ” 
        // 方向符号<- <-> -> <=> => <| |> 单字符版本：←:U+2190 ↔:U+2194 →:U+2192 ⇒:U+21D2 ⇔:U+21D4
        单线左单箭头, 单线双向单箭头, 单线右单箭头, 双线双向双箭头, 双线右双箭头, 小于竖线, 竖线大于,
        // 关键字型 算符 sizeof typeof 的尺寸 的类型 as is
        求尺寸, 求类型, 的尺寸, 的类型, 当作, 是,
        /* ********************************************************************
         *                             标识符 字面量
         *********************************************************************/
        // 字面量： 数字 字符 字符串
        标识符, 逻辑值字面量, 整数字面量, 浮点数字面量, 字符字面量, 字符串字面量,
        /* ********************************************************************
         *                               待用保留词
         *********************************************************************/
        // 介词，待用保留词：一些当前不使用，留待未来使用的保留字。为防止误用，这里会识别然后锁定
        介词, 待用保留词,
        /* ********************************************************************
         *                                 声明 
         *********************************************************************/
        包, 模块, 命名域, 链接, 导入, 导出, 接口,
        变量, 常量, 设, 值, // var const let val
        宏, 过程, 类, 方法, 函数, 算符, // proc func
        // 保留词 控制语句
        标号, 转到, 中止, 继续, //label goto break continue
        调用, 善后, 返回,       // call defer return
        分支, 匹配, 如果, 则, 否则,  //分支branch 匹配match if then else
        循环, 对于, 当, 直到, //循环 loop for while until
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
        没有错误=0,
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

    typedef enum {
        未知_节点,
        /* ######################### 表达式节点 ######################### */
        // 一元后缀 [] () .· as -> ++ -- 
        下标运算符_节点, 调用运算符_节点, 点运算符_节点, 强制类型转换_节点,
        右箭头运算符_节点, 递增_节点, 递减_节点,
        // 特殊后缀：参数列表
        表达式列表_节点,
        // 一元前缀节点 & * + - ~ ! ￢ ¬  (int)var sizeof() typeof()
        取地址_节点, 解引用_节点, 取正_节点, 取负_节点, 位取反_节点, 非_节点,
        // 一元前后缀混合节点
        取尺寸_节点, 取类型_节点,
        // 二元中缀节点
        加法_节点, 减法_节点, 乘法_节点, 除法_节点, 取模_节点,
        等于_节点, 不等于_节点, 小于_节点, 小于等于_节点, 大于_节点, 大于等于_节点,
        位左移_节点, 位右移_节点,
        短路与_节点, 短路或_节点, 与_节点, 或_节点, 异或_节点, 同或_节点,
        // 三元 条件表达式
        条件表达式_节点,
        // 赋值表达式 := 、+= -= *= ·= ×= /= ÷= %= 、<<= >>= &= ∧= |= ∨= ^= ⊕=
        赋值_节点, 加赋值_节点, 减赋值_节点, 乘赋值_节点, 除赋值_节点, 模赋值_节点,
        位左移赋值_节点, 位右移赋值_节点, 与赋值_节点, 或赋值_节点, 异或赋值_节点,
        // 《基本表达式》节点
        标识符_节点, 空标识符_节点, 逻辑值字面量_节点, 整数字面量_节点, 浮点数字面量_节点,
        字符字面量_节点, 字符串字面量_节点,
        /* ############################# 语句节点 ############################# */
        语句块_节点, 显式调用_节点, 返回_节点, 善后_节点,
        标号_节点, 转到_节点, 中止_节点, 继续_节点,
        如果_节点, 如果否则_节点, 匹配_节点, 模式_节点,
        死循环_节点, 原始循环_节点, 原始循环三元控制块_节点, 当循环_节点, 直到循环_节点, 遍历_节点, 并行遍历,
        /* ############################# 声明节点 ############################# */
        类型声明_节点, 类型描述_节点, 类型描述组_节点,
        // 基本类型
        数据型_节点, 逻辑型_节点,
        自然型_节点, 自然型8_节点, 自然型16_节点, 自然型32_节点, 自然型64_节点,
        整型_节点, 整型8_节点, 整型16_节点, 整型32_节点, 整型64_节点,
        浮点_节点, 浮点32_节点, 浮点64_节点,
        字符_节点, 字符8_节点, 字符16_节点, 字符32_节点, ascii字符_节点,
        // 构造类型
        自定义类型_节点, 过程类型_节点, 函数类型_节点,
        指针类型_节点, 数组类型_节点, 
        结构体类型_节点, 共用体类型_节点, 枚举体类型_节点,
        字段_节点, 枚举成员_节点,
        // 内置数据结构 动态数组、集合、映射等
        标识符列表_节点, 形参列表_节点, 形参声明_节点,
        文件_节点,
        包声明_节点, 导入声明_节点, 导入描述_节点, 导入描述组_节点,
        声明区_节点,
        宏声明_节点,
        常量声明_节点, 常量描述_节点, 常量描述组_节点,
        变量声明_节点, 变量描述_节点, 变量描述组_节点,
        过程声明_节点, 签名_节点, 函数声明_节点,

    } 节点类型;
    
    // 《语句》《表达式》通用三元节点 一元-左指针；二元-左右指针；三元-左中右指针
    typedef struct 三元节点{ // 下标[] 调用() 成员访问. // 解引用-> 递增++ 递减--
        节点类型 类型;
        // unicode切片 所在文件;
        行列位置 开始位置; 行列位置 结束位置;
        uint32_t 字节地址;
        enum {默认, 公开, 私有} 访问修饰;
        struct 三元节点 * 左指针;
        struct 三元节点 * 中指针;
        struct 三元节点 * 右指针;
    } 三元节点;

    // 《声明》 专用节点，前指针仅用于 绑定过程/绑定功能 的绑定形参声明
    typedef struct{ // 过程声明、绑定过程声明、函数声明
        节点类型 类型;
        // unicode切片 所在文件;
        行列位置 开始位置; 行列位置 结束位置;
        uint32_t 字节地址;
        三元节点 * 前指针;
        三元节点 * 左指针;
        三元节点 * 中指针;
        三元节点 * 右指针;
    } 签名节点;

    typedef struct{
        节点类型 类型;
        // unicode切片 所在文件;
        行列位置 开始位置; 行列位置 结束位置;
        uint32_t 字节地址;
        uint16_t 长度;
        uint16_t 容量;
        三元节点 ** 数据区;
    } 列表节点;
    
    // 《基本表达式》 通用节点
    typedef struct {
        节点类型 类型;
        行列位置 开始位置; 行列位置 结束位置;
        uint32_t 字节地址;
        union {
            unicode切片 标识符;
            bool        逻辑值;
            int         整数;
            double      浮点数;
            unicode字符 字符;
            unicode切片 字符串;
        } 值;
    } 基本表达式节点;

    extern 单词信息体 当前单词;
    三元节点 * 文件_解析();
    //测试用，正常不应暴露下列内部函数
    单词信息体 读入下一单词();
    三元节点 * 表达式_解析();
    /******************************************************************************
     *                                   语义分析
     *****************************************************************************/
    // 类型表
    typedef enum {
        空类型, 逻辑类型,
        自然型类型, 自然型类型8, 自然型类型16, 自然型类型32, 自然型类型64,
        整型类型, 整型类型8, 整型类型16, 整型类型32, 整型类型64,
        浮点型类型, 浮点型类型32, 浮点型类型64,
        字符类型, 字符类型8, 字符类型16, 字符类型32, ascii字符类型,
        // 构造类型
        数组类型, 指针类型, 相对指针类型, 字符串类型,
        枚举类型, 结构体类型, 共用体类型,
        // 过程、函数
        过程指针类型,
    } 支持类型;

    typedef struct 类型节点{
        支持类型 类型;
        struct 类型节点 * 下级类型;
        int 尺寸;
        int 对齐字节;
        union {
            struct 类型节点 * 数组元素类型;
            // 类型节点 * 
        }u;
    } 类型节点;
    
    // 符号
    typedef struct {
        unicode切片 名字;
        enum {
            常量类别=1, 标号类别, 全局变量, 参数, 局部变量
        } 类别;
        // unicode切片 所在文件;
        行列位置 位置;
        // 符号 * 上一符号;
        uint8_t 存储类型;
        uint8_t 类型;
        union {
            // 附加信息
        } 附加信息;
    } 符号;
    
    // 符号节点
    typedef struct 符号节点{
        符号              符号;
        struct 符号节点 * 下一节点;
    } 符号节点;
        
    // 符号表 使用《散列表》实现，每个作用域对应一个散列表，下级散列表链接上级散列表
    typedef struct 符号表{
        int              嵌套等级;
        struct 符号表   *上一符号表;
        int              用量;
        int              容量;
        符号节点       ** 散列区;
    } 符号表;
    
    // 类型检查
    typedef struct{
        
    } 表达式类型;
    
#endif