#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "autf8.h"
//#include "finallyRecovery.h"

char *toBeastSound_2byte(const unsigned char *utf8src, int len,const char *dict[4])
{
    if(len < 1)return NULL;
    for(int i=0;i<4;i++){
        unsigned long val;
        int ret = UTF8_getc(dict[i], strlen(dict[i]), &val);
        if(ret < 0){
            printf("UTF8_getc error\n");fflush(stdout);
            return NULL;
        }
        if(val > 0xffff){
            printf("toBeastSound_2byte: dict unicode > 2byte\n");fflush(stdout);
            return NULL;
        }
    }

    unsigned long val;
    int ret,cnt = 0,unicodecnt=0;
    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    //AUTOPTRTYPENEWCNT(unsigned long,unicode,len);
    //AUTOPTRTYPENEWSIZE(unsigned long,unicode,sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(cnt < len){
        ret = UTF8_getc(utf8src+cnt, len-cnt, &val);
        if(ret < 0){
            printf("UTF8_getc error\n");fflush(stdout);
            free(unicode);
            return NULL;
        }
        cnt += ret;
        if(val > 0xffff){
            printf("toBeastSound_2byte: unicode > 2byte\n");fflush(stdout);
            free(unicode);
            return NULL;
        }
        unicode[unicodecnt] = val;
        unicodecnt++;
    }
    
    int lenght[4];
    lenght[0] = strlen(dict[0]);
    lenght[1] = strlen(dict[1]);
    lenght[2] = strlen(dict[2]);
    lenght[3] = strlen(dict[3]);
    int maxlen = lenght[0];
    for(int i=0;i<4;i++){
        if(maxlen < lenght[i])maxlen = lenght[i];
    }
    int beastsoundlen = lenght[0] + lenght[1] + lenght[2] + lenght[3];
    beastsoundlen += 8*maxlen*unicodecnt;
    for(int i=0;i<unicodecnt;i++){
        //printf("%08x ",(int)unicode[i]);
        if(unicode[i] > 0xffffffff){
            free(unicode);
            return NULL;
        }
    }
    //printf("\n");

    char *beastsound = (char *)malloc(beastsoundlen);
    if(!beastsound){
        free(unicode);
        return NULL;
    }
    beastsound[0] = 0;
    strcat(beastsound,dict[3]);
    strcat(beastsound,dict[1]);
    strcat(beastsound,dict[0]);
    int index = 0,c,k;
    for(int i=0;i<unicodecnt;i++){
        for(int j=0;j < 4;j++){
            if(index > 15)index = 0;
            c = (unicode[i] >> (12-j*4)) & 0xf;
            k = (c + index)%16;
            //k = c + index%16;if(k > 15)k -= 16;
            //printf("kk:%d,c:%d,k:%d,index:%d,%s,%s\n",(c + index)%16,c,k,index,dict[k/4],dict[k%4]);
            strcat(beastsound,dict[k/4]);
            strcat(beastsound,dict[k%4]);
            index++;
        }
    }
    strcat(beastsound,dict[2]);

    free(unicode);

    return beastsound;
}

