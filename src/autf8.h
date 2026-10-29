
#ifndef AUTF8_H
#define AUTF8_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTF-8 to Unicode
 *
 * @param str: UTF-8 字符串
 * @param len: 字符串长度
 * @param val: Unicode 数据
 * @return: UTF-8字的长度, 失败返回负值
 */
int UTF8_getc(const unsigned char *str, int len, unsigned long *val);

/*
 * Unicode to UTF-8
 *
 * 注意: 目标字符串长度必须大于等于 6 字节
 *
 * @param str: 目标字符串
 * @param len: 目标字符串长度
 * @param val: Unicode 数据
 * @return: 写入的字节数, 失败返回负值
*/
int UTF8_putc(unsigned char *str, int len, unsigned long value);

// 判断是否是组合字符段(unicode)
// 返回0: 不是组合字符或变体选择符
int isCombiningCharacter(unsigned long unicode);

/*
 * Unicode to GBK
 * 注意: 目标字符串长度必须大于等于 2 字节(16bit),有效数据为前2字节
 * 
 * @param unicode: Unicode 数据
 * @param gbk: 目标字符串
 * @return: 0, 失败返回负值
*/
int unicode2gbk(unsigned long unicode, const char *gbk);

/*
 * GBK to Unicode
 * 注意: gbk长度必须大于等于 2 字节(16bit),有效数据为前2字节
 * 
 * @param gbk: GBK 字符串
 * @param unicode: Unicode 数据
 * @return: 0, 失败返回负值
*/
int gbk2unicode(const char *gbk, unsigned long *unicode);

#ifdef __cplusplus
}
#endif

#endif
