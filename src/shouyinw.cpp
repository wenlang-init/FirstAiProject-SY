#include "shouyinw.h"
#include "ui_shouyinw.h"
#include "beastSound.h"
#include "funchelper.h"
#include "autf8.h"
#include "usecode.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QThread>
#include <QHBoxLayout>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QDebug>
#include <QDragEnterEvent>
#include "edcrypt.h"

#define qdebug qDebug().noquote()
#ifdef Q_OS_ANDROID
# include <QtCore/private/qandroidextras_p.h>
# include <QtCore/qjniobject.h>
#endif // ifdef Q_OS_ANDROID

// #include "decode/base64.h"

// 检查变体选择符
bool isVariationSelector(char32_t codePoint) {
    return isCombiningCharacter(codePoint) ? true : false;

    // return (codePoint >= 0xFE00 && codePoint <= 0xFE0F) || // 标准VS1~VS16
    //       (codePoint >= 0xE0100 && codePoint <= 0xE01EF); // 补充 变体选择器补充字符
}

// 遍历QString检测
void checkVariationSelectors(const QString& str) {
    for (int i = 0; i < str.length();) {
        QChar ch = str[i];
        char32_t codePoint;

        if (ch.isHighSurrogate() && (i + 1 < str.length()) &&
            str[i + 1].isLowSurrogate()) {
            codePoint = QChar::surrogateToUcs4(ch, str[i + 1]);

            if (isVariationSelector(codePoint)) {
                qDebug() << "Found supplementary VS at position" << i;
                i += 2; // 跳过代理对
                continue;
            }
        } else {
            codePoint = ch.unicode();

            if (isVariationSelector(codePoint)) {
                qDebug() << "Found standard VS at position" << i;
            }
        }
        i++;
    }
}

struct utf8node
{
    QString  word;
    char32_t variant;
    utf8node() {
        word = "";
        variant = 0;
    }
};

QList<utf8node>getVariationSelectors(const QString& input) {
    QList<utf8node> vsList;

    // qDebug() << input.size();

    for (int i = 0; i < input.length();) {
        QChar current = input[i];

        if (current.isHighSurrogate() && (i + 1 < input.length()) &&
            input[i + 1].isLowSurrogate()) {
            char32_t codePoint = QChar::surrogateToUcs4(current, input[i + 1]);

            if (!isVariationSelector(codePoint)) {
                utf8node vsEntry;
                vsEntry.word.append(current);
                vsEntry.word.append(input[i + 1]);
                vsList.append(vsEntry);

                // qDebug() << __FILE__ << __LINE__ << i << current <<input[i +
                // 1] << vsEntry.word;
            } else {
                vsList.last().variant = codePoint;

                // qDebug() << __FILE__ << __LINE__ << i << current << input[i +
                // 1];
            }
            i += 2;
            continue;
        }

        if (!isVariationSelector(current.unicode())) {
            utf8node vsEntry;
            vsEntry.word.append(current);
            vsList.append(vsEntry);

            // qDebug() << __FILE__ << __LINE__ << i << vsEntry.word
            // <<current.unicode();
        } else {
            // qDebug() << __FILE__ << __LINE__ << __FUNCTION__ << i << current;

            if (vsList.size() > 0) vsList.last().variant = current.unicode();
        }
        i++;
    }
    return vsList;
}

// 移除变体选择符
QString removeVariationSelectors(const QString& input) {
    QString result;

    for (int i = 0; i < input.length();) {
        QChar current = input[i];

        if (current.isHighSurrogate() && (i + 1 < input.length())) {
            QChar next = input[i + 1];

            if (next.isLowSurrogate()) {
                char32_t codePoint = QChar::surrogateToUcs4(current, next);

                if (!isVariationSelector(codePoint)) {
                    result.append(current);
                    result.append(next);
                }
                i += 2;
                continue;
            }
        }

        if (!isVariationSelector(current.unicode())) {
            result.append(current);
        }
        i++;
    }
    return result;
}

// 添加标准VS (U+FE0F 示例)
QString addEmojiVariant(const QString& baseChar) {
    return baseChar + QChar(0xFE0F);
}

// 添加标准VS 0xFE00~0xFE0F
QString addEmojiVariant(const QString& baseChar, char32_t variant) {
    if ((variant < 0xFE00) || (variant > 0xFE0F)) {
        return baseChar; // 非法范围，返回原字符
    }
    return baseChar + QChar(variant);
}

// 添加补充VS (U+E0100 示例)
QString addSupplementaryVariant(const QString& baseChar) {
    return baseChar + QChar(0xDB40) + QChar(0xDD00); // 代理对表示 U+E0100
}

// 添加补充VS 0xE0100~0xE01EF
QString addSupplementaryVariant(const QString& baseChar, char32_t variant) {
    if ((variant < 0xE0100) || (variant > 0xE01EF)) {
        return baseChar; // 非法范围，返回原字符
    }
    char32_t codePoint = variant - 0x10000;
    QChar    highSurrogate = QChar((codePoint >> 10) + 0xD800);
    QChar    lowSurrogate = QChar((codePoint & 0x3FF) + 0xDC00);
    return baseChar + highSurrogate + lowSurrogate;
}