char *fromBeastSound_2byte(const unsigned char *beastsound, int len)
{
    if(len < 12)return NULL;
    int ret,cnt = 0,unicodecnt=0;
    unsigned long dictunicode[4];
    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    //AUTOPTRTYPENEWSIZE(unsigned long,unicode,sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(cnt < len){
        ret = UTF8_getc(beastsound+cnt, len-cnt, &unicode[unicodecnt]);
        if(ret < 0){
            printf("UTF8_getc error\n");fflush(stdout);
            free(unicode);
            return NULL;
        }
        if(unicode[unicodecnt] > 0xffff){
            printf("fromBeastSound_2byte: unicode > 2byte\n");fflush(stdout);
            free(unicode);
            return NULL;
        }
        //421 3
        if(unicodecnt == 0){
            dictunicode[3] = unicode[unicodecnt];
        } else if(unicodecnt == 1){
            dictunicode[1] = unicode[unicodecnt];
        } else if(unicodecnt == 2){
            dictunicode[0] = unicode[unicodecnt];
        }
        cnt += ret;
        unicodecnt++;
    }

    dictunicode[2] = unicode[unicodecnt-1];
    for(int i=0;i< 4;i++){
        for(int j=0;j<4;j++){
            if(i == j)continue;
            if(dictunicode[i] == dictunicode[j]){
                printf("dict error\n");
                free(unicode);
                return NULL;
            }
        }
    }

    if((unicodecnt-4)%8 != 0){
        printf("data error,cnt:%d\n",unicodecnt);
        free(unicode);
        return NULL;
    }
    char *utf8 = (char *)malloc(sizeof(char)*unicodecnt*6+1); // 假设1个字6byte
    if(!utf8){
        free(unicode);
        return NULL;
    }
    utf8[0] = 0;
    int index = 0;
    for(int i=3;i<unicodecnt-1;i+=8){
        int c,k;
        unsigned long utf8_unicode = 0;
        int dictindex[8]={-1};
        for(int k=0;k<8;k++){
            int isfound = 0;
            for(int m=0;m<4;m++){
                if(unicode[i+k] == dictunicode[m]){
                    dictindex[k] = m;
                    isfound = 1;
                    break;
                }
            }
            if(isfound == 0){
                printf("data error\n");
                free(utf8);
                free(unicode);
                return NULL;
            }
        }
        for(int j=0;j < 4;j++){
            if(index > 15)index = 0;
            k = dictindex[j*2]*4 + dictindex[j*2+1];
            //c = (k - index)%16;
            //if(c < 0)c += 16;
            c = ((k - index) + 16)%16;
            utf8_unicode |= (unsigned long)c << (12-j*4);
            index++;
        }
        char utf8char[7];
        int cnt = UTF8_putc(utf8char,6,utf8_unicode);
        if(cnt < 0){
            printf("UTF8_putc error\n");
            free(utf8);
            free(unicode);
            return NULL;
        }
        utf8char[cnt] = 0;
        strcat(utf8,utf8char);
    }
    free(unicode);
    return utf8;
}

char *toBeastSound_4byte(const unsigned char *utf8src, int len,const char *dict[4])
{
    if(len < 1)return NULL;
    unsigned long val;
    int ret,cnt = 0,unicodecnt=0;
    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    //AUTOPTRTYPENEWSIZE(unsigned long,unicode,sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(cnt < len){
        ret = UTF8_getc(utf8src+cnt, len-cnt, &val);
        if(ret < 0){
            printf("UTF8_getc error\n");fflush(stdout);
            free(unicode);
            return NULL;
        }
        cnt += ret;
        unicode[unicodecnt] = val;
        unicodecnt++;
    }
    
    int lenght[4];
    lenght[0] = strlen(dict[0]);
    lenght[1] = strlen(dict[1]);
    lenght[2] = strlen(dict[2]);
    lenght[3] = strlen(dict[3]);
    int maxlen = lenght[0];
    for(int i=0;i<4;i++){
        if(maxlen < lenght[i])maxlen = lenght[i];
    }
    int beastsoundlen = lenght[0] + lenght[1] + lenght[2] + lenght[3];
    beastsoundlen += 16*maxlen*unicodecnt;

    char *beastsound = (char *)malloc(beastsoundlen);
    if(!beastsound){
        free(unicode);
        return NULL;
    }
    beastsound[0] = 0;
    strcat(beastsound,dict[3]);
    strcat(beastsound,dict[1]);
    strcat(beastsound,dict[0]);
    int index = 0,c,k;
    for(int i=0;i<unicodecnt;i++){
        for(int j=0;j < 8;j++){
            if(index > 15)index = 0;
            c = (unicode[i] >> (28-j*4)) & 0xf;
            k = (c + index)%16;
            strcat(beastsound,dict[k/4]);
            strcat(beastsound,dict[k%4]);
            index++;
        }
    }
    strcat(beastsound,dict[2]);

    free(unicode);

    return beastsound;
}

