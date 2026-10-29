#ifndef USECODE_H
#define USECODE_H

#include <QObject>

class useCode : public QObject {
    Q_OBJECT

public:

    explicit useCode(QObject *parent = nullptr);
    QByteArray getCode(QByteArray    sdata,
                       unsigned char enkey);

    QByteArray getDecode(QByteArray sdata);
#if 0 // defined(Q_OS_WIN)
    QByteArray GzipCompress(QByteArray postBody);
    QByteArray GZipUnCompress(QByteArray src);
    bool       GzipCompress(const QString& srcFilename,
                            const QString& destFilename);
    bool       GZipUnCompress(const QString& srcFilename,
                              const QString& destFilename);
#endif // if defined(Q_OS_WIN)

signals:
};

#endif // USECODE_H