ShouYinW::ShouYinW(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShouYinW)
    , checkindex(1)
{
    // QPalette pal = this->palette();
    // pal.setColor(QPalette::Background, Qt::transparent);
    // this->setPalette(pal);

    // setWindowFlags(Qt::FramelessWindowHint);    // 无边框
    // setAttribute(Qt::WA_TranslucentBackground); // 背景透明
    // setWindowOpacity(0.9);                      // 透明度

    this->setAcceptDrops(true);
    ui->setupUi(this);
    ui->labelinfo->setWordWrap(true);

    // ui->textEditIn->hide();
    // ui->textEditIn->setReadOnly(true);

    ui->lineEdit1->setPlaceholderText("video/default.mp4");
    ui->lineEdit2->setPlaceholderText("default.zip");

    QFont font = ui->spinBox->font();
    font.setCapitalization(QFont::AllUppercase);
    ui->spinBox->setFont(font);
    ui->spinBox->setDisplayIntegerBase(16);

    // ui->textEditIn->setFocus();

    // setAttribute(Qt::WA_DeleteOnClose, true);

    QStringList strlist;
    strlist << "MD4" << "MD5" << "SHA-1" << "SHA-224"
            << "SHA-256" << "SHA-384" << "SHA-512" << "SHA3-224"
            << "SHA3-256" << "SHA3-384" << "SHA3-512" << "Keccak-224"
            << "Keccak-256" << "Keccak-384" << "Keccak-512" << "BLAKE2b-160"
            << "BLAKE2b-25" << "BLAKE2b-384" << "BLAKE2b-512" << "BLAKE2s-128"
            << "BLAKE2s-160" << "BLAKE2s-224" << "BLAKE2s-25";

    ui->comboBox->addItems(strlist);
    ui->comboBox->setEditable(false);
    ui->comboBox->setCurrentIndex(1);

    thread = new QThread(this);
    FuncHelper::getInstance().moveToThread(thread);
    thread->start();
    connect(this,
            &ShouYinW::getFileHash,
            &FuncHelper::getInstance(),
            &FuncHelper::slotGetFileHash);
    connect(&FuncHelper::getInstance(), &FuncHelper::sig_hashinfo, this,
            [ = ](int value) {
        QString info = QString("%1:\n计算中...%2%").arg(_filename).arg(value);
        ui->textEditIn->setPlainText(info);
    });
    connect(&FuncHelper::getInstance(), &FuncHelper::sig_hashfinished, this,
            [ = ](const QString& hash) {
        ui->textEditIn->setPlainText(_filename + ":\n" + hash);
        ui->pushButtonmd52->setEnabled(true);
    });

    ftdt = new FTDE;
    connect(ftdt, &FTDE::messgeString, this, [ = ](QString info) {
        // setLabelInfo(info);
        ui->textEditIn->setText(info);

        if (info.contains("成功") || info.contains("失败") || info.contains("取消")) {
            setbtnEnable(true);
        }
    });
    connect(this, &ShouYinW::tobeastSound,   ftdt, &FTDE::tobeastSound);
    connect(this, &ShouYinW::frombeastSound, ftdt, &FTDE::frombeastSound);
    connect(this, &ShouYinW::sigdecrpty,     ftdt, &FTDE::decrpty);
    connect(this, &ShouYinW::sigencrpty,     ftdt, &FTDE::encrpty);
    connect(this, &ShouYinW::getDF,          ftdt, &FTDE::getDF);

    ui->lineEdit2->installEventFilter(this);
    ui->lineEdit1->installEventFilter(this);
}

ShouYinW::~ShouYinW()
{
    delete ui;

    FuncHelper::getInstance().stopGetFileHASH();
    thread->quit();
    thread->wait();
    delete thread;

    ftdt->stop();
    delete ftdt;
}

void ShouYinW::setbtnEnable(bool enable)
{
    ui->pushButtonf1->setEnabled(enable);
    ui->pushButtonf2->setEnabled(enable);
    ui->pushButtone->setEnabled(enable);
    ui->pushButtond->setEnabled(enable);
    ui->pushButtonff->setEnabled(enable);
}

#ifdef Q_OS_ANDROID
QString ShouYinW::uriToPath(const QString& uri) {
    QJniObject jniUri = QJniObject::fromString(uri);
    QJniObject contentResolver = jniUri.callObjectMethod(
        "getContentResolver", "()Landroid/content/ContentResolver;"
        );
    QJniObject cursor = contentResolver.callObjectMethod(
        "query",
        "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;",
        jniUri.object(),
        nullptr,
        nullptr,
        nullptr,
        nullptr
        );
    QString filePath;

    if (cursor.isValid() && cursor.callMethod<jboolean>("moveToFirst")) {
        int columnIndex = cursor.callMethod<jint>("getColumnIndex",
                                                  "(Ljava/lang/String;)I",
                                                  "_data");
        filePath = cursor.callObjectMethod("getString",
                                           "(I)Ljava/lang/String;",
                                           columnIndex).toString();
    }
    return filePath;
}

#endif // ifdef Q_OS_ANDROID

void ShouYinW::setLabelInfo(const QString& info)
{
    ui->labelinfo->setText(info);
}

int ShouYinW::getBinary(unsigned long long number, int binaryNum, int *value)
{
    if ((binaryNum < 2) || (binaryNum > 36)) {
        printf("Error: Binary number must be between 2 and 36.\n");
        fflush(stdout);
        return -1;
    }

    if (value == NULL) {
        printf("Error: values is NULL.\n");
        fflush(stdout);
        return -2;
    }

    int cnt = 0;
    unsigned long long remainder = number % binaryNum;
    unsigned long long val = number / binaryNum;

    if (number < 0) {
        printf("Error: number is minus.\n");
        fflush(stdout);
        return -3;
    }
    value[cnt++] = remainder;

    while (val != 0) {
        remainder = val % binaryNum;
        val = val / binaryNum;
        value[cnt++] = remainder;
    }

    return cnt;
}

