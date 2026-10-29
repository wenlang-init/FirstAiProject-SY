
/*
   |  Unicode符号范围      |  UTF-8编码方式
 n |  (十六进制)           | (二进制)
---+-----------------------+------------------------------------------------------
 1 | 0000 0000 - 0000 007F |                                              0xxxxxxx
 2 | 0000 0080 - 0000 07FF |                                     110xxxxx 10xxxxxx
 3 | 0000 0800 - 0000 FFFF |                            1110xxxx 10xxxxxx 10xxxxxx
 4 | 0001 0000 - 0010 FFFF |                   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 5 | 0020 0000 - 03FF FFFF |          111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 6 | 0400 0000 - 7FFF FFFF | 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

/* UTF8 utilities */

/*-
 * This parses a UTF8 string one character at a time. It is passed a pointer
 * to the string and the length of the string. It sets 'value' to the value of
 * the current character. It returns the number of characters read or a
 * negative error code:
 * -1 = string too short
 * -2 = illegal character
 * -3 = subsequent characters not of the form 10xxxxxx
 * -4 = character encoded incorrectly (not minimal length).
 */

 #include "unicode2gbktab.h"

int UTF8_getc(const unsigned char *str, int len, unsigned long *val)
{
    const unsigned char *p;
    unsigned long value;
    int ret;
    if (len <= 0)
        return 0;
    p = str;

    /* Check syntax and work out the encoded value (if correct) */
    if ((*p & 0x80) == 0) {
        value = *p++ & 0x7f;
        ret = 1;
    } else if ((*p & 0xe0) == 0xc0) {
        if (len < 2)
            return -1;
        if ((p[1] & 0xc0) != 0x80)
            return -3;
        value = (*p++ & 0x1f) << 6;
        value |= *p++ & 0x3f;
        if (value < 0x80)
            return -4;
        ret = 2;
    } else if ((*p & 0xf0) == 0xe0) {
        if (len < 3)
            return -1;
        if (((p[1] & 0xc0) != 0x80)
            || ((p[2] & 0xc0) != 0x80))
            return -3;
        value = (*p++ & 0xf) << 12;
        value |= (*p++ & 0x3f) << 6;
        value |= *p++ & 0x3f;
        if (value < 0x800)
            return -4;
        ret = 3;
    } else if ((*p & 0xf8) == 0xf0) {
        if (len < 4)
            return -1;
        if (((p[1] & 0xc0) != 0x80)
            || ((p[2] & 0xc0) != 0x80)
            || ((p[3] & 0xc0) != 0x80))
            return -3;
        value = ((unsigned long)(*p++ & 0x7)) << 18;
        value |= (*p++ & 0x3f) << 12;
        value |= (*p++ & 0x3f) << 6;
        value |= *p++ & 0x3f;
        if (value < 0x10000)
            return -4;
        ret = 4;
    } else if ((*p & 0xfc) == 0xf8) {
        if (len < 5)
            return -1;
        if (((p[1] & 0xc0) != 0x80)
            || ((p[2] & 0xc0) != 0x80)
            || ((p[3] & 0xc0) != 0x80)
            || ((p[4] & 0xc0) != 0x80))
            return -3;
        value = ((unsigned long)(*p++ & 0x3)) << 24;
        value |= ((unsigned long)(*p++ & 0x3f)) << 18;
        value |= ((unsigned long)(*p++ & 0x3f)) << 12;
        value |= (*p++ & 0x3f) << 6;
        value |= *p++ & 0x3f;
        if (value < 0x200000)
            return -4;
        ret = 5;
    } else if ((*p & 0xfe) == 0xfc) {
        if (len < 6)
            return -1;
        if (((p[1] & 0xc0) != 0x80)
            || ((p[2] & 0xc0) != 0x80)
            || ((p[3] & 0xc0) != 0x80)
            || ((p[4] & 0xc0) != 0x80)
            || ((p[5] & 0xc0) != 0x80))
            return -3;
        value = ((unsigned long)(*p++ & 0x1)) << 30;
        value |= ((unsigned long)(*p++ & 0x3f)) << 24;
        value |= ((unsigned long)(*p++ & 0x3f)) << 18;
        value |= ((unsigned long)(*p++ & 0x3f)) << 12;
        value |= (*p++ & 0x3f) << 6;
        value |= *p++ & 0x3f;
        if (value < 0x4000000)
            return -4;
        ret = 6;
    } else
        return -2;
    *val = value;
    return ret;
}

/*
 * This takes a character 'value' and writes the UTF8 encoded value in 'str'
 * where 'str' is a buffer containing 'len' characters. Returns the number of
 * characters written or -1 if 'len' is too small. 'str' can be set to NULL
 * in which case it just returns the number of characters. It will need at
 * most 6 characters.
 */

