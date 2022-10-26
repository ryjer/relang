#include "utf8_unicode/utf8_unicode.h"  //utf8与unicode字符的转换库
#include <stdlib.h>
/******************************************************************************
 *                                 词法分析
 * 输入文件 >> posix文件缓冲 >> 编码转换器 >> 字符扫描器 >> 预处理器 >> 词法分析器
 *****************************************************************************/

/**
	字符扫描器：从文件中按顺序读取一个字符返回[文件不读完不要修改fin，否则数据丢失]
	返回值：扫描字符体中的'.错误'独立提供错误信息，当其为 EOF 时表示文件结束
	关联全局变量：上一字符、上一字符行号、上一字符列号、待取字符、待取字符行号、待取字符列号
*/
// 字符信息体: 提供扫描到的字符的同时，也提供该字符的行列位置
typedef struct{
    int8_t  错误;     // 用于报告文件结束EOF等错误
    int32_t 行号;     //假定文件行数不超过 2^32 行
    int16_t 列号;     // 一行的列数有限
    unicode字符 字符; // 扫描的字符
} 字符信息体;

/* 扫描序列信息：保存上一字符信息，从而支持 回退一个字符，但支持回退一次
 *  +--------+--------+--------+
 *  |上一字符 |当前字符 |待取字符|
 *  +--------+--------+--------+
 */
uint8_t 上一字符文件字节长度 = 0;  // 保存字符在文件中的字节长度，准备回退一个字符是使用
// 行号和列号遵循常见'文本编辑器'的计数标准，从1开始计数
字符信息体 上一字符信息 = { // 保存上一字符信息，以便于根据换行符更新行列号，以及回退
    .错误=EOF, .行号=1, .列号=0, .字符='\0'
};
//文件回吐一个字符，根据上一字符长度回调文件字节指针，同时更新上一字符行列信息
void 文件_回吐一个字符(FILE * 输入文件) {
    fseek(输入文件, -(上一字符文件字节长度), SEEK_CUR);
    /* 由于之前已经读入的字符属于 标识符、字面量、界符 的字符都在同一行
     * 因此回退一个字符不会影响导致行号变化，因此不用调整行号
     */
    if (上一字符文件字节长度 != 0) {
        上一字符信息.列号 --;
    }
}

//读取一个utf8字符，将其转换为'unicode编码'后与'行列地址'一同返回
字符信息体 文件_扫描字符(FILE * 输入文件) {
    utf8字符体 utf8字符;
    字符信息体 当前字符 = {.错误=0, .行号=1, .列号=1, .字符='\0'};
    // 尝试读取，检查文件是否结束
    utf8字符 = 文件_读取utf8字符(输入文件);
    if (utf8字符.长度 == 0) { // EOF错误检查
        当前字符.错误 = EOF;
        当前字符.行号 = 上一字符信息.行号;
        当前字符.列号 = 上一字符信息.列号 + 1; // 假定文件结尾有一个EOF字符
        当前字符.字符 = '\0';
        上一字符文件字节长度 = 0;
        return 当前字符;
    } else { 当前字符.错误 = 0; }
    // 文件未结束，填充'当前字符'信息
    当前字符.字符 = utf8转unicode(utf8字符);
    // 更新行列信息
    if (上一字符信息.字符 == '\n') { // 上一字符是换行，重置为新行起始
        当前字符.行号 = 上一字符信息.行号 + 1;
        当前字符.列号 = 1;
    } else { // 之前没有换行，当前行，列+1
        当前字符.行号 = 上一字符信息.行号;
        当前字符.列号 = 上一字符信息.列号 + 1;
    }
    // 当前字符后移一位，上一字符保存当前字符信息
    上一字符信息 = 当前字符;
    上一字符文件字节长度 = utf8字符.长度;
    // 返回
    return 当前字符;
}

/******************************************************************************
 * 分词器：将标识符，关键字，数字，界符分别识别出来
 * 返回值：解析单词符号，返回 单词计数、单词符号、文本行列地址、类型号、列范围 的结构体
 * 关联：全局变量sym,id[],str[],num,letter
 * 翻译：token:单词
 *****************************************************************************/