int ShouYinW::getnumber(unsigned long long *number,
                        int                 binaryNum,
                        const char         *value)
{
    if ((binaryNum < 2) || (binaryNum > 36)) {
        printf("Error: Binary number must be between 2 and 36.\n");
        fflush(stdout);
        return -1;
    }
    int len = strlen(value);
    unsigned long long step = 1;
    unsigned long long out = 0;

    for (int i = len - 1; i >= 0; i--) {
        char c = value[i];
        long long digit;

        if ((c >= '0') && (c <= '9')) {
            digit = c - '0';
        } else if ((c >= 'A') && (c <= 'Z')) {
            digit = c - 'A' + 10;
        } else if ((c >= 'a') && (c <= 'z')) {
            digit = c - 'a' + 10;
        } else {
            printf("Error: Invalid character '%c' in input.\n", c);
            fflush(stdout);
            return -2;
        }

        if (binaryNum <= digit) {
            printf("Error: Invalid character '%c' in input.\n", c);
            fflush(stdout);
            return -100;
        }

        out += digit * step;
        step *= binaryNum;
    }

    *number = out;
    return 0;
}

bool ShouYinW::eventFilter(QObject *obj, QEvent *event)
{
    // QEvent::FocusOut;QEvent::FocusIn
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == ui->lineEdit1) {
            QString fileName = QFileDialog::getOpenFileName(this,
                                                            QStringLiteral("打开"),
                                                            "./",
                                                            QStringLiteral(
                                                                "(*)"));
            ui->lineEdit1->setText(fileName);

            // return true;
        } else if (obj == ui->lineEdit2) {
            QString fileName = QFileDialog::getOpenFileName(this,
                                                            QStringLiteral("打开"),
                                                            "./",
                                                            QStringLiteral("(*)"));
            ui->lineEdit2->setText(fileName);

            // return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ShouYinW::on_pushButton1_clicked()
{
    setLabelInfo("");
    QString qsdict = ui->lineEdit->text();

#if 0
    QList<utf8node> utf8wordtab = getVariationSelectors(qsdict);

    qDebug() << __FILE__ << __LINE__ << __FUNCTION__ << utf8wordtab.size();

    for (int i = 0; i < utf8wordtab.size(); i++) {
        qDebug() << __FILE__ << __LINE__ << __FUNCTION__ <<
            utf8wordtab[i].word.size() << utf8wordtab[i].word <<
            utf8wordtab[i].variant;

        for (int j = 0; j < utf8wordtab[i].word.size(); j++) {
            if (utf8wordtab[i].word[j].isHighSurrogate()) {
                qDebug() << i << j << utf8wordtab[i].word;
            }
        }
    }
    {
        QByteArray utf8d = qsdict.toUtf8();
        int cnt = 0, ret, unicodecnt = 0, len = utf8d.size();
        unsigned long *unicode = (unsigned long *)malloc(
            sizeof(unsigned long) * len);

        while (cnt < len) {
            ret = UTF8_getc((const unsigned char *)utf8d.data() + cnt,
                            len - cnt,
                            &unicode[unicodecnt]);

            if (ret < 0) {
                printf("UTF8_getc error\n"); fflush(stdout);
                break;
            }
            cnt += ret;
            unicodecnt++;
        }
        qDebug() << __FILE__ << __LINE__ << __FUNCTION__ << unicodecnt;

        for (int i = 0; i < unicodecnt; i++) {
            qDebug() << __FILE__ << __LINE__ << __FUNCTION__ << i <<
                QString::number(unicode[i], 16);
        }
        free(unicode);
    }
#endif // if 0

    // for(int i=0;i<qsdict.length();i++){
    //     QChar high = qsdict[i];
    //     qDebug()<<"11111111111" << high;
    //     // 字符❤️(\u2764\ufe0f组合字符)解析出问题
    //     if (high.isHighSurrogate() && i + 1 < qsdict.length()) {
    //         QChar low = qsdict[i + 1];
    //         qDebug()<<"222222222222" << high << low;
    //         if (low.isLowSurrogate()) {
    //             uint32_t codePoint = QChar::surrogateToUcs4(high, low);
    //             // 处理码点
    //             i++; // 跳过低代理
    //             qDebug()<<"333333333333" << high << low <<
    // QString::number(codePoint,16);
    //             continue;
    //         }
    //     }
    // }
    // 字符❤️(\u2764\ufe0f组合字符)解析出问题

    // qsdict = removeVariationSelectors(qsdict);
    auto ucs4 = qsdict.toUcs4(); // 字符的Unicode如果超过16bit,QString.size 返回2

    // ，根据这个函数可得到正确个数

    if (ucs4.size() != 4) {
        setLabelInfo(QString("秘钥为4个不同的字(注:不能是❤️这样的组合字符),%1").arg(
                         qsdict));
        qDebug() << ucs4 << ucs4.size() << qsdict << qsdict.size() <<
            qsdict.unicode() << qsdict.split("");

        QString s = removeVariationSelectors(qsdict);
        auto    _ucs4 = s.toUcs4();
        qDebug() << _ucs4 << _ucs4.size() << s << s.size() << s.unicode();

        // QString text = u8"▶";  // U+25B6
        // text += QChar(0xFE0F); // 添加VS-16（Emoji样式）
        // qDebug() << text;      // 显示为Emoji风格的▶
        // QString clean = removeVariationSelectors(text);
        // qDebug() << clean;     // 显示为文本风格的▶
        return;
    }

    for (int i = 0; i < ucs4.size(); i++) {
        for (int j = i + 1; j < ucs4.size(); j++) {
            if (ucs4.at(i) == ucs4.at(j)) {
                setLabelInfo(QString("秘钥为4个不同的字(注:不能是❤️这样的组合字符),%1").arg(
                                 qsdict));
                qDebug() << ucs4 << ucs4.size() << qsdict << qsdict.size();
                return;
            }
        }
    }
    QByteArray  dictqb[4];
    const char *dict[4];

    for (int i = 0; i < 4; i++) {
#if 0
        dictqb[i] = QString::fromUcs4((const uint *)&ucs4[i]).toUtf8(); // QChar::fromUcs4(ucs4[i]);
#else // if 0
        char utf8[7];
        int  cnt = UTF8_putc((unsigned char *)utf8, 6, ucs4[i]);

        if (cnt < 1) {
            setLabelInfo(QString("秘钥错误,%1").arg(qsdict));
            return;
        }
        dictqb[i].append(utf8, cnt);
        dictqb[i].append("\0");
#endif // if 0
        dict[i] = dictqb[i].constData();

        // DEBUG_PRINT_LOG("%s\n",dict[i]);
        // DEBUG_PRINT_LOG("%s\n",utf8);
        // qDebug()<<QString::fromUtf8(utf8);
    }
    QByteArray indata = ui->textEditIn->toPlainText().toUtf8();

    if (indata.size() < 1) return;

    switch (checkindex) {
    case 1:
    {
        for (int i = 0; i < 4; i++) {
            unsigned long val;
            int ret = UTF8_getc((const unsigned char *)dict[i],
                                strlen(dict[i]),
                                &val);

            if (ret < 0) {
                qDebug() << "UTF8_getc error\n";
                setLabelInfo("编码错误");
                return;
            }

            if (val > 0xffff) {
                qDebug() << "toBeastSound_2byte: unicode > 2byte\n";
                setLabelInfo(QString("编码错误:词典不能是unicode大于16bit的字 %1").arg(qsdict));
                return;
            }
        }
        char *p =
            toBeastSound_2byte((const unsigned char *)indata.constData(),
                               (int)indata.size(),
                               dict);

        if (p) {
            QString dest;
            dest.append(p);
            free(p);

            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("编码错误:字符串不能包含unicode大于16bit的字");
        }
        break;
    }

    case 2:
    {
        char *p =
            toBeastSound_4byte((const unsigned char *)indata.constData(),
                               (int)indata.size(),
                               dict);

        if (p) {
            QString dest;
            dest.append(p);
            free(p);
            QString dd;

            // for (int i = 0; i < dest.size(); i++) {
            //     qDebug() << i << dest[i] << QChar(0x2764);
            //     dd.append(dest[i]);

            //     if (QChar(0x2764) == dest[i]) dd.append(QChar(0xFE0F));

            //     // dd=addEmojiVariant(dd,0xFE0F);
            //     ui->textEditOut->setPlainText(dd);
            // }

            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("编码错误");
        }
        break;
    }

    case 3:
    {
        char *p = toBeastSound((const char *)indata.constData(),
                               (int)indata.size(),
                               dict);

        if (p) {
            QString dest;
            dest.append(p);
            free(p);

            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("编码错误");
        }
        break;
    }

    default:
        break;
    }
}

void ShouYinW::on_pushButton2_clicked()
{
    setLabelInfo("");
    QByteArray indata = ui->textEditIn->toPlainText().toUtf8();

    if (indata.size() < 1) return;

    switch (checkindex) {
    case 1:
    {
        char *p = fromBeastSound_2byte((const unsigned char *)indata.constData(),
                                       (int)indata.size());

        if (p) {
            QString dest;
            dest.append(p);
            free(p);
            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("解码错误");
        }
        break;
    }

    case 2:
    {
        char *p = fromBeastSound_4byte((const unsigned char *)indata.constData(),
                                       (int)indata.size());

        if (p) {
            QString dest;
            dest.append(p);
            free(p);
            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("解码错误");
        }
        break;
    }

    case 3:
    {
        int   destsize;
        char *p = fromBeastSound((const char *)indata.constData(),
                                 (int)indata.size(),
                                 &destsize);

        if (p) {
            QByteArray destb;
            destb.append(p, destsize);
            free(p);
            QString dest = destb;
            ui->textEditIn->setPlainText(dest);
        } else {
            setLabelInfo("解码错误");
        }
        break;
    }

    default:
        break;
    }
}

void ShouYinW::on_checkBox1_clicked()
{
    checkindex = 1;
    ui->checkBox1->setChecked(true);
    ui->checkBox1_2->setChecked(false);
    ui->checkBox3->setChecked(false);
}

void ShouYinW::on_checkBox1_2_clicked()
{
    checkindex = 2;
    ui->checkBox1->setChecked(false);
    ui->checkBox1_2->setChecked(true);
    ui->checkBox3->setChecked(false);
}

void ShouYinW::on_checkBox3_clicked()
{
    checkindex = 3;
    ui->checkBox1->setChecked(false);
    ui->checkBox1_2->setChecked(false);
    ui->checkBox3->setChecked(true);
}

void ShouYinW::on_pushButtonb1_clicked()
{
    setLabelInfo("");
    QString str = ui->textEditIn->toPlainText();

    if (str.size() < 1) return;

    QByteArray data = str.toUtf8().toBase64();

    ui->textEditIn->setPlainText(data);
}

void ShouYinW::on_pushButtonb2_clicked()
{
    setLabelInfo("");
    QString str = ui->textEditIn->toPlainText();

    if (str.size() < 1) return;

    QByteArray data = QByteArray::fromBase64(
        str.toUtf8());
    ui->textEditIn->setPlainText(QString::fromUtf8(data));
}

void ShouYinW::on_pushButton_clicked()
{
    setLabelInfo("");
    useCode ucode;
    QString str = ui->textEditIn->toPlainText();

    if (str.size() < 1) return;

    QByteArray data = ucode.getCode(str.toStdString().c_str(),
                                    ui->checkBox->isChecked() ?
                                    (unsigned char)(rand() % 256)
                                    : ui->spinBox->value());

    // qDebug().noquote() << data;
    ui->textEditIn->setPlainText(data);

    // QByteArray aa =
    //
    //
    // "sadsapkansak;lfsa\nadsgasdfagdskhkjbfdhsdfhsljfdgsuahsjlkjfdbgsalsjashgashdaushiguisahkjgiashgdiuashdigaiushhgdisusad1阿法狗萨芬";
    // QByteArray bb = ucode.GzipCompress(aa);
    // QByteArray cc = ucode.GZipUnCompress(bb);
    // qDebug() << aa.size() << bb.size() << cc.size() << aa.compare(cc);
    // qDebug() << ucode.GzipCompress("D:\\杂项\\下载\\OP_720.wmv",
    //                                "D:\\杂项\\下载\\OP_720.wmv.zip");
    // qDebug() << ucode.GZipUnCompress("D:\\杂项\\下载\\OP_720.wmv.zip",
    //                                  "D:\\杂项\\下载\\OP_720-.wmv");
}

void ShouYinW::on_pushButtonu_clicked()
{
    setLabelInfo("");
    useCode ucode;
    QString str = ui->textEditIn->toPlainText();

    if (str.size() < 1) return;

    QByteArray data = ucode.getDecode(str.toStdString().c_str());
    ui->textEditIn->setPlainText(data);
}

void ShouYinW::on_pushButtonmd51_clicked()
{
    setLabelInfo("");
    QString str = ui->textEditIn->toPlainText();
    QByteArray data = str.toUtf8();
    int value = getHashValue(ui->comboBox->currentIndex());
    QString md5 =
        FuncHelper::getInstance().getDataHash(data,
                                              (QCryptographicHash::Algorithm)value);

    ui->textEditIn->setPlainText(md5);
}

void ShouYinW::on_pushButtonmd52_clicked()
{
    setLabelInfo("");
    QFileDialog fileDialog(this);

    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("打开文件"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::
                                                              MoviesLocation)
                            .value(0, QDir::homePath()));

    if (fileDialog.exec() == QDialog::Accepted) {
        if (fileDialog.selectedFiles().isEmpty()) return;

        QString file = fileDialog.selectedFiles().constFirst();
        _filename = file;

        ui->textEditIn->setPlainText(_filename);

        FuncHelper::getInstance().stopGetFileHASH();
        int value = ui->comboBox->currentIndex();
        ui->pushButtonmd52->setEnabled(false);
        emit getFileHash(_filename, getHashValue(value));
    }
}