char *fromBeastSound_4byte(const unsigned char *beastsound, int len)
{
    if(len < 20)return NULL;
    int ret,cnt = 0,unicodecnt=0;
    unsigned long dictunicode[4];

    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    //AUTOPTRTYPENEWSIZE(unsigned long,unicode,sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(cnt < len){
        ret = UTF8_getc(beastsound+cnt, len-cnt, &unicode[unicodecnt]);
        if(ret < 0){
            printf("UTF8_getc error\n");
            free(unicode);
            return NULL;
        }
        //421 3
        if(unicodecnt == 0){
            dictunicode[3] = unicode[unicodecnt];
        } else if(unicodecnt == 1){
            dictunicode[1] = unicode[unicodecnt];
        } else if(unicodecnt == 2){
            dictunicode[0] = unicode[unicodecnt];
        }
        cnt += ret;
        unicodecnt++;
    }
    dictunicode[2] = unicode[unicodecnt-1];
    for(int i=0;i< 4;i++){
        for(int j=0;j<4;j++){
            if(i == j)continue;
            if(dictunicode[i] == dictunicode[j]){
                printf("dict error\n");
                free(unicode);
                return NULL;
            }
        }
    }

    if((unicodecnt-4)%16 != 0){
        printf("data error,cnt:%d\n",unicodecnt);
        free(unicode);
        return NULL;
    }
    char *utf8 = (char *)malloc(sizeof(char)*unicodecnt*6+1); // 假设1个字6byte
    utf8[0] = 0;
    int index = 0;
    for(int i=3;i<unicodecnt-1;i+=16){
        int c,k;
        unsigned long utf8_unicode = 0;
        int dictindex[16]={0};
        for(int k=0;k<16;k++){
            int isfound = 0;
            for(int m=0;m<4;m++){
                if(unicode[i+k] == dictunicode[m]){
                    dictindex[k] = m;
                    isfound = 1;
                    break;
                }
            }
            if(isfound == 0){
                printf("data error\n");
                free(utf8);
                free(unicode);
                return NULL;
            }
        }
        for(int j=0;j < 8;j++){
            if(index > 15)index = 0;
            k = dictindex[j*2]*4 + dictindex[j*2+1];
            //c = (k - index)%16;
            //if(c < 0)c += 16;
            c = ((k - index) + 16)%16;
            utf8_unicode |= (unsigned long)c << (28-j*4);
            index++;
        }
        char utf8char[7];
        int cnt = UTF8_putc(utf8char,6,utf8_unicode);
        if(cnt < 0){
            printf("UTF8_putc error\n");
            free(utf8);
            free(unicode);
            return NULL;
        }
        utf8char[cnt] = 0;
        strcat(utf8,utf8char);
    }
    free(unicode);
    return utf8;
}

char *toBeastSound(const char *src, int len,const char *dict[4])
{
    if(src == NULL || len < 1)return NULL;
    int lenght[4];
    lenght[0] = strlen(dict[0]);
    lenght[1] = strlen(dict[1]);
    lenght[2] = strlen(dict[2]);
    lenght[3] = strlen(dict[3]);
    int maxlen = lenght[0];
    for(int i=0;i<4;i++){
        if(maxlen < lenght[i])maxlen = lenght[i];
    }
    int beastsoundlen = maxlen*len*4 + lenght[0] + lenght[1] + lenght[2] + lenght[3];
    char *beastsound = (char *)malloc(beastsoundlen);
    if(!beastsound){
        perror("malloc:");
        return NULL;
    }
    int beastsoundindex = 0;
    memcpy(beastsound+beastsoundindex,dict[3],lenght[3]);
    beastsoundindex += lenght[3];
    memcpy(beastsound+beastsoundindex,dict[1],lenght[1]);
    beastsoundindex += lenght[1];
    memcpy(beastsound+beastsoundindex,dict[0],lenght[0]);
    beastsoundindex += lenght[0];
    memcpy(beastsound+beastsoundindex,dict[2],lenght[2]);
    beastsoundindex += lenght[2];

    int index = 0,c,k;;
    for(int i=0;i<len;i++){
        for(int j=0;j<2;j++){
            if(index > 15)index = 0;
            c = (src[i] >> (4-j*4)) & 0xf;
            k = (c + index)%16;
            memcpy(beastsound+beastsoundindex,dict[k/4],lenght[k/4]);
            beastsoundindex += lenght[k/4];
            memcpy(beastsound+beastsoundindex,dict[k%4],lenght[k%4]);
            beastsoundindex += lenght[k%4];
            index++;
        }
    }

    beastsound[beastsoundindex] = 0;
    return beastsound;
}

char *fromBeastSound(const char *beastsound, int len,int *destsize)
{
    if(beastsound==NULL || len < 8)return NULL;
    int ret,cnt = 0,unicodecnt=0;
    unsigned long dictunicode[4];
    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    //AUTOPTRTYPENEWSIZE(unsigned long,unicode,sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(cnt < len){
        ret = UTF8_getc(beastsound+cnt, len-cnt, &unicode[unicodecnt]);
        if(ret < 0){
            printf("UTF8_getc error\n");
            free(unicode);
            return NULL;
        }
        //421 3
        if(unicodecnt == 0){
            dictunicode[3] = unicode[unicodecnt];
        } else if(unicodecnt == 1){
            dictunicode[1] = unicode[unicodecnt];
        } else if(unicodecnt == 2){
            dictunicode[0] = unicode[unicodecnt];
        } else if(unicodecnt == 3){
            dictunicode[2] = unicode[unicodecnt];
        }
        cnt += ret;
        unicodecnt++;
    }

    if(unicodecnt < 4){
        printf("dict error\n");
        free(unicode);
        return NULL;
    }

    for(int i=0;i< 4;i++){
        for(int j=i+1;j<4;j++){
            if(dictunicode[i] == dictunicode[j]){
                printf("dict error\n");
                free(unicode);
                return NULL;
            }
        }
    }

    if((unicodecnt-4)%4 != 0){
        printf("data error,cnt:%d\n",unicodecnt);
        free(unicode);
        return NULL;
    }
    char *destdata = (char *)malloc(sizeof(char)*(unicodecnt-4)/4);
    int index = 0;
    for(int i=4;i<unicodecnt;i+=4){
        int c,k;
        char destval = 0;
        int dictindex[4]={0};
        for(int k=0;k<4;k++){
         int isfound = 0;
            for(int m=0;m<4;m++){
                if(unicode[i+k] == dictunicode[m]){
                    dictindex[k] = m;
                    isfound = 1;
                    break;
                }
            }
            if(isfound == 0){
                printf("%s,%d,data error\n",__FUNCTION__,__LINE__);
                free(destdata);
                free(unicode);
                return NULL;
            }
        }
        for(int j=0;j < 2;j++){
            if(index > 15)index = 0;
            k = dictindex[j*2]*4 + dictindex[j*2+1];
            //c = (k - index)%16;
            //if(c < 0)c += 16;
            c = ((k - index) + 16)%16;
            destval |= (char)c << (4-j*4);
            index++;
        }
        memcpy(destdata+((i-4)/4),&destval,1);
    }
    free(unicode);
    *destsize = (unicodecnt-4)/4;
    return destdata;
}

char *toBeastSoundStreamStart(const char *dict[4])
{
    int lenght[4];
    lenght[0] = strlen(dict[0]);
    lenght[1] = strlen(dict[1]);
    lenght[2] = strlen(dict[2]);
    lenght[3] = strlen(dict[3]);
    int beastsoundlen = 1 + lenght[0] + lenght[1] + lenght[2] + lenght[3];
    char *beastsound = (char *)malloc(beastsoundlen);
    if(!beastsound){
        perror("malloc:");
        return NULL;
    }
    int beastsoundindex = 0;
    memcpy(beastsound+beastsoundindex,dict[3],lenght[3]);
    beastsoundindex += lenght[3];
    memcpy(beastsound+beastsoundindex,dict[1],lenght[1]);
    beastsoundindex += lenght[1];
    memcpy(beastsound+beastsoundindex,dict[0],lenght[0]);
    beastsoundindex += lenght[0];
    memcpy(beastsound+beastsoundindex,dict[2],lenght[2]);
    beastsoundindex += lenght[2];
    beastsound[beastsoundindex] = 0;
    return beastsound;
}

char *toBeastSoundStream(const char *src, int len,const char *dict[4])
{
    if(src == NULL || len < 1)return NULL;
    int lenght[4];
    lenght[0] = strlen(dict[0]);
    lenght[1] = strlen(dict[1]);
    lenght[2] = strlen(dict[2]);
    lenght[3] = strlen(dict[3]);
    int maxlen = lenght[0];
    for(int i=0;i<4;i++){
        if(maxlen < lenght[i])maxlen = lenght[i];
    }
    int beastsoundlen = maxlen*len*4+1;
    char *beastsound = (char *)malloc(beastsoundlen);
    if(!beastsound){
        perror("malloc:");
        return NULL;
    }
    int beastsoundindex = 0;

    int index = 0,c,k;;
    for(int i=0;i<len;i++){
        for(int j=0;j<2;j++){
            if(index > 15)index = 0;
            c = (src[i] >> (4-j*4)) & 0xf;
            k = (c + index)%16;
            memcpy(beastsound+beastsoundindex,dict[k/4],lenght[k/4]);
            beastsoundindex += lenght[k/4];
            memcpy(beastsound+beastsoundindex,dict[k%4],lenght[k%4]);
            beastsoundindex += lenght[k%4];
            index++;
        }
    }

    beastsound[beastsoundindex] = 0;
    char *dst = (char*)malloc(beastsoundindex+1);
    memcpy(dst,beastsound,beastsoundindex+1);
    free(beastsound);
    if(!dst){
        perror("malloc:");
        return NULL;
    }
    return dst;
}