#include "unicode_slice/unicode_slice.h" // unicode切片函数，用于保存字符串
enum 单词类型 { // 大类: 标识符, 字面量, 界符, 无效词法记号
    文件结束, 空单词, 标识符, 异常单词, 注释,
    数字字面量, 字符字面量, 字符串字面量, // 字面量
    返回, // 控制语句关键字 return
    如果, 否则, // 分支语句关键字 if else
    当, 循环, 跳出, 继续, // 循环语句 while for break continue
    // 基本数据类型
    无类型, 逻辑型, // void bool
    自然型, 自然型8, 自然型16, 自然型32, 自然型64,  // uint无符号整型/自然数型
    整型, 整型8, 整型16, 整型32, 整型64, // int整型
    字符, 字符8, 字符16, 字符32, ascii字符, // char字符类型，指定编码类型
    // 构造数据类型
    结构体, 共用体, 枚举体, // 结构体、共用体、枚举编码
    字符串, // string字符串类型
    // 界符
    赋值, 加, 减, 乘, 除, 取模, 递增, 递减,              //算术运算符 = + - * / % ++ --
    自加, 自减, 自乘, 自除, 自取模,                      //自算术运算符 += -= *= /= %=
    不等于, 等于, 小于, 小于等于, 大于, 大于等于,         //比较运算符 '!=' '==' '<' '<=' '>' '>='
    非, 与, 或, 异或, 同或, 位非, 位异或, 左移, 右移,     //逻辑运算符 ! & | ⊕ ⊙ ~ ^ << >>
    短路与, 短路或,                                     //短路逻辑运算符 && ||
    自与, 自或, 自异或, 自同或, 自位异或, 自左移, 自右移, //自逻辑运算符 &= |= ⊕= ⊙= ^= <<= >>=
    交, 并,                                            //关系运算符 ∩ ∪
    属于, 不属于, 空集, 包含于, 不包含与, 真包含于,       //集合运算符 Ø ∈∊ ∉、⊂ ⊆、⊄ ⊈、⊊ ⫋、
    逗号, 句号, 冒号, 分号,                                     //中间界符 , . : ;  ，。：；
    左圆括号, 右圆括号, 左方括号, 右方括号, 左花括号, 右花括号,    //边缘界符 ()[]{}
    单引号, 反引号, 双引号, 左单引号, 右单引号, 左双引号, 右双引号,//引号' ` " ‘ ’ “ ” 
};

//序列号示例： {@1, 1:6, 'parrt', <标识符>, 6:10}
typedef struct{
    int8_t      错误;
    uint32_t    计数;
    uint32_t    行号;
    uint16_t    列号;       //首字符的列号作为整个单词的列号
    unicode切片 单词切片;   //记录单词，在字符串字面量时记录转换后的值
    int64_t     数字值;     //当为数字字面量是，记录对应二进制值
    uint16_t    类型;
} 单词信息体;

