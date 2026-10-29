#include "edcrypt.h"

static unsigned char enKey = 0xC2;

unsigned char encrypt(unsigned char *p, unsigned int size, unsigned char key)
{
    for(unsigned int i=0; i<size; i++)
    {
        p[i] ^= key;
    }
    return key^enKey;
}

void decrypt(unsigned char *p, unsigned int size, unsigned char key)
{
    key ^= enKey;
    for(unsigned int i=0; i<size; i++)
    {
        p[i] ^= key;
    }
}

int encrypt4(unsigned char *p, unsigned int size, unsigned char key[4], unsigned char dekey[4])
{
    if(!key || !dekey){
        return -1;
    }

    dekey[0] ^= key[0]^enKey;
    dekey[1] ^= key[1]^enKey;
    dekey[2] ^= key[2]^enKey;
    dekey[3] ^= key[3]^enKey;

    unsigned int imod = size%4;
    unsigned int icnt = size/4;
    if(imod == 3){
        p[size - 1] ^= dekey[2];
        p[size - 2] ^= dekey[1];
        p[size - 3] ^= dekey[0];
    } else if(imod == 2){
        p[size - 1] ^= dekey[1];
        p[size - 2] ^= dekey[0];
    } else if(imod == 1){
        p[size - 1] ^= dekey[0];
    }

    for(unsigned int i=0; i<icnt*4; i+=4)
    {
        p[i] ^= key[0];
        p[i+1] ^= key[1];
        p[i+2] ^= key[2];
        p[i+3] ^= key[3];
    }
    return imod;
}

void decrypt4(unsigned char *p, unsigned int size, unsigned char key[4])
{
    unsigned char dekey[4]={0};
    dekey[0] ^= key[0]^enKey;
    dekey[1] ^= key[1]^enKey;
    dekey[2] ^= key[2]^enKey;
    dekey[3] ^= key[3]^enKey;

    unsigned int imod = size%4;
    unsigned int icnt = size/4;
    if(imod == 3){
        p[size - 1] ^= dekey[2];
        p[size - 2] ^= dekey[1];
        p[size - 3] ^= dekey[0];
    } else if(imod == 2){
        p[size - 1] ^= dekey[1];
        p[size - 2] ^= dekey[0];
    } else if(imod == 1){
        p[size - 1] ^= dekey[0];
    }

    for(unsigned int i=0; i<icnt*4; i+=4)
    {
        p[i] ^= dekey[0];
        p[i+1] ^= dekey[1];
        p[i+2] ^= dekey[2];
        p[i+3] ^= dekey[3];
    }
}

void encryptbs(char *data,int len,char *key)
{
    char c,k;
    for(int i=0;i<len;i++){
        unsigned char temp = 0;
        for(int j=0;j<2;j++){
            if(*key > 15)*key = 0;
            c = (data[i] >> (4-j*4)) & 0xf;
            k = (c + *key)%16;
            temp |= (k/4 << (6-j*4)) | (k%4 << (4-j*4));
            (*key)++;
        }
        data[i] = temp;
    }
}

void decryptbs(char *data,int len,char *key)
{
    char c,k;
    for(int i=0;i<len;i++){
        char temp = 0;
        for(int j=0;j<2;j++){
            if(*key > 15)*key = 0;
            k = ((data[i] >> (6-j*4))&0x3)*4 + ((data[i] >> (4-j*4))&0x3);
            c = k - (*key);
            c+=16; // 由于是用于%16,所以这里等效于if(c < 0)c += 16;
            c = c%16;
            // 负数取模的不确定性：当 (k - (*key)) 为负数时，% 运算符的行为在 C/C++ 中是实现定义的（implementation-defined）。不同的编译器可能有不同的实现。
            // 也行会得到这样的结果，导致与预期不符：
            // k = 1,key=11,c=-10是预期结果，但是这里可能会得到c=10的结果
            //c = (k - (*key))%16;
            //if(c < 0)c += 16;
            temp |= c << (4-j*4);
            (*key)++;
        }
        data[i] = temp;
    }
}

#if 0
// 如果需要更强的安全性，考虑使用标准加密库
#include <openssl/aes.h>

void secure_encrypt(unsigned char *data, int len, unsigned char *key) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    for(int i = 0; i < len; i += 16) {
        AES_encrypt(data + i, data + i, &aes_key);
    }
}
#endif