void ShouYinW::on_pushButtonmd53_clicked()
{
    setLabelInfo("");
    FuncHelper::getInstance().stopGetFileHASH();
}

int ShouYinW::getHashValue(int value) {
    int control = QCryptographicHash::Algorithm::Md5;

    switch (value) {
    case 0:
        control = QCryptographicHash::Md4;
        break;

    case 1:
        control = QCryptographicHash::Md5;
        break;

    case 2:
        control = QCryptographicHash::Sha1;
        break;

    case 3:
        control = QCryptographicHash::Sha224;
        break;

    case 4:
        control = QCryptographicHash::Sha256;
        break;

    case 5:
        control = QCryptographicHash::Sha384;
        break;

    case 6:
        control = QCryptographicHash::Sha512;
        break;

    case 7:
        control = QCryptographicHash::Sha3_224;
        break;

    case 8:
        control = QCryptographicHash::Sha3_256;
        break;

    case 9:
        control = QCryptographicHash::Sha3_384;
        break;

    case 10:
        control = QCryptographicHash::Sha3_512;
        break;

    case 11:
        control = QCryptographicHash::Keccak_224;
        break;

    case 12:
        control = QCryptographicHash::Keccak_256;
        break;

    case 13:
        control = QCryptographicHash::Keccak_384;
        break;

    case 14:
        control = QCryptographicHash::Keccak_512;
        break;

    case 15:
        control = QCryptographicHash::Blake2b_160;
        break;

    case 16:
        control = QCryptographicHash::Blake2b_256;
        break;

    case 17:
        control = QCryptographicHash::Blake2b_384;
        break;

    case 18:
        control = QCryptographicHash::Blake2b_512;
        break;

    case 19:
        control = QCryptographicHash::Blake2s_128;
        break;

    case 20:
        control = QCryptographicHash::Blake2s_160;
        break;

    case 21:
        control = QCryptographicHash::Blake2s_224;
        break;

    case 22:
        control = QCryptographicHash::Blake2s_256;
        break;

    default:
        break;
    }
    return control;
}

