#ifndef LEXER_H
#define LEXER_H
    #include "compiler.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include "../utf8_unicode/utf8_unicode.h"  //utf8与unicode字符的转换库
    #include "../unicode切片/unicode切片.h" // unicode切片函数，用于保存字符串
    // 提供扫描到的'字符'及该字符的'行列位置'
    typedef struct{
        int8_t  错误;     // 用于报告文件结束EOF等错误
        int32_t 行号;     //假定文件行数不超过 2^32 行
        int16_t 列号;     // 一行的列数有限
        unicode字符 字符; // 扫描的字符
    } 字符信息体;

    //序列化示例： {@1, 1:1, "symbol", <单词类型码>, 6} 
    typedef struct{
        错误码      错误;
        uint32_t    计数;
        uint16_t    行内计数;   //续行符 下一行的单词会被视作相同的单词行
        uint32_t    行号;
        uint16_t    列号;       //首字符的列号作为整个单词的列号
        unicode切片 单词切片;    //记录单词，在字符和字符串字面量时记录转换后的值
        int64_t     字面值;     // 当为数字字面量是，记录对应二进制值
        单词类型    类型;
    } 单词信息体;
    // 字符扫描
    uint32_t 单词计数 = 0;
    uint16_t 行内单词计数 = 0;
    uint8_t 上一字符文件字节长度 = 0;  // 保存字符在文件中的字节长度，准备回退一个字符是使用
    // 行号和列号遵循常见'文本编辑器'的计数标准，从1开始计数
    字符信息体 上一字符信息 = { // 保存上一字符信息，以便于根据换行符更新行列号，以及回退
        .错误=EOF, .行号=1, .列号=0, .字符='\0'
    };

    void 文件_回吐一个字符(FILE * 输入文件);
    字符信息体 文件_扫描字符(FILE * 输入文件);
    单词信息体 文件_扫描单词(FILE * 输入文件);
#endif