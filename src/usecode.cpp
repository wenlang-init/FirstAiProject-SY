#include "usecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <QDebug>
#include <QFile>
#include <QTime>
#if 0 // defined(Q_OS_WIN)
# include <QtZlib/zlib.h>
#endif // if defined(Q_OS_WIN)
#define qdebug qDebug().noquote()

#define KEYCORE 0XC2

unsigned char encrypt(unsigned char *p, unsigned int size, unsigned char key)
{
    for (unsigned int i = 0; i < size; i++)
    {
        p[i] ^= key;
    }
    return key ^ KEYCORE;
}

void decrypt(unsigned char *p, unsigned int size, unsigned char key)
{
    key ^= KEYCORE;

    for (unsigned int i = 0; i < size; i++)
    {
        p[i] ^= key;
    }
}

void creat_decode(const char   *srcchr,
                  const int     srcchrlen,
                  unsigned char enkey,
                  QByteArray  & dstData) {
    if (srcchrlen < 1) return;

    QByteArray sd, data;

    // sd = QByteArray::fromRawData(srcchr, srcchrlen);
    sd = QByteArray(srcchr, srcchrlen);
    data.resize(srcchrlen * 7 + 10000, 0);
    unsigned char *srcdata = (unsigned char *)sd.data();

    unsigned char dekey = encrypt(srcdata, srcchrlen, enkey);

    char *file_save = data.data();
    int   findex = 0;
    findex += sprintf(file_save + findex,
                      "#include <stdio.h>\n#include <string.h>\n");
    findex += sprintf(file_save + findex,
                      "void decrypt(unsigned char *p, unsigned int size, unsigned char key)\n{\n");
    findex += sprintf(file_save + findex, "\tkey ^= 0x%02X;\n", KEYCORE);
    findex += sprintf(file_save + findex,
                      "\tfor(unsigned int i=0; i<size; i++)\n\t{\n\t\tp[i] ^= key;\n\t}\n}\n");
    findex += sprintf(file_save + findex, "int main(){\n\t");


    findex += sprintf(file_save + findex, "unsigned char encode[] = {\n\t");

    for (int i = 0; i < srcchrlen - 1; i++) {
        if ((i > 0) && (i % 10 == 0)) {
            findex += sprintf(file_save + findex, "\n\t");
        }
        findex += sprintf(file_save + findex, "0x%02x,", srcdata[i]);
    }
    findex += sprintf(file_save + findex, "0x%02x};\n", srcdata[srcchrlen - 1]);


    findex += sprintf(file_save + findex,
                      "\tunsigned char dekey = 0x%02X;\n",
                      dekey);
    findex += sprintf(file_save + findex,
                      "\tdecrypt(encode,sizeof(encode),dekey);\n");
    findex += sprintf(file_save + findex,
                      "\tfor(int i=0;i<sizeof(encode);i++)printf(\"%%c\",encode[i]);printf(\"\\n\");\n");
    findex += sprintf(file_save + findex, "\treturn 0;\n}\n");
    findex += sprintf(file_save + findex, "\n");

    // file_save[findex] = 0;

    // printf("代码：\n");
    // printf("%s", file_save); fflush(stdout);

#if 0

    // const char* filename = "/storage/emulated/0/123.c";
    // const char* filename = "/storage/emulated/0/Download/WeiXin/123.c";
    FILE *fp = fopen(filename, "w");

    if (fp != NULL) {
        printf("start save file:%s\n", filename);
        fwrite(file_save, 1, findex, fp);
        fclose(fp);
    } else {
        perror("fopen");
    }
#endif // if 0

    dstData = data.left(findex);
}

useCode::useCode(QObject *parent)
    : QObject{parent}
{}

QByteArray useCode::getCode(QByteArray    sdata,
                            unsigned char enkey)
{
    QByteArray dstData;

    creat_decode(sdata.constData(), sdata.size(), enkey, dstData);
    return dstData;
}

QByteArray useCode::getDecode(QByteArray sdata)
{
    QByteArray destdata;
    QString    src = sdata;
    int index = src.indexOf("unsigned char dekey");

    if (index < 0) {
        return destdata;
    }

    int indexend = src.indexOf(";", index);

    if ((indexend < 0) || (indexend - 4 < 0)) {
        return destdata;
    }

    unsigned char dekey;
    QString strdekey = src.mid(indexend - 4, 4);
    bool    ok;
    dekey = strdekey.toUInt(&ok, 16);

    if (!ok) {
        return destdata;
    }

    index = src.indexOf("unsigned char encode");

    if (index < 0) {
        return destdata;
    }
    index = src.indexOf("{", index);

    if (index < 1) {
        return destdata;
    }

    indexend = src.indexOf("}", index);

    if (indexend < 2) {
        return destdata;
    }

    QString encode;
    encode = src.mid(index + 1, indexend - index - 1);
    encode = encode.remove("\n");
    encode = encode.remove("\t");
    encode = encode.remove("\r");
    encode = encode.trimmed();
    QStringList byteStrs = encode.split(",");
    QByteArray  encdata;

    for (const QString& byteStr : byteStrs) {
        bool ok;
        unsigned char byte = byteStr.trimmed().toUInt(&ok, 16);

        if (!ok) {
            return destdata;
        }
        encdata.append(byte);
    }
    decrypt((unsigned char *)encdata.data(), encdata.size(), dekey);
    return encdata;
}

#if 0 // defined(Q_OS_WIN)
static const int BufferSize = 1024 * 4;
QByteArray useCode::GzipCompress(QByteArray postBody)
{
    QByteArray outBuf;
    z_stream   c_stream;

    // int err = 0;
    // int windowBits = 15;
    int GZIP_ENCODING = 16;

    if (!postBody.isEmpty())
    {
        c_stream.zalloc = (alloc_func)0;
        c_stream.zfree = (free_func)0;
        c_stream.opaque = (voidpf)0;
        c_stream.next_in = (Bytef *)postBody.data();
        c_stream.avail_in = postBody.size();

        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                         MAX_WBITS + GZIP_ENCODING, 8,
                         Z_DEFAULT_STRATEGY) != Z_OK) return QByteArray();

        char destBuf[BufferSize];

        do {
            c_stream.avail_out = BufferSize;
            c_stream.next_out = (Bytef *)destBuf;

            int err = deflate(&c_stream, Z_FINISH);

            if (err == Z_STREAM_ERROR) break;
            outBuf.append(destBuf, BufferSize - c_stream.avail_out);

            if (err == Z_STREAM_END) break;
        } while (c_stream.avail_out == 0);

        deflateEnd(&c_stream);
    }
    return outBuf;
}

QByteArray useCode::GZipUnCompress(QByteArray src)
{
    QByteArray outBuffer;
    z_stream   strm;

    strm.zalloc = NULL;
    strm.zfree = NULL;
    strm.opaque = NULL;

    strm.avail_in = src.size();
    strm.next_in = (Bytef *)src.data();

    int err = -1;
    err = inflateInit2(&strm, MAX_WBITS + 16);

    if (err == Z_OK) {
        char destBuf[BufferSize];

        do {
            strm.avail_out = BufferSize;
            strm.next_out = (Bytef *)destBuf;

            int err = inflate(&strm, Z_FINISH);

            if (err == Z_STREAM_ERROR) break;
            outBuffer.append(destBuf, BufferSize - strm.avail_out);

            if (err != Z_OK) {
                if ((err == Z_STREAM_END) || (Z_NEED_DICT == err) ||
                    (Z_DATA_ERROR == err) ||
                    (Z_MEM_ERROR == err)) break;
            }
        } while (strm.avail_out == 0);
    }
    inflateEnd(&strm);
    return outBuffer;
}