void ShouYinW::on_pushButtonC_clicked()
{
    setLabelInfo("");
    QString str = ui->textEditIn->toPlainText();
    QClipboard *clip = QApplication::clipboard();

    if (clip == nullptr) return;

    clip->setText(str);
}

void ShouYinW::on_pushButtonP_clicked()
{
    setLabelInfo("");

    // 获取剪切板对象
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasText()) {
        QString str = mimeData->text();

        ui->textEditIn->setPlainText(str);
    }
}

void ShouYinW::on_pushButtonf1_clicked()
{
    setLabelInfo("");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                     QStringLiteral("打开"), "./",
                                                     QStringLiteral("(*)"));

    if (fileName.isEmpty()) return;

    QString fileNames = QFileDialog::getSaveFileName(this,
                                                     QStringLiteral("保存"),
                                                     "./",
                                                     QStringLiteral("(*.txt)"));

    if (fileNames.isEmpty()) return;

    setbtnEnable(false);
    emit tobeastSound(fileName, fileNames, ui->lineEdit->text());
}

void ShouYinW::on_pushButtonf2_clicked()
{
    setLabelInfo("");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                     QStringLiteral("打开"), "./",
                                                     QStringLiteral("(*.txt)"));

    if (fileName.isEmpty()) return;

    QString fileNames = QFileDialog::getSaveFileName(this,
                                                     QStringLiteral("保存"),
                                                     "./",
                                                     QStringLiteral("(*)"));

    if (fileNames.isEmpty()) return;

    setbtnEnable(false);

    emit frombeastSound(fileName, fileNames);
}

