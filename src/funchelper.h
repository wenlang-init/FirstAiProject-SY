#ifndef FUNCHELPER_H
#define FUNCHELPER_H

#include <QObject>
#include <QCryptographicHash>
#include <QPixmap>
#if defined(Q_OS_WINDOWS)
# include <qt_windows.h>
#endif // if defined(Q_OS_WINDOWS)

class FuncHelper : public QObject {
    Q_OBJECT

public:

    static FuncHelper& getInstance();

#if defined(Q_OS_WINDOWS)

    // 将标准输出重定向到控制台
    // 检测到从控制台启动，或onlyShow=true则启动控制台输出，否则无控制台输出
    // 在创建 QApplication 之前处理控制台
    static void AttachConsoleAndRedirect(bool onlyShow = false);
#endif // if defined(Q_OS_WINDOWS)
    void        getFileHashSig(QString filePath,
                               int     hash);

    // 现代 CPU 的 sqrtss / rsqrtss 指令（SSE）已经非常快，且精度更高，
    // 因此在大多数场景下，直接使用标准库函数是最佳选择
    // 快速逆平方根 1/sqrt(x) , number为非负数
    static float Q_rsqrt(float number) {
        uint32_t i;
        float    x2, y;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        y = number;

        // IEEE 754浮点数标准 , 对数近似值
        i = *(uint32_t  *)&y;

        // double : 0x5fe6eb50c7b537a9
        // float : 0x5f3759df
        i = 0x5f3759df - (i >> 1);
        y = *(float *)&i;

        // 使用牛顿-拉弗森方法（y = y*(1.5 - 0.5*x*y*y)）
        // 迭代一次即可获得相当高的精度（相对误差约 0.17%），
        // 迭代两次可达到约 1‰ 的精度
        y = y * (threehalfs - (x2 * y * y)); // 1st iteration
        // y  = y * (threehalfs - (x2 * y * y)); // 2nd iteration, can be
        // removed
        return y;
    }

protected:

    explicit FuncHelper(QObject *parent = nullptr);

public slots:

    void slotGetFileHash(QString filePath,
                         int     hash);
    void slotGetFileMD5(QString filePath);

private:

    bool isruningGetFileHASH = true;

public:

    void stopGetFileHASH() {
        if (isruningGetFileHASH) isruningGetFileHASH = false;
    }

    QString    getFileHash(const QString& filePath,
                           int            hash);
    QString    getDataHash(const QByteArray& data,
                           int               hash);
    QString    getFileMD5(const QString& filePath);
    QString    getDataMD5(const QByteArray& data);

    void       getRandomString(QString& sOut,
                               int      iNeedStrLong = 64);
    QByteArray executeCmd(const QString    & cmd,
                          const QStringList& args);
    void       execCmd(const QString    & cmd,
                       const QString    & workDir,
                       const QStringList& args);
    QByteArray executeBashCmd(const QString& strCmd);

    bool       killExec(const QString& sExename);

    void       getALLFilePath(const QString& sPath,
                              QStringList  & fileList,
                              bool           isRecursion = true);

#if defined(Q_OS_WINDOWS)
    struct WindowInfo {
        HWND    hwnd;
        DWORD   processID;
        QString className;
        QString windowName;
        QString filePathName;
    };

    // 根据类名 获取HWND,窗口
    HWND getWindowHWNDFromClassname(QString className,
                                    QRect & rect);

    // 根据窗口名 获取HWND,窗口
    HWND getWindowHWNDFromTitle(QString title,
                                QRect & rect);

    // 获取窗口rect
    QRect                        getRectFromHwnd(HWND wind);

    // 获取窗口rect,无边框
    QRect                        getRectNoBorderFromHwnd(HWND wind);

    // 根据HWND 获取进程ID
    DWORD                        getWindowProcessID(HWND wind);

    // 根据HWND 获取进程文件名(含路径)
    QString                      getWindowFilePathName(HWND wind);

    // 根据HWND 获取进程窗口名
    QString                      getWindowName(HWND wind);

    // 根据HWND 获取进程窗口类名
    QString                      getWindowClass(HWND wind);

    // 根据进程ID 获取显示的窗口的hwnd
    HWND                         getWindowByPID(DWORD pid);

    // 根据进程ID 获取显示的窗口的hwnd等信息
    QList<HWND>                  getWindowHwndList();

    // 根据进程ID 获取显示的窗口的hwnd等信息
    QList<FuncHelper::WindowInfo>getWindowInfoList();

    // 获取系统缩放比,系统放大倍数(在设置中的缩放比率)
    double                       getSystemZoom();

    // 物理分辨率与系统分辨率的比例()
    double                       getSystemPixZoom();

    // 获取wind的窗口到image

    bool   getScreenImage(HWND    wind,
                          QImage& image,
                          bool    hasBorder = true);
    QImage getWindowScreenImageFromDXGI(HWND hWnd);
    bool   getWindowScreenImageFromDXGI(QImage     & image,
                                        const QRect& rect);
#endif // if defined(Q_OS_WINDOWS)

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

    // 设置当前线程为最高优先级(pthread)
    bool setThreadPriorityToTopPriority(const QString& sThreadName);
    int  setProcessCPU(int pid,
                       int cpu);
    int  getProcessCPU(int pid);
    int  setCurrentProcessCPU(int cpu);
    int  getCurrentProcessCPU();
#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int  setThreadCPU(const void *tid,
                      int         cpu);
    int  getThreadCPU(const void *tid);
    int  setCurrentThreadCPU(int cpu);
    int  getCurrentThreadCPU();

    bool getLinkInfo(const QString& sfilename,
                     QString      & filename,
                     QPixmap      & pixmap);

signals:

    void sig_hashinfo(int value);
    void sig_hashfinished(const QString& md5);

    void sigGetFileHashPrivate(QString filePath,
                               int     hash);
};

///////////////////////////////////////////////////////////////////////////
/// 反射实现通过类名创建类对象
///////////////////////////////////////////////////////////////////////////

// 声明具有动态创建的基类
#define DECLEAR_DYNCRT_BASE(CBase)                                                \
public:                                                                           \
    typedef CBase *(*ClassGen)();                  /* 声明函数指针*/                    \
    static CBase *Create(const string& class_name) /* 工厂函数 */                     \
    {                                                                             \
        std::map<string, ClassGen>::iterator iter = m_class_set.find(class_name); \
        if (m_class_set.end() != iter)                                            \
        {                                                                         \
            return ((*iter).second)();                                            \
        }                                                                         \
        return NULL;                                                              \
    }                                                                             \
protected:                                                                        \
    static void Register(const string& class_name, ClassGen class_gen) /* 注册函数
                                                                        */ \
    {                                                                      \
        m_class_set.insert(map<string, ClassGen>::value_type(class_name,   \
                                                             class_gen));  \
    }                                                                      \
    static std::map<string, ClassGen> m_class_set; /* 存储子类信息 */

// 用于实现基类
#define IMPLEMENT_DYNCRT_BASE(CBase) \
    std::map<string, CBase::ClassGen>CBase::m_class_set;

// 用于声明一个能够被动态创建的类(用一个全局对象进行注册)
#define DECLEAR_DYNCRT_CLASS(CDerived, CBase)                               \
public:                                                                     \
    struct CDerived ## Register /* 辅助类，用于注册 */                              \
    {                                                                       \
        CDerived ## Register()                                              \
        {                                                                   \
            static bool bRegistered = false; /* 注册子类，保证唯一注册一次 */            \
            if (!bRegistered)                                               \
            {                                                               \
                CBase::Register(# CDerived, CDerived::Create); /* 注册子类信息 */ \
                bRegistered = true;                                         \
            }                                                               \
        }                                                                   \
    };                                                                      \
    static CBase *Create() /* 工厂函数 */                                       \
    {                                                                       \
        return new CDerived;                                                \
    }                                                                       \
    static struct CDerived ## Register m_t ## CDerived ## Register;

// 用于实现一个能被动态创建的类
#define IMPLEMENT_DYNCRT_CLASS(CDerived) \
    static CDerived::CDerived ## Register m_t ## CDerived ## Register;

/////////////////////////////使用方法/////////////////////////////////////
#if 0

# include <string>
# include <map>
# include <iostream>

using namespace std;

// 声明基类:
class CBase {
    DECLEAR_DYNCRT_BASE(CBase)
    DECLEAR_DYNCRT_CLASS(CBase, CBase)

public:

    virtual void Print()
    {
        std::cout << "This is base!" << std::endl;
    }
};
IMPLEMENT_DYNCRT_BASE(CBase)
IMPLEMENT_DYNCRT_CLASS(CBase)

// 声明继承类
class CDerived : public CBase {
    DECLEAR_DYNCRT_CLASS(CDerived, CBase)

public:

    virtual void Print()
    {
        cout << "This is derived!" << endl;
    }
};
IMPLEMENT_DYNCRT_CLASS(CDerived)

// 声明再继承类
class ExCDerived : public CDerived {
    DECLEAR_DYNCRT_CLASS(ExCDerived, CBase)

public:

    virtual void Print()
    {
        cout << "This is ExCDerived!" << endl;
    }
};
IMPLEMENT_DYNCRT_CLASS(ExCDerived)

int test()
{
    CBase *base = CBase::Create("CBase");

    if (base)
    {
        base->Print();
    }
    CBase *base2 = CBase::Create("CDerived");

    if (base2)
    {
        base2->Print();
    }
    CBase *base3 = CBase::Create("ExCDerived");

    if (base3)
    {
        base3->Print();
    }
    return 0;
}

#endif // if 0
#endif // FUNCHELPER_H
