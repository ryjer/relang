#include "compiler.h"
#include "lexer.h"
#include <stdio.h>

//gcc -g -Wall lexer_test.c lexer.c ../散列表_unicode切片_编码/散列表_unicode切片_编码.c ../unicode切片/unicode切片.c ../utf8_unicode/utf8_unicode.c && ./a.out
// 文件_扫描单词(FILE * 输入文件); 测试
int main() {
    FILE * 输入文件 = fopen("lexer_test.txt", "r");
    FILE * 输出文件;
    输出文件 = stdout;
    单词信息体 单词信息;
    // 读取一个单词
    单词信息 = 文件_扫描单词(输入文件);
    // 文件未结束
    while (单词信息.错误 != EOF) {
        printf("{@%d, %d, #%d, %d:%d, \"", 单词信息.计数, 单词信息.字节地址, 单词信息.行内计数, 单词信息.位置.行, 单词信息.位置.列);
        if (单词信息.单词切片.数据[0] == '\n') {
            fprintf(输出文件, "\\n");
        } else {
            unicode切片_文件打印(单词信息.单词切片, 输出文件);
        }
        printf("\", <%d>, %d} ", 单词信息.类型, 单词信息.单词切片.长度);
        // 计算散列值
        printf("%%%u ", unicode切片_计算散列值(单词信息.单词切片, 127));
        // 条件输出 数字值和错误
        if (单词信息.类型==整数字面量 || 单词信息.类型 == 字符字面量) {printf("%d %#x %#o ", 单词信息.字面值.字符, 单词信息.字面值.字符, 单词信息.字面值.字符);}
        // 检查字符串转义
        if (单词信息.类型==字符串字面量) {
            unicode切片_文件打印(单词信息.字面值.字符串, 输出文件);
        }
        if (单词信息.错误 != 0) {printf("\t错误码:%u ", 单词信息.错误);}
        fputc('\n', 输出文件);
        单词信息 = 文件_扫描单词(输入文件);
    }
    // 文件结束EOF
    printf("{@%d, %d, #%d, %d:%d, \"EOF", 单词信息.计数, 单词信息.字节地址, 单词信息.行内计数, 单词信息.位置.行, 单词信息.位置.列);
    printf("\", <%d>, 0} err:%d\n", 单词信息.类型, 单词信息.错误);
    // fprintf(stdout, "\n");
    return 0;
}

// gcc -g -Wall lexer.c ../utf8_unicode/utf8_unicode.c && ./a.out
// 文件_扫描字符(输入文件); 测试例程
// int main() {
//     FILE * 输入文件 = fopen("test.txt", "r");
//     字符信息体 tmp;
//     printf("行\t 列\t 字符\t 错误\n");
//     tmp=文件_扫描字符(输入文件);
//     // 文件未结束
//     while (tmp.错误 != EOF) {
//         printf("%d\t %d\t ", tmp.行号, tmp.列号);
//         if (tmp.字符 == '\n') {
//             fprintf(stdout, "\\n");
//         } else {
//             文件_写入utf8字符(stdout, unicode转utf8(tmp.字符));
//         }
//         fprintf(stdout, "\t %d", tmp.错误);
//         putchar('\n');
//         tmp=文件_扫描字符(输入文件);
//     }
//     // 文件结束
//     fprintf(stdout, "%d\t %d\t EOF\t %d\n", tmp.列号, tmp.列号, tmp.错误);
//     return 0;
// }