void ShouYinW::on_pushButtonfc_clicked()
{
    ftdt->stop();
}

void ShouYinW::on_pushButtoncl_clicked()
{
    ui->textEditIn->clear();
}

void ShouYinW::on_pushButtone_clicked()
{
    setLabelInfo("");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                     QStringLiteral("打开"), "./",
                                                     QStringLiteral("(*)"));

    if (fileName.isEmpty()) return;

    QString fileNames = QFileDialog::getSaveFileName(this,
                                                     QStringLiteral("保存"),
                                                     "./",
                                                     QStringLiteral("(*)"));

    if (fileNames.isEmpty()) return;

    setbtnEnable(false);

    emit sigencrpty(fileName, fileNames);
}

void ShouYinW::on_pushButtond_clicked()
{
    setLabelInfo("");

    QString fileName = QFileDialog::getOpenFileName(this,
                                                     QStringLiteral("打开"), "./",
                                                     QStringLiteral("(*)"));

    if (fileName.isEmpty()) return;

    QString fileNames = QFileDialog::getSaveFileName(this,
                                                     QStringLiteral("保存"),
                                                     "./",
                                                     QStringLiteral("(*)"));

    if (fileNames.isEmpty()) return;

    setbtnEnable(false);

    emit sigdecrpty(fileName, fileNames);
}

void ShouYinW::on_pushButtonff_clicked()
{
    setLabelInfo("");

    QString fileName1, fileName2, fileName3;
    fileName1 = ui->lineEdit1->text();
    fileName2 = ui->lineEdit2->text();

    fileName3 = QFileDialog::getSaveFileName(this,
                                             QStringLiteral("保存"),
                                             "./",
                                             QStringLiteral("(*)"));

    if (fileName2.isEmpty() || fileName3.isEmpty()) {
        return;

        // fileName3 = "./testout.mp4";
    }

    setbtnEnable(false);

    emit getDF(fileName1, fileName2, fileName3);
}

void ShouYinW::on_pushButtonj_clicked()
{
    setLabelInfo("");
    QString inputstr = ui->lineEditj->text();

    int inputmode = ui->spinBox1->value();
    int outmode = ui->spinBox2->value();

    if (inputmode == outmode) return;

    quint64 inputnum;

    if (getnumber(&inputnum, inputmode, inputstr.toUtf8().constData()) != 0) {
        setLabelInfo("输入错误");
        return;
    }

    QString outstr;

    if (outmode == 10) {
        outstr = QString::number(inputnum);
    } else {
        int buff[1024];
        int cnt = getBinary(inputnum, outmode, buff);

        if (cnt < 0) {
            setLabelInfo("转换错误");
            return;
        }

        for (int i = cnt - 1; i >= 0; i--) {
            if (buff[i] < 10) {
                outstr.append(QChar('0' + buff[i]));
            } else {
                outstr.append(QChar('A' + buff[i] - 10));
            }
        }
    }
    ui->textEditIn->setText(outstr);
}

void ShouYinW::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        // qdebug << "拖入文件:" << event->mimeData()->urls();
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
    QWidget::dragEnterEvent(event);
}

void ShouYinW::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        qdebug << urls;

        if (urls.size() > 0) {
            QString file = urls[0].toLocalFile();

            // qdebug << file;
        }
    }
    QWidget::dropEvent(event);
}

FTDE::FTDE(QObject *parent) : QObject(parent)
{
    isrun = 0;
    thread = new QThread(this);
    this->moveToThread(thread);
    thread->start();
}

FTDE::~FTDE()
{
    stop();
    thread->quit();
    thread->wait();
}

void FTDE::stop()
{
    isrun = 0;
}

void FTDE::tobeastSound(QString srcfile, QString dstfile, QString qsdict)
{
    auto ucs4 = qsdict.toUcs4();

    if (ucs4.size() != 4) {
        emit messgeString(QString("编码失败.秘钥为4个不同的字(注:不能是❤️这样的组合字符),%1").arg(
                              qsdict));
        return;
    }

    QByteArray  dictqb[4];
    const char *dict[4];

    for (int i = 0; i < 4; i++) {
        char utf8[7];
        int  cnt = UTF8_putc((unsigned char *)utf8, 6, ucs4[i]);

        if (cnt < 1) {
            emit messgeString(QString("编码失败.秘钥错误,%1").arg(qsdict));
            return;
        }
        dictqb[i].append(utf8, cnt);
        dictqb[i].append("\0");
        dict[i] = dictqb[i].constData();
    }

    QFile file(srcfile);

    if (!file.open(QIODevice::ReadOnly)) {
        emit messgeString(QString("open失败:%1").arg(file.errorString()));
        return;
    }

    QFile files(dstfile);

    if (!files.open(QIODevice::WriteOnly)) {
        emit messgeString(QString("open失败:%1").arg(files.errorString()));
        file.close();
        return;
    }

    char *p = toBeastSoundStreamStart(dict);

    if (!p) {
        emit messgeString("编码失败");
        file.close();
        files.close();
        return;
    }

    files.write(p, strlen(p));
    free(p);

    int    readsize = 1024;
    char   buff[1024 * 2];
    qint64 ret = 0;
    qint64 filesize = file.size();
    qdebug << filesize;

    QTime time = QTime::currentTime();

    isrun = 1;

    while (ret < filesize) {
        if (!isrun) {
            qdebug << "取消编码";
            emit messgeString("取消编码");
            file.close();
            files.close();
            files.remove();
            return;
        }
        int _size = file.read(buff, readsize);

        if (_size < 0) {
            qDebug() << file.errorString();
            emit messgeString("编码失败");
            file.close();
            files.close();
            files.remove();
            return;
        }

        p = toBeastSoundStream((const char *)buff, _size, dict);

        if (!p) {
            emit messgeString("编码失败");
            file.close();
            files.close();
            files.remove();
            return;
        }

        files.write(p, strlen(p));
        free(p);

        ret += _size;

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = ret * 1.0 / filesize * 100;
            emit messgeString(QString("%1%,编码中：\n%2\n->\n%3\n").arg(step).
                              arg(QUrl::fromLocalFile(srcfile).path()).
                              arg(QUrl::fromLocalFile(dstfile).toString()));
        }
    }

    file.close();
    files.close();
    emit messgeString(QString("编码成功：\n%1\n->\n%2").
                      arg(QUrl::fromLocalFile(srcfile).path()).
                      arg(QUrl::fromLocalFile(dstfile).toString()));
}