uint32_t 单词计数 = 0;
// 词法分析器主接口，返回一个带有类型标记的 单词信息体
单词信息体 文件_扫描单词(FILE * 输入文件) {
    字符信息体 读入字符;
    单词信息体 单词信息;
    // 跳过空白，直到读到第一个非空白
    do { //读入标识符剩下的字符
        // 尝试读取首字符，同时检查文件结束错误
        读入字符 = 文件_扫描字符(输入文件); //读入下一字符
        if (读入字符.错误 == EOF) { //文件结束，停止读取跳出循环
            单词信息.错误 = EOF;
            单词信息.计数 = 单词计数 + 1;
            单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
            单词信息.类型 = 文件结束;
            return 单词信息;
        }
    }while(是空白(读入字符.字符));
    // 文件未结束，正常判断，初始化结构体
    单词计数++;         // 更新单词计数
    单词信息.错误 = 0;   // 没有错误，设为0
    单词信息.计数 = 单词计数;
    单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
    unicode切片_初始化(&(单词信息.单词切片), 8);
    单词信息.数字值 = 0;
    //标识符
    if (是字母(读入字符.字符)||是汉字(读入字符.字符)||(读入字符.字符=='_')) {
        单词信息.类型 = 标识符;
        do { //读入标识符剩下的字符
            unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
            读入字符 = 文件_扫描字符(输入文件); //读入下一字符
        }while((读入字符.错误!=EOF) && (是字母数字(读入字符.字符)||是汉字(读入字符.字符)||(读入字符.字符=='_')));
        文件_回吐一个字符(输入文件);  //最后回吐一个字符
        // 检查是否是保留字/关键字
    } //数字，不考虑正负号，默认正数，前缀法只支持小写字母
    else if (是数字(读入字符.字符)) {
        单词信息.类型 = 数字字面量;
        // 字面值转换
        if (读入字符.字符 != '0') {                         //10进制数字
            do {
                unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                读入字符 = 文件_扫描字符(输入文件); //读入下一字符
            }while((读入字符.错误!=EOF) && 是10进制数字(读入字符.字符));
            文件_回吐一个字符(输入文件);  //最后回吐一个字符
        } else {                                            // 特殊进制数字，前缀法表示
            unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
            读入字符 = 文件_扫描字符(输入文件); //忽视第一个0，读取进制字母 h x d o b
            if (读入字符.错误 == EOF) { //文件结束，停止读取跳出循环
                单词信息.错误 = EOF;          单词信息.计数 = 单词计数 + 1;
                单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
                单词信息.类型 = 文件结束;
                return 单词信息;
            }
            if (读入字符.字符=='x' || 读入字符.字符=='h') {     //16进制数
                unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                读入字符 = 文件_扫描字符(输入文件); //忽视第一个0，读取进制字母 h x o b
                if (读入字符.错误 == EOF) { //文件结束，停止读取跳出循环
                    单词信息.错误 = EOF;          单词信息.计数 = 单词计数 + 1;
                    单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
                    单词信息.类型 = 文件结束;
                    return 单词信息;
                }
                if (是16进制数字(读入字符.字符)) {
                    do {
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                        读入字符 = 文件_扫描字符(输入文件);
                    } while((读入字符.错误!=EOF) && 是16进制数字(读入字符.字符));
                    文件_回吐一个字符(输入文件);
                } else {
                    // 0x 后无数字错误
                }
            } else if (读入字符.字符 == 'o') {                  //8进制数
                unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                读入字符 = 文件_扫描字符(输入文件); //忽视第一个0，读取进制字母 h x o b
                if (读入字符.错误 == EOF) { //文件结束，停止读取跳出循环
                    单词信息.错误 = EOF;          单词信息.计数 = 单词计数 + 1;
                    单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
                    单词信息.类型 = 文件结束;
                    return 单词信息;
                }
                if ('0'<=读入字符.字符 && 读入字符.字符<='7') {
                    do {
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                        读入字符 = 文件_扫描字符(输入文件);
                    } while((读入字符.错误!=EOF) && '0'<=读入字符.字符 && 读入字符.字符<='8');
                    文件_回吐一个字符(输入文件);
                } else {
                    // 0o 后无数字错误
                }
            } else if (读入字符.字符 == 'b') {                  //2进制数
                unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                读入字符 = 文件_扫描字符(输入文件); //忽视第一个0，读取进制字母 h x o b
                if (读入字符.错误 == EOF) { //文件结束，停止读取跳出循环
                    单词信息.错误 = EOF;          单词信息.计数 = 单词计数 + 1;
                    单词信息.行号 = 读入字符.行号; 单词信息.列号 = 读入字符.列号;
                    单词信息.类型 = 文件结束;
                    return 单词信息;
                }
                if ('0'<=读入字符.字符 && 读入字符.字符<='1') {
                    do {
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                        读入字符 = 文件_扫描字符(输入文件);
                    } while((读入字符.错误!=EOF) && '0'<=读入字符.字符 && 读入字符.字符<='1');
                    文件_回吐一个字符(输入文件);
                } else {
                    // 0b 后无数字错误
                }
            } else {
                // 不明进制字母报错
            }
        }
        // 最后单独进行 字面量 >> 数字值 转换
        单词信息.数字值 = unicode切片_转整数(&(单词信息.单词切片), 0);
    } //界符
    else { //算符出现频率高的放前面：圆括号()、花括号{}、分号;、
        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
        if (读入字符.字符 <= 0x7F) { // ascii 字符
            switch (读入字符.字符) {
            //边缘界符: () [] {}
            case '(': 单词信息.类型 = 左圆括号; break;
            case ')': 单词信息.类型 = 右圆括号; break;
            case '[': 单词信息.类型 = 左方括号; break;
            case ']': 单词信息.类型 = 右方括号; break;
            case '{': 单词信息.类型 = 左花括号; break;
            case '}': 单词信息.类型 = 右花括号; break;
            //中间界符: , . : ;
            case ',': 单词信息.类型 = 逗号; break;
            case '.': 单词信息.类型 = 句号; break;
            case ':': 单词信息.类型 = 冒号; break;
            case ';': 单词信息.类型 = 分号; break;
            //比较运算符 == != < <= > >= 位运算符 << <<= >> >>=
            case '=': 单词信息.类型 = 赋值;     //'='、'=='
                读入字符 = 文件_扫描字符(输入文件);     //判等 == 检查
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 ==
                    单词信息.类型 = 等于;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '!': 单词信息.类型 = 非;   //'!'、'!='
                读入字符 = 文件_扫描字符(输入文件);      //不等于 != 检查
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 !=
                    单词信息.类型 = 不等于;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '<': 单词信息.类型 = 小于;     //'<'、'<='、'<<'、'<<='
                读入字符 = 文件_扫描字符(输入文件);     //预读检查 <= << <<=
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 <=
                    单词信息.类型 = 小于等于;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '<') {     //没结束，是 <<
                    单词信息.类型 = 左移;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    读入字符 = 文件_扫描字符(输入文件); //预读检查   <<=
                    if (读入字符.错误 == EOF) {        //文件结束，停止读取跳出循环
                        break;
                    } else if (读入字符.字符 == '=') { //没结束，是 <<=
                        单词信息.类型 = 自左移;
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    } else { 文件_回吐一个字符(输入文件); }
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '>': 单词信息.类型 = 大于;     //'>'、'>='、'>>'、'>>='
                读入字符 = 文件_扫描字符(输入文件);      //预读检查 >= >> >>=
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 >=
                    单词信息.类型 = 大于等于;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '>') {     //没结束，是 >>
                    单词信息.类型 = 右移;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    读入字符 = 文件_扫描字符(输入文件); //预读检查 >>=
                    if (读入字符.错误 == EOF) {        //文件结束，停止读取跳出循环
                        break;
                    } else if (读入字符.字符 == '=') { //没结束，是 >>=
                        单词信息.类型 = 自右移;
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    } else { 文件_回吐一个字符(输入文件); }
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            //算术运算符+ - * / % += -= *= /= %= ++ -- 注释 // /*
            case '+': 单词信息.类型 = 加;       //'+'、'++'、'+='
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '+') {     //没结束，是 ++
                    单词信息.类型 = 递增;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '=') {     //没结束，是 ++
                    单词信息.类型 = 自加;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '-': 单词信息.类型 = 减;       //'-'、'--'、'-='
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '-') {     //没结束，是 --
                    单词信息.类型 = 递减;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '=') {     //没结束，是 -=
                    单词信息.类型 = 自减;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '*': 单词信息.类型 = 乘;       //'*'、'*='
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 -=
                    单词信息.类型 = 自乘;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '/': 单词信息.类型 = 除;       //'/'、'/=' 行注释'//'、块注释'/*'
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     // /=
                    单词信息.类型 = 自除;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '/') {     // 单行注释 '//'
                    单词信息.类型 = 注释;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    do { //读入标识符剩下的字符，直到换行符或者文件结束
                        读入字符 = 文件_扫描字符(输入文件); //读入下一字符
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    }while(!(读入字符.字符=='\n'||读入字符.错误==EOF));
                    单词信息.单词切片.长度--;       //抹去读入的无意义的'换行符'空白
                } else if (读入字符.字符 == '*') {     //块注释   '/*'
                    单词信息.类型 = 注释;
                    int 块注释嵌套层数 = 1;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                    do {
                        读入字符 = 文件_扫描字符(输入文件); //读入下一字符
                        unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                        if (读入字符.字符 == '/') { // 嵌套一层块注释检查 '/*'
                            读入字符 = 文件_扫描字符(输入文件);
                            unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                            if (读入字符.字符 == '*'){      //新块注释 /*
                                块注释嵌套层数++;
                            } 
                        }else if (读入字符.字符 == '*') {   //跳出一层检查 '*/'
                            读入字符 = 文件_扫描字符(输入文件);
                            unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                            if (读入字符.字符 == '/'){      //新块注释 /*
                                块注释嵌套层数--;
                            }
                        }
                    }while((读入字符.错误!=EOF) && (块注释嵌套层数>0));
                    // 块嵌套退出错误，例如文件结束但是块注释没有结束
                    if (块注释嵌套层数 != 0) {
                        
                    }
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '%': 单词信息.类型 = 取模;     //% %=
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //没结束，是 %=
                    单词信息.类型 = 自取模;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            //位运算符 & | ~ ^ &= |= ~= ^= 逻辑运算符 && ||
            case '&': 单词信息.类型 = 与;     // & && &=
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '&') {     //是 &&
                    单词信息.类型 = 短路与;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '=') {     //是 &=
                    单词信息.类型 = 自与;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '|': 单词信息.类型 = 或;     // | || |=
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '|') {     //是 ||
                    单词信息.类型 = 短路或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else if (读入字符.字符 == '=') {     //是 |=
                    单词信息.类型 = 自或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            case '~': 单词信息.类型 = 位非;     // ~
                break;
            case '^': 单词信息.类型 = 位异或;   // ^ ^=
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {            //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {     //是 ^=
                    单词信息.类型 = 自位异或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); } //不是++，回吐一个字符
                break;
            default: // 其它为单词错误
                ;
                break;
            }
        } else { // 高阶 unicode 字符
            switch (读入字符.字符) { 
            //边缘界符: （）
            case 0xff08: 单词信息.类型 = 左圆括号; break;   //'（': U+FF08
            case 0xff09: 单词信息.类型 = 右圆括号; break;   //'（': U+FF09
            //中间界符: ，：；
            case 0xff0c: 单词信息.类型 = 逗号; break;       //'，': U+FF0C
            case 0xff1a: 单词信息.类型 = 冒号; break;       //'：': U+FF1A
            case 0xff1b: 单词信息.类型 = 分号; break;       //'；': U+FF1B
            //比较运算符 ≠ ≤ ≥
            case 0x2260: 单词信息.类型 = 不等于; break;     //'≠': U+2260
            case 0x2264: 单词信息.类型 = 小于等于; break;   //'≤': U+FF1B
            case 0x2265: 单词信息.类型 = 大于等于; break;   //'≥': U+FF1B
            //算术运算符 × ÷ ×= ÷=
            case 0x00d7: 单词信息.类型 = 乘;                //'×': U+00D7
                读入字符 = 文件_扫描字符(输入文件);         //判等 == 检查
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          //没结束，是 ×=
                    单词信息.类型 = 自乘;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            case 0x00f7: 单词信息.类型 = 除;                //'÷': U+00F7
                读入字符 = 文件_扫描字符(输入文件);         //判等 == 检查
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          //没结束，是 ×=
                    单词信息.类型 = 自除;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            //逻辑运算符 ∧ ∨ ¬ ￢ ∧= ∨= ¬= ￢=
            case 0x2227: 单词信息.类型 = 与;                //'∧': U+2227 与
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          // ∧=
                    单词信息.类型 = 自与;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            case 0x2228: 单词信息.类型 = 或;                //'∨': U+2228 或
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          // ∧=
                    单词信息.类型 = 自或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            case 0x00ac: case 0xffe2:           //'¬': U+00AC '￢':U+FFE2 非标记 全角非标记
                单词信息.类型 = 非;    
                break;
            case 0x2295: 单词信息.类型 = 异或;              //'⊕': U+2295 异或
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          // '⊕='
                    单词信息.类型 = 自异或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            case 0x2299: 单词信息.类型 = 同或;              //'⊙': U+2299 同或
                读入字符 = 文件_扫描字符(输入文件);      
                if (读入字符.错误 == EOF) {                 //文件结束，停止读取跳出循环
                    break;
                } else if (读入字符.字符 == '=') {          // '⊙='
                    单词信息.类型 = 自同或;
                    unicode切片_追加(&(单词信息.单词切片), 读入字符.字符);
                } else { 文件_回吐一个字符(输入文件); }     //不是++，回吐一个字符
                break;
            // 集合运算符/关系运算符: ∩ ∪ ∈∊ ∉ Ø 
            case 0x2229: 单词信息.类型 = 交; break;         //'∩': U+2229
            case 0x222a: 单词信息.类型 = 并; break;         //'∪': U+222A
            case 0x2208: case 0x220a:                       //'∈': U+2208 '∊': U+220A
                单词信息.类型 = 属于;  
                break;
            case 0x2209: 单词信息.类型 = 不属于; break;     //'∉': U+2209
            // 集合运算符：不包含与⊄ ⊈、包含于⊂ ⊆、真包含与⊊ ⫋
            case 0x2284: case 0x2288:                       //'⊄': U+2284 '⊈': U+2288
                单词信息.类型 = 不包含与;
                break;
            case 0x2282: case 0x2286:                       //'⊂': U+2282 '⊆': U+2286
                单词信息.类型 = 包含于;
                break;
            case 0x228a: case 0x2acb:                       //'⊊': U+228A '⫋': U+2ACB
                单词信息.类型 = 真包含于;
                break;
            default:
                break;
            }
        }
    }
    return 单词信息;
}

