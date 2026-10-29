#ifndef BESTSOUND_H
#define BESTSOUND_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    转换为兽音语言编码,适用于2byte内的Unicode字符,兼容公用编码
    utf8src: 源数据,为utf-8编码
    len: 源数据长度
    dict: 兽音语言编码字典,为一个长度为4的数组,每个元素为一个字符串,表示一个兽音语言编码字符
    return: 兽音语言编码数据
*/
char *toBeastSound_2byte(const unsigned char *utf8src, int len,const char *dict[4]);
/*
    转换为utf-8编码,适用于2byte内的Unicode字符,兼容公用编码
    beastsound: 兽音语言编码数据
    len: 兽音语言编码数据长度
    return: utf-8编码数据
*/
char *fromBeastSound_2byte(const unsigned char *beastsound, int len);

/*
    转换为兽音语言编码
    utf8src: 源数据,为utf-8编码
    len: 源数据长度
    dict: 兽音语言编码字典,为一个长度为4的数组,每个元素为一个字符串,表示一个兽音语言编码字符
    return: 兽音语言编码数据
*/
char *toBeastSound_4byte(const unsigned char *utf8src, int len,const char *dict[4]);
/*
    转换为utf-8编码
    beastsound: 兽音语言编码数据
    len: 兽音语言编码数据长度
    return: utf-8编码数据
*/
char *fromBeastSound_4byte(const unsigned char *beastsound, int len);

/*
    转换为兽音语言编码,对原始数据编码,得到4倍大小数据
    src: 源数据
    len: 源数据长度
    dict: 兽音语言编码字典(UTF-8编码),为一个长度为4的数组,每个元素为一个字符串,表示一个兽音语言编码字符
    return: 兽音语言编码数据
*/
char *toBeastSound(const char *src, int len,const char *dict[4]);
/*
    转换为原始数据编码
    beastsound: 兽音语言编码数据
    len: 兽音语言编码数据长度
    destsize: 输出参数,返回原始数据长度
    return: utf-8编码数据
*/
char *fromBeastSound(const char *beastsound, int len,int *destsize);

// 流式编码：开始编码
char *toBeastSoundStreamStart(const char *dict[4]);
// 流式编码：编码函数
char *toBeastSoundStream(const char *src, int len,const char *dict[4]);

// 流式解码：开始解码
// beastsound:编码数据
// len:数据长度，必须大于字典需要的4个字，建议传入的长度大于24
// dict:函数执行后得到的字典unicode
int fromBeastSoundStreamStart(const char *beastsound, int len,unsigned long (*dict)[4]);
// 流式解码：解码函数
// beastsound:编码数据
// len:数据长度
// dict:fromBeastSoundStreamStart得到的dict
// dstsize:解码后的数据长度(返回的char*数据的长度)
// finishsize: 当前传入数据在这次处理完成的字节长度
// _index:内部编码控制数，需要初始化为0(fromBeastSoundStreamStart中使用的初始值为0，
//          所以这里需要定义初始值为0的index，后续传入改变量的地址)
char *fromBeastSoundStream(const char *beastsound, int len,const unsigned long dict[4],int *dstsize,int *finishsize,int *_index);

#ifdef __cplusplus
}
#endif
#endif