void FTDE::frombeastSound(QString srcfile, QString dstfile)
{
    QFile file(srcfile);

    if (!file.open(QIODevice::ReadOnly)) {
        emit messgeString(QString("open失败:%1").arg(file.errorString()));
        return;
    }

    QFile files(dstfile);

    if (!files.open(QIODevice::WriteOnly)) {
        emit messgeString(QString("open失败:%1").arg(files.errorString()));
        file.close();
        return;
    }

    unsigned long dict_unicode[4];

    int    readsize = 1024;
    char   buff[1024 * 2];
    qint64 ret = 0;
    int    finish_len = 0;
    int    index = 0;
    bool   isstart = false;
    qint64 filesize = file.size();

    QTime time = QTime::currentTime();

    isrun = 1;

    while (ret < filesize) {
        if (!isrun) {
            emit messgeString("取消解码");
            file.close();
            files.close();
            files.remove();
            return;
        }

        if (finish_len >= readsize) {
            qdebug << finish_len;
            emit messgeString("解码失败");
            file.close();
            files.close();
            files.remove();
            return;
        }
        int _size = file.read(buff + finish_len, readsize - finish_len);

        if (_size < 0) {
            qDebug() << file.errorString();
            qdebug << finish_len;
            emit messgeString("解码失败");
            file.close();
            files.close();
            files.remove();
            return;
        }

        ret += _size;

        int datasize = _size + finish_len;

        if (isstart == false) {
            int finishsize = fromBeastSoundStreamStart(buff,
                                                       datasize,
                                                       &dict_unicode);

            if (finishsize < 0) {
                file.close();
                files.close();
                emit messgeString("解码失败");
                qdebug << finish_len;
                files.remove();
                return;
            }
            memmove(buff, buff + finishsize, datasize - finishsize);
            datasize -= finishsize;
            isstart = true;
        }
        int   finishsize;
        int   dstsize;
        char *p = fromBeastSoundStream(buff,
                                       datasize,
                                       dict_unicode,
                                       &dstsize,
                                       &finishsize,
                                       &index);

        // qdebug << datasize << _size << finishsize  << dstsize << finish_len;

        if (!p) {
            file.close();
            files.close();
            emit messgeString("解码失败");
            files.remove();
            return;
        }

        files.write(p, dstsize);
        free(p);
        finish_len = datasize - finishsize;

        memmove(buff, buff + finishsize, datasize - finishsize);

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = ret * 1.0 / filesize * 100;
            emit messgeString(QString("%1%,解码中：\n%2\n->\n%3").arg(step).
                              arg(QUrl::fromLocalFile(srcfile).path()).
                              arg(QUrl::fromLocalFile(dstfile).path()));
        }
    }

    file.close();
    files.close();
    emit messgeString(QString("解码成功：\n%1\n->\n%2").
                      arg(QUrl::fromLocalFile(srcfile).path()).
                      arg(QUrl::fromLocalFile(dstfile).path()));
}

void FTDE::encrpty(QString srcfile, QString dstfile)
{
    QFile file(srcfile);

    if (!file.open(QIODevice::ReadOnly)) {
        emit messgeString(QString("open失败:%1").arg(file.errorString()));
        return;
    }

    QFile files(dstfile);

    if (!files.open(QIODevice::WriteOnly)) {
        emit messgeString(QString("open失败:%1").arg(files.errorString()));
        file.close();
        return;
    }

    int    readsize = 1024;
    char   buff[1024 * 2];
    qint64 cnt = 0, ret;
    char   key = 0;
    qint64 filesize = file.size();

    QTime time = QTime::currentTime();

    isrun = 1;

    while (cnt < filesize) {
        if (!isrun) {
            emit messgeString("取消加密");
            file.close();
            files.close();
            files.remove();
            return;
        }
        ret = file.read(buff, readsize);

        if (ret < 0) {
            qDebug() << file.errorString();
            emit messgeString("加密失败");
            file.close();
            files.close();
            files.remove();
            return;
        }
        encryptbs(buff, ret, &key);

        if (files.write(buff, ret) != ret) {
            qDebug() << file.errorString();
            emit messgeString("write失败");
            file.close();
            files.close();
            files.remove();
        }
        cnt += ret;

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = cnt * 1.0 / filesize * 100;
            emit messgeString(QString("%1%,加密中：\n%2\n->\n%3").arg(step).
                              arg(QUrl::fromLocalFile(srcfile).path()).
                              arg(QUrl::fromLocalFile(dstfile).path()));
        }
    }
    file.close();
    files.close();
    emit messgeString(QString("加密成功：\n%1\n->\n%2").
                      arg(QUrl::fromLocalFile(srcfile).path()).
                      arg(QUrl::fromLocalFile(dstfile).path()));
}