//gcc -g -Wall lexer.c utf8_unicode/utf8_unicode.c unicode_slice/unicode_slice.c && ./a.out
// 文件_扫描单词(FILE * 输入文件); 测试
int main() {
    FILE * 输入文件 = fopen("test.txt", "r");
    单词信息体 单词信息;
    // 读取一个单词
    单词信息 = 文件_扫描单词(输入文件);
    // 文件未结束
    while (单词信息.错误 != EOF) {
        printf("{@%d, %d:%d, \"", 单词信息.计数, 单词信息.行号, 单词信息.列号);
        if (单词信息.单词切片.数据[0] == '\n') {
            fprintf(stdout, "\\n");
        } else {
            unicode切片_文件打印(&(单词信息.单词切片), stdout);
        }
        printf("\", <%d>, %d} ", 单词信息.类型, 单词信息.单词切片.长度);
        // 条件输出 数字值和错误
        if (单词信息.类型 == 数字字面量) {printf("%ld %#lx %#lo\t ", 单词信息.数字值, 单词信息.数字值, 单词信息.数字值);}
        if (单词信息.错误 != 0) {printf("错误:%ld", 单词信息.数字值);}
        fputc('\n', stdout);
        单词信息 = 文件_扫描单词(输入文件);
    }
    // 文件结束
    printf("{@%d, %d:%d, \"EOF", 单词信息.计数, 单词信息.行号, 单词信息.列号);
    printf("\", <%d>, 0} 错误:%d\n", 单词信息.类型, 单词信息.错误);
    // fprintf(stdout, "\n");
    return 0;
}

// gcc -g -Wall lexer.c  utf8_unicode/utf8_unicode.c && ./a.out
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