bool useCode::GzipCompress(const QString& srcFilename,
                           const QString& destFilename)
{
    // -(15 ~ 8) : 纯 deflate 压缩+(15 ~ 8) : 带 zlib 头和尾> 16 : 带 gzip 头和尾
    int   GZIP_ENCODING = 16;
    QFile sfile(srcFilename);

    if (sfile.open(QIODevice::ReadOnly) == false)
    {
        qdebug << "打开文件失败:" << srcFilename;
        return false;
    }

    qint64 fileSize = sfile.size();
    qint64 fsize = fileSize;
    const qint64 bufferSize = BufferSize / 4;

    char sbuffer[bufferSize];
    int  bytesRead = 0;
    int  readSize = qMin(fileSize, bufferSize);

    QFile dfile(destFilename);

    if (dfile.open(QIODevice::WriteOnly) == false)
    {
        qdebug << "打开文件失败:" << destFilename;
        return false;
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    // if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK) return false;
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     MAX_WBITS + GZIP_ENCODING, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) return false;

    char  destBuf[BufferSize];
    QTime time = QTime::currentTime();

    while (readSize > 0 && (bytesRead = sfile.read(sbuffer, readSize)) > 0)
    {
        fileSize -= bytesRead;

        // QByteArray tmpdata = QByteArray::fromRawData(sbuffer, bytesRead);
        // qdebug << fileSize << bytesRead;
        zs.avail_in = bytesRead;
        zs.next_in = (Bytef *)sbuffer;
        int err = Z_OK;

        do {
            zs.avail_out = BufferSize;
            zs.next_out = (Bytef *)destBuf;

            if (fileSize <= 0) {
                err = deflate(&zs, Z_FINISH);
            } else {
                err = deflate(&zs, Z_NO_FLUSH);
            }

            if (err == Z_STREAM_ERROR) break;
            dfile.write(destBuf, BufferSize - zs.avail_out);

            if (err != Z_OK) {
                // qdebug << err << zs.total_in << zs.total_out << zs.avail_in
                // <<zs.avail_out;
            }
        } while (zs.avail_out == 0);

        readSize = qMin(fileSize, bufferSize);

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int infobl = (fsize - fileSize) * 100.0 / fsize;

            // emit sig_md5info(infobl);
            qdebug << infobl;
        }
    }
    deflateEnd(&zs);
    sfile.close();
    dfile.close();
    return true;
}

bool useCode::GZipUnCompress(const QString& srcFilename,
                             const QString& destFilename)
{
    // -(15 ~ 8) : 纯 deflate 压缩+(15 ~ 8) : 带 zlib 头和尾> 16 : 带 gzip 头和尾
    int   GZIP_ENCODING = 16;
    QFile sfile(srcFilename);

    if (sfile.open(QIODevice::ReadOnly) == false)
    {
        qdebug << "打开文件失败:" << srcFilename;
        return false;
    }

    qint64 fileSize = sfile.size();
    qint64 fsize = fileSize;
    const qint64 bufferSize = BufferSize / 4;

    char sbuffer[bufferSize];
    int  bytesRead = 0;
    int  readSize = qMin(fileSize, bufferSize);

    QFile dfile(destFilename);

    if (dfile.open(QIODevice::WriteOnly) == false)
    {
        qdebug << "打开文件失败:" << destFilename;
        return false;
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (inflateInit2(&zs, MAX_WBITS + GZIP_ENCODING) != Z_OK) return false;

    char  destBuf[BufferSize];
    QTime time = QTime::currentTime();

    while (readSize > 0 && (bytesRead = sfile.read(sbuffer, readSize)) > 0)
    {
        fileSize -= bytesRead;

        // QByteArray tmpdata = QByteArray::fromRawData(sbuffer, bytesRead);
        // qdebug << fileSize << bytesRead;
        zs.avail_in = bytesRead;
        zs.next_in = (Bytef *)sbuffer;
        int err = Z_OK;

        do {
            zs.avail_out = BufferSize;
            zs.next_out = (Bytef *)destBuf;

            if (fileSize <= 0) {
                err = inflate(&zs, Z_FINISH);
            } else {
                err = inflate(&zs, Z_NO_FLUSH);
            }

            if (err == Z_STREAM_ERROR) break;
            dfile.write(destBuf, BufferSize - zs.avail_out);

            if (err != Z_OK) {
                // qdebug << err << zs.total_in << zs.total_out << zs.avail_in
                // <<zs.avail_out;

                if ((Z_NEED_DICT == err) || (Z_DATA_ERROR == err) ||
                    (Z_MEM_ERROR == err)) break;
            }
        } while (zs.avail_out == 0);

        if (err == Z_STREAM_END) {
            qdebug << err << zs.total_in << zs.total_out << zs.avail_in <<
                zs.avail_out << fileSize;
            break;
        }

        readSize = qMin(fileSize, bufferSize);

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int infobl = (fsize - fileSize) * 100.0 / fsize;

            // emit sig_md5info(infobl);
            qdebug << infobl;
        }
    }
    inflateEnd(&zs);
    sfile.close();
    dfile.close();
    return true;
}

#endif // if defined(Q_OS_WIN)