void FTDE::decrpty(QString srcfile, QString dstfile)
{
    QFile file(srcfile);

    if (!file.open(QIODevice::ReadOnly)) {
        emit messgeString(QString("open失败:%1").arg(file.errorString()));
        return;
    }

    QFile files(dstfile);

    if (!files.open(QIODevice::WriteOnly)) {
        emit messgeString(QString("open失败:%1").arg(files.errorString()));
        file.close();
        return;
    }

    int    readsize = 1024;
    char   buff[1024 * 2];
    qint64 cnt = 0, ret = 0;
    char   key = 0;
    qint64 filesize = file.size();

    QTime time = QTime::currentTime();

    isrun = 1;

    while (cnt < filesize) {
        if (!isrun) {
            emit messgeString("取消解密");
            file.close();
            files.close();
            files.remove();
            return;
        }
        ret = file.read(buff, readsize);

        if (ret < 0) {
            qDebug() << file.errorString();
            emit messgeString("解密失败");
            file.close();
            files.close();
            files.remove();
            return;
        }
        decryptbs(buff, ret, &key);

        if (files.write(buff, ret) != ret) {
            qDebug() << file.errorString();
            emit messgeString("write失败");
            file.close();
            files.close();
            files.remove();
        }
        cnt += ret;

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = cnt * 1.0 / filesize * 100;
            emit messgeString(QString("%1%,解密中：\n%2\n->\n%3").arg(step).
                              arg(QUrl::fromLocalFile(srcfile).path()).
                              arg(QUrl::fromLocalFile(dstfile).path()));
        }
    }
    file.close();
    files.close();
    emit messgeString(QString("解密成功：\n%1\n->\n%2").
                      arg(QUrl::fromLocalFile(srcfile).path()).
                      arg(QUrl::fromLocalFile(dstfile).path()));
}

void FTDE::getDF(QString srcfile1, QString srcfile2, QString dictstr)
{
    QFile f1(srcfile1), f2(srcfile2), f3(dictstr);

    if (!f1.open(QIODevice::ReadOnly)) {
        QString str = QString("打开 %1 失败:%2").arg(f1.fileName())
                      .arg(f1.errorString());
        emit messgeString(str);
        qdebug << str;

        f1.setFileName(":/video/default.mp4");

        if (!f1.open(QIODevice::ReadOnly)) {
            QString str = QString("打开 %1 失败:%2").arg(f1.fileName())
                          .arg(f1.errorString());
            emit messgeString(str);
            qdebug << str;
            return;
        }
    }

    if (!f2.open(QIODevice::ReadOnly)) {
        QString str = QString("打开 %1 失败:%2").arg(f2.fileName()).arg(
            f2.errorString());
        emit messgeString(str);
        qdebug << str;
        f1.close();
        return;
    }

    if (!f3.open(QIODevice::WriteOnly)) {
        QString str = QString("打开 %1 失败:%2").arg(f3.fileName()).arg(
            f3.errorString());
        emit messgeString(str);
        qdebug << str;
        f1.close();
        f2.close();
        return;
    }

    qint64 cnt1 = f1.size();
    qint64 cnt2 = f2.size();
    qint64 csize = cnt1 + cnt2;
    qint64 nowsize = 0;
    qint64 cnt = 0;
    char   buff[1024];

    QTime time = QTime::currentTime();

    isrun = 1;

    while (cnt < cnt1) {
        if (!isrun) {
            emit messgeString("取消生成");
            f1.close();
            f2.close();
            f3.remove();
            return;
        }

        qint64 ret = f1.read(buff, sizeof(buff));

        if (ret < 0) {
            QString str = QString("read %1 失败:%2").arg(f1.fileName()).arg(
                f1.errorString());
            emit messgeString(str);
            f1.close();
            f2.close();
            f3.close();
            return;
        }

        f3.write(buff, ret);

        cnt += ret;
        nowsize += ret;

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = nowsize * 1.0 / csize * 100;
            emit messgeString(QString("%1%,处理中：\n%2 + %3 -> %4").arg(step).
                              arg(f1.fileName()).arg(f2.fileName()).arg(
                                  f3.fileName()));
        }
    }

    cnt = 0;

    while (cnt < cnt2) {
        if (!isrun) {
            emit messgeString("取消生成");
            f1.close();
            f2.close();
            f3.remove();
            return;
        }
        qint64 ret = f2.read(buff, sizeof(buff));

        if (ret < 0) {
            QString str = QString("read %1 失败:%2").arg(f2.fileName()).arg(
                f2.errorString());
            emit messgeString(str);
            f1.close();
            f2.close();
            f3.close();
            return;
        }

        f3.write(buff, ret);

        cnt += ret;
        nowsize += ret;

        if (time.msecsTo(QTime::currentTime()) > 1000) {
            time = QTime::currentTime();
            int  step = nowsize * 1.0 / csize * 100;
            emit messgeString(QString("%1%,处理中：\n%2 + %3 -> %4").arg(step).
                              arg(f1.fileName()).arg(f2.fileName()).arg(
                                  f3.fileName()));
        }
    }

    QString str = "生成成功：" + f1.fileName() + " + " + f2.fileName()
                  + "->" + f3.fileName();
    emit messgeString(str);
    qdebug << str;

    f1.close();
    f2.close();
    f3.close();
}