int fromBeastSoundStreamStart(const char *beastsound, int len,unsigned long (*dict)[4])
{
    if(beastsound==NULL || len < 4)return -1;

    int ret,cnt = 0,unicodecnt=0;
    unsigned long dictunicode[4];

    while(cnt < len){
        ret = UTF8_getc(beastsound+cnt, len-cnt, &dictunicode[unicodecnt]);
        if(ret < 0){
            printf("UTF8_getc error\n");
            return -1;
        }
        cnt += ret;
        unicodecnt++;
        if(unicodecnt == 4){
            break;
        }
    }

    // FILE *file = fopen("./aaa.txt","a");
    // fwrite(beastsound,1,cnt,file);
    // fclose(file);

    for(int i=0;i< 4;i++){
        for(int j=i+1;j<4;j++){
            if(dictunicode[i] == dictunicode[j]){
                printf("dict error\n");
                return -1;
            }
        }
    }
    //4213
    (*dict)[0] = dictunicode[2];
    (*dict)[1] = dictunicode[1];
    (*dict)[2] = dictunicode[3];
    (*dict)[3] = dictunicode[0];

    return cnt;
}
#include <stdio.h>
char *fromBeastSoundStream(const char *beastsound, int len,const unsigned long dict[4],int *dstsize,int *finishsize,int *_index)
{
    if(beastsound==NULL || finishsize == NULL)return NULL;
    int ret,cnt = 0,unicodecnt=0,cnttmp=0,allcnt=0;
    unsigned long *unicode = (unsigned long *)malloc(sizeof(unsigned long)*len);
    if(!unicode)return NULL;
    while(allcnt < len){
        ret = UTF8_getc(beastsound+allcnt, len-allcnt, &unicode[unicodecnt]);
        if(ret < 0){
            break;
        }
        unicodecnt++;
        cnttmp += ret;
        if(unicodecnt % 4 == 0){
            cnt += cnttmp;
            cnttmp = 0;
        }
        allcnt += ret;
    }

    // FILE *file = fopen("./aaa.txt","a");
    // fwrite(beastsound,1,cnt,file);
    // fclose(file);

    unicodecnt = unicodecnt/4*4;
    //printf("unicodecnt=%d,unicodedict=[%lu,%lu,%lu,%lu]\n",unicodecnt,dict[0],dict[1],dict[2],dict[3]);fflush(stdout);
    int _dstsize = sizeof(char)*unicodecnt/4;
    char *destdata = (char *)malloc(_dstsize);
    //int index = 0;
    for(int i=0;i<unicodecnt;i+=4){
        int c,k;
        char destval = 0;
        int dictindex[4]={0};
        for(int k=0;k<4;k++){
         int isfound = 0;
            for(int m=0;m<4;m++){
                if(unicode[i+k] == dict[m]){
                    dictindex[k] = m;
                    isfound = 1;
                    break;
                }
            }
            if(isfound == 0){
                printf("%s,%d,data error,unicode[%d]=%lu\n",__FUNCTION__,__LINE__,i+k,unicode[i+k]);fflush(stdout);
                free(destdata);
                free(unicode);
                return NULL;
            }
        }
        for(int j=0;j < 2;j++){
            if(*_index > 15)*_index = 0;
            k = dictindex[j*2]*4 + dictindex[j*2+1];
            //c = (k - (*_index))%16;
            //if(c < 0)c += 16;
            c = ((k - (*_index)) + 16)%16;
            destval |= (char)c << (4-j*4);
            (*_index)++;
        }
        memcpy(destdata+(i/4),&destval,1);
    }
    free(unicode);
    *finishsize = cnt;
    *dstsize = _dstsize;
    return destdata;
}
