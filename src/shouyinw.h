#ifndef SHOUYINW_H
#define SHOUYINW_H

#include <QWidget>
#include <QMutex>
namespace Ui {
class ShouYinW;
}
class FTDE;
class ShouYinW : public QWidget {
    Q_OBJECT

public:

    Q_INVOKABLE explicit ShouYinW(QWidget *parent = nullptr);
    ~ShouYinW();
    void    setbtnEnable(bool enable);
#ifdef Q_OS_ANDROID
    QString uriToPath(const QString& uri);
#endif // ifdef Q_OS_ANDROID
    void    setLabelInfo(const QString& info);

    // 二进制转换。将number转换为binaryNum进制数，结果存储在value中。
    // number: 待转换的数字
    // binaryNum: 目标进制数，范围为2-36
    // value: 用于存储转换结果的数组，由调用者提供足够的内存空间
    // return: 转换后的数字的位个数
    int getBinary(unsigned long long number,
                  int                binaryNum,
                  int               *value);
    int getnumber(unsigned long long *number,
                  int                 binaryNum,
                  const char         *value);

protected:

    bool eventFilter(QObject *obj,
                     QEvent  *event) override;
    void dropEvent(QDropEvent *event)override;
    void dragEnterEvent(QDragEnterEvent *event)override;

private slots:

    void on_pushButton1_clicked();

    void on_pushButton2_clicked();

    void on_checkBox1_clicked();

    void on_checkBox1_2_clicked();

    void on_checkBox3_clicked();

    void on_pushButtonb1_clicked();

    void on_pushButtonb2_clicked();

    void on_pushButton_clicked();

    void on_pushButtonu_clicked();

    void on_pushButtonmd51_clicked();

    void on_pushButtonmd52_clicked();

    void on_pushButtonmd53_clicked();

    int  getHashValue(int value);

    void on_pushButtonC_clicked();

    void on_pushButtonP_clicked();

    void on_pushButtonf1_clicked();

    void on_pushButtonf2_clicked();

    void on_pushButtonfc_clicked();

    void on_pushButtoncl_clicked();

    void on_pushButtone_clicked();

    void on_pushButtond_clicked();

    void on_pushButtonff_clicked();

    void on_pushButtonj_clicked();

signals:

    void getFileHash(QString filePath,
                     int     hash);
    void tobeastSound(QString srcfile,
                      QString dstfile,
                      QString dictstr);
    void frombeastSound(QString srcfile,
                        QString dstfile);

    void sigencrpty(QString srcfile,
                    QString dstfile);
    void sigdecrpty(QString srcfile,
                    QString dstfile);

    void getDF(QString srcfile1,
               QString srcfile2,
               QString dictstr);

private:

    Ui::ShouYinW *ui;
    int checkindex;
    QThread *thread;
    QString _filename;
    FTDE *ftdt;
};

class FTDE : public QObject {
    Q_OBJECT

public:

    explicit FTDE(QObject *QObject = nullptr);
    ~FTDE();
    void stop();

public slots:

    void tobeastSound(QString srcfile,
                      QString dstfile,
                      QString dictstr);
    void frombeastSound(QString srcfile,
                        QString dstfile);

    void encrpty(QString srcfile,
                 QString dstfile);
    void decrpty(QString srcfile,
                 QString dstfile);

    void getDF(QString srcfile1,
               QString srcfile2,
               QString dictstr);

signals:

    void messgeString(QString info);

private:

    QThread *thread;
    qint8 isrun = 0;
    QMutex mutex;
};

#endif // SHOUYINW_H
