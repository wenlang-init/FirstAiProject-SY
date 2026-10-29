#ifndef EDCRYPT_H
#define EDCRYPT_H

#ifdef __cplusplus
extern "C" {
#endif
/*
 * 加密算法,对数据进行异或操作
 *
 * @param p 数据指针
 * @param size 数据长度
 * @param key 加密密钥
 * @return 解密密钥
 *
 * @note 加密后的数据可以通过decrypt函数解密
*/
unsigned char encrypt(unsigned char *p, unsigned int size, unsigned char key);
/*
 * 解密算法,对数据进行异或操作
 *
 * @param p 数据指针
 * @param size 数据长度
 * @param key 解密密钥
 *
 * @note 解密密钥可以通过encrypt函数获取
*/
void decrypt(unsigned char *p, unsigned int size, unsigned char key);

/*
 * 加密算法,对数据进行异或操作
 *
 * @param p 数据指针
 * @param size 数据长度
 * @param key 加密密钥
 * @param dekey 解密密钥（输出）
 * @return 不足4个字符的余数或-1
 *
 * @note 加密后的数据可以通过decrypt函数解密
*/
int encrypt4(unsigned char *p, unsigned int size, unsigned char key[4],unsigned char dekey[4]);
/*
 * 解密算法,对数据进行异或操作
 *
 * @param p 数据指针
 * @param size 数据长度
 * @param key 解密密钥
 *
 * @note 解密密钥可以通过encrypt4函数获取
*/
void decrypt4(unsigned char *p, unsigned int size, unsigned char key[4]);

/*
 * 加密算法，每4bit得到2个数字(0~4)，每个字节得到4个0~4的数字，即8bit->8bit
 * 根据n=N*4+M; N=n/4,M=n%4;得到原始数据
 * @param data 数据指针
 * @param len 数据长度
 * @param key 加密偏移控制(0~15)，解密需一一对应
 *
 * @note key的初始值有第一次加密前指定，后续加密使用上次的key值
 */
void encryptbs(char *data,int len,char *key);
/*
 * 解密算法(0~4)，每个字节得到4个0~4的数字，转为8bit，即8bit->8bit
 * @param data 数据指针
 * @param len 数据长度
 * @param key 解密偏移控制(0~15)
 *
 * @note 解密时的key需使用第一次加密传入的key值，后续解密使用上次的key值
 */
void decryptbs(char *data,int len,char *key);

#ifdef __cplusplus
}
#endif

#endif // EDCRYPT_H