int UTF8_putc(unsigned char *str, int len, unsigned long value)
{
    if (!str)
        len = 6;                /* Maximum we will need */
    else if (len <= 0)
        return -1;
    if (value < 0x80) {
        if (str)
            *str = (unsigned char)value;
        return 1;
    }
    if (value < 0x800) {
        if (len < 2)
            return -1;
        if (str) {
            *str++ = (unsigned char)(((value >> 6) & 0x1f) | 0xc0);
            *str = (unsigned char)((value & 0x3f) | 0x80);
        }
        return 2;
    }
    if (value < 0x10000) {
        if (len < 3)
            return -1;
        if (str) {
            *str++ = (unsigned char)(((value >> 12) & 0xf) | 0xe0);
            *str++ = (unsigned char)(((value >> 6) & 0x3f) | 0x80);
            *str = (unsigned char)((value & 0x3f) | 0x80);
        }
        return 3;
    }
    if (value < 0x200000) {
        if (len < 4)
            return -1;
        if (str) {
            *str++ = (unsigned char)(((value >> 18) & 0x7) | 0xf0);
            *str++ = (unsigned char)(((value >> 12) & 0x3f) | 0x80);
            *str++ = (unsigned char)(((value >> 6) & 0x3f) | 0x80);
            *str = (unsigned char)((value & 0x3f) | 0x80);
        }
        return 4;
    }
    if (value < 0x4000000) {
        if (len < 5)
            return -1;
        if (str) {
            *str++ = (unsigned char)(((value >> 24) & 0x3) | 0xf8);
            *str++ = (unsigned char)(((value >> 18) & 0x3f) | 0x80);
            *str++ = (unsigned char)(((value >> 12) & 0x3f) | 0x80);
            *str++ = (unsigned char)(((value >> 6) & 0x3f) | 0x80);
            *str = (unsigned char)((value & 0x3f) | 0x80);
        }
        return 5;
    }
    if (len < 6)
        return -1;
    if (str) {
        *str++ = (unsigned char)(((value >> 30) & 0x1) | 0xfc);
        *str++ = (unsigned char)(((value >> 24) & 0x3f) | 0x80);
        *str++ = (unsigned char)(((value >> 18) & 0x3f) | 0x80);
        *str++ = (unsigned char)(((value >> 12) & 0x3f) | 0x80);
        *str++ = (unsigned char)(((value >> 6) & 0x3f) | 0x80);
        *str = (unsigned char)((value & 0x3f) | 0x80);
    }
    return 6;
}

#if 0
// 使用ICU库判断
#include <unicode/uchar.h>

int isCombiningCharacter(uint32_t unicode) {
    UCharCategory category = u_charType(unicode);

    //UBool u_isalpha(UChar32 c);        // 是否为字母
    //UBool u_isdigit(UChar32 c);        // 是否为数字
    //UBool u_isspace(UChar32 c);        // 是否为空白
    //UBool u_ispunct(UChar32 c);        // 是否为标点

    // 判断是否为组合标记
    // “一般类别”均为 Mn (非间距标记)、Mc (间距组合标记) 或 Me (包围标记
    if (category == U_NON_SPACING_MARK ||    // Mn
        category == U_COMBINING_SPACING_MARK || // Mc
        category == U_ENCLOSING_MARK) {      // Me
        return 1;
    }

    return 0;
}
#else
int isCombiningCharacter(unsigned long unicode)
{
    // Unicode 组合字符范围
    if ((unicode >= 0x0300 && unicode <= 0x036F)    || // Combining Diacritical Marks(组合用附加符号)
        (unicode >= 0x1AB0 && unicode <= 0x1AFF)    || // Combining Diacritical Marks Extended(组合用附加符号扩展)
        (unicode >= 0x1DC0 && unicode <= 0x1DFF)    || // Combining Diacritical Marks Supplement(组合用附加符号补充)
        (unicode >= 0x20D0 && unicode <= 0x20FF)    || // Combining Diacritical Marks for Symbols(组合用记号附加符号)
        (unicode >= 0xFE20 && unicode <= 0xFE2F)    || // Combining Half Marks(组合用半符号)
        (unicode >= 0x135D && unicode <=  0x135F)    // 埃塞俄比亚文
            // ......
            )
    {
        return 1; // 组合字符
    }
    //  VARIATION SELECTOR-16。它实是一种特殊类型的组合字符，
    // 但它的作用与传统的变音符号（如U+0301 重音符）完全不同。
    // 它的核心作用是：告诉系统，应将前面的那个基础字符渲染为
    // “表情符号样式”（Emoji Style），而不是“文本样式”（Text Style）。
    // U+FE0F不是用来组合新字母的，而是用来切换已有字符的显示模式(表情符号开关)
    if((unicode >= 0xFE00 && unicode <= 0xFE0F) || // 标准 变体选择器补充字符 VS1~VS16
           (unicode >= 0xE0100 && unicode <= 0xE01EF) // 补充 变体选择器补充字符
            )
    {
        return 2; // 变体选择符
    };
    return 0;
}
#endif

int unicode2gbk(unsigned long unicode, char *gbk)
{
    unsigned short gbk16;
    for(unsigned long i=0;i<Unic2GBKTabSize;i++){
        if(unicode2gbkTab[i] == unicode){
            gbk16 = i;
            gbk[0] = (gbk16 & 0xFF00) >> 8;
            gbk[1] = gbk16 & 0x00FF;
            return 0;
        }
    }
    return -1;
}

int gbk2unicode(char *gbk, unsigned long *unicode)
{
    unsigned short gbk16;
    gbk16 = (gbk[0] << 8) | gbk[1];
    if(gbk16 >= Unic2GBKTabSize){
        return -1;
    }
    *unicode = unicode2gbkTab[gbk16];
    return 0;
}
