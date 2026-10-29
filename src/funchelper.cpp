#include "funchelper.h"
#include <QFile>
#include <QCryptographicHash>
#include <QDateTime>
#include <QProcess>
#include <QTime>
#include <QEventLoop>
#include <QFileInfo>
#include <QDirIterator>
#include <QFileIconProvider>
#include "cxxlog.h"

FuncHelper& FuncHelper::getInstance()
{
    static FuncHelper funcHelper;

    return funcHelper;
}

void FuncHelper::getFileHashSig(QString filePath, int hash)
{
    emit sigGetFileHashPrivate(filePath, hash);
}

FuncHelper::FuncHelper(QObject *parent)
    : QObject{parent}
{
    connect(this,
            &FuncHelper::sigGetFileHashPrivate,
            this,
            &FuncHelper::slotGetFileHash);
}

#if defined(Q_OS_WINDOWS)
# include <windows.h>
# include <stdio.h>
void FuncHelper::AttachConsoleAndRedirect(bool onlyShow)
{
    // 尝试附加到父进程的控制台（如果是从cmd启动的，这会成功）
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // 重定向 stdout/stderr 到刚刚附加的控制台
        FILE *fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);

        // 使 C++ 标准流与 C 标准流同步，确保 qDebug/cout 能输出
        std::ios::sync_with_stdio();

        qDebug() << "检测到命令行环境，已附加到现有控制台。";
    } else {
        // 如果没有父控制台（双击运行），AttachConsole 会失败
        // 此时我们可以选择什么都不做（静默运行），或者手动创建一个新控制台用于调试

        if (onlyShow) {
            if (AllocConsole()) {
                FILE *fp;
                freopen_s(&fp, "CONOUT$", "w", stdout);
                freopen_s(&fp, "CONOUT$", "w", stderr);
                std::ios::sync_with_stdio();
                system("color 0");
                qDebug() << "总是显示输出模式：已创建新控制台。";
            }
        } else {
            // # ifndef QT_NO_DEBUG

            // if (AllocConsole()) {
            //     FILE *fp;
            //     freopen_s(&fp, "CONOUT$", "w", stdout);
            //     freopen_s(&fp, "CONOUT$", "w", stderr);
            //     std::ios::sync_with_stdio();
            //     system("color 0");
            //     qDebug() << "调试模式：已创建新控制台。";
            // }
            // # endif // ifndef QT_NO_DEBUG
        }
    }
}

#endif // if defined(Q_OS_WINDOWS)
void FuncHelper::slotGetFileHash(QString filePath, int bhash) {
    QString result;
    QFile   file(filePath);

    if (!file.exists()) {
        emit sig_hashfinished(result + " : " + file.errorString());
        return;
    }
    qint64 fileSize = file.size();
    qint64 fsize = fileSize;
    const qint64 bufferSize = 1024 * 500;

    if (file.open(QIODevice::ReadOnly))
    {
        char buffer[bufferSize];
        int  bytesRead = 0;
        int  readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash((QCryptographicHash::Algorithm)bhash);
        QTime time = QTime::currentTime();
        isruningGetFileHASH = true;

        QTime btime = time;

        // emit sig_md5info("");
        while (readSize > 0 && (bytesRead = file.read(buffer, readSize)) > 0)
        {
            if (isruningGetFileHASH == false)
            {
                file.close();
                isruningGetFileHASH = true;
                emit sig_hashfinished(result + " : 取消计算");
                return;
            }
            fileSize -= bytesRead;
            hash.addData(QByteArrayView(buffer, bytesRead));

            // hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);

            if (time.msecsTo(QTime::currentTime()) > 1000) {
                time = QTime::currentTime();
                int  infobl = (fsize - fileSize) * 100.0 / fsize;
                emit sig_hashinfo(infobl);
            }
        }

        file.close();
        result = QString(hash.result().toHex());
        emit sig_hashfinished(result + " 用时:" +
                              QString::number(btime.secsTo(QTime::currentTime()))
                              + "s");
    } else {
        emit sig_hashfinished(result + " : " + file.errorString());
        return;
    }
}

void FuncHelper::slotGetFileMD5(QString filePath)
{
    slotGetFileHash(filePath, QCryptographicHash::Md5);
}

QString FuncHelper::getFileHash(const QString& filePath, int bhash) {
    QString result;
    QFile   file(filePath);

    if (!file.exists()) {
        return result;
    }
    qint64 fileSize = file.size();
    const qint64 bufferSize = 1024 * 500;

    if (file.open(QIODevice::ReadOnly))
    {
        char buffer[bufferSize];
        int  bytesRead = 0;
        int  readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash((QCryptographicHash::Algorithm)bhash);

        while (readSize > 0 && (bytesRead = file.read(buffer, readSize)) > 0)
        {
            fileSize -= bytesRead;
            hash.addData(QByteArrayView(buffer, bytesRead));

            // hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        file.close();
        result = QString(hash.result().toHex());
    }
    return result;
}

QString FuncHelper::getDataHash(const QByteArray& data, int bhash) {
    QCryptographicHash hash((QCryptographicHash::Algorithm)bhash);

    hash.addData(QByteArrayView(data));
    return hash.result().toHex();
}

QString FuncHelper::getFileMD5(const QString& filePath)
{
    return getFileHash(filePath, QCryptographicHash::Md5);
}

QString FuncHelper::getDataMD5(const QByteArray& data)
{
    return getDataHash(data, QCryptographicHash::Md5);
}

void FuncHelper::getRandomString(QString& sOut, int iNeedStrLong)
{
    srand(QDateTime::currentMSecsSinceEpoch()); // 为随机值设定一个seed

    static const QString sChrs =
        "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    for (int i(0); i < iNeedStrLong; i++)
    {
        auto randomx = rand() % (sizeof(sChrs) - 1);
        sOut.append(sChrs[(int)randomx]);
    }
}

QByteArray FuncHelper::executeCmd(const QString& cmd, const QStringList& args)
{
    QByteArray strResult;
    QProcess   p;

    // connect(&p,SIGNAL(readyReadStandardOutput()),this,SLOT(readstanderoutput()));
    // connect(&p,SIGNAL(readyReadStandardError()),this,SLOT(readstandererror()));
    // connect(&p,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(errorOccurred(QProcess::ProcessError)));
    // connect(&p,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finished(int,QProcess::ExitStatus)));
    // connect(&p,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(stateChanged(QProcess::ProcessState)));


    p.setProgram(cmd);
    p.setArguments(args);
    p.start();

    if (!p.waitForFinished()) {
        strResult = p.readAllStandardError();
        return strResult;
    }

    strResult = p.readAllStandardOutput();
    return strResult;

#if 0
    {
        QEventLoop loop;

        QByteArray strResult;
        QProcess   p;

        // connect(&p,SIGNAL(readyReadStandardOutput()),this,SLOT(readstanderoutput()));
        // connect(&p,SIGNAL(readyReadStandardError()),this,SLOT(readstandererror()));
        // connect(&p,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(stateChanged(QProcess::ProcessState)));
        connect(&p, &QProcess::errorOccurred, this,
                [&](QProcess::ProcessError error) {
            qDebug() << "Process error:" << error;
            strResult = p.readAllStandardError();
            loop.quit();
        });
        connect(&p, &QProcess::finished, this,
                [&](int exitCode,
                    QProcess::ExitStatus exitStatus) {
            qDebug() << "Process finished with exit code:" << exitCode
                     << "and exit status:" << exitStatus;
            strResult = p.readAllStandardOutput();
            loop.quit();
        });

        p.setProgram(cmd);
        p.setArguments(args);
        p.start();

        loop.exec();
        return strResult;
    }
#endif // if 0
}

void FuncHelper::execCmd(const QString    & cmd,
                         const QString    & workDir,
                         const QStringList& args)
{
    QProcess *p = new QProcess;

    connect(p, &QProcess::finished, this, [ = ]() {
        QProcess *p = (QProcess *)sender();
        delete p;
    });
    connect(p, &QProcess::errorOccurred, this,
            [ = ](QProcess::ProcessError error) {
        QProcess *p = (QProcess *)sender();
        delete p;
    });
    connect(p, &QProcess::readyReadStandardOutput, this, [ = ]() {
        QProcess *p = (QProcess *)sender();
        qDebug().noquote() <<
            QString::fromLocal8Bit(p->readAllStandardOutput());
    });
    connect(p, &QProcess::readyReadStandardError, this, [ = ]() {
        QProcess *p = (QProcess *)sender();
        qCritical().noquote() <<
            QString::fromLocal8Bit(p->readAllStandardError());
    });

    // connect(&p,SIGNAL(readyReadStandardOutput()),this,SLOT(readstanderoutput()));
    // connect(&p,SIGNAL(readyReadStandardError()),this,SLOT(readstandererror()));
    // connect(&p,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(errorOccurred(QProcess::ProcessError)));
    // connect(&p,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finished(int,QProcess::ExitStatus)));
    // connect(&p,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(stateChanged(QProcess::ProcessState)));

    p->setProgram(cmd);
    p->setWorkingDirectory(workDir);
    p->setArguments(args);
    p->start();
}

QByteArray FuncHelper::executeBashCmd(const QString& strCmd)
{
    QByteArray strResult;
    QProcess   p;

    p.start("bash", QStringList() << "-c" << strCmd);

    if (!p.waitForFinished()) {
        // p.readAllStandardError();
        return strResult;
    }

    strResult = p.readAllStandardOutput();

    return strResult;
}

bool FuncHelper::killExec(const QString& sExename)
{
#ifdef __WIN32
    QStringList strlist;
    strlist << "/FI" << QString("imagename eq %1").arg(sExename);

    // tasklist /FI "imagename eq a.exe"
    // taskkill -im a.exe -f
    // taskkill /PID 12345 -f
    QProcess p;
    p.start("tasklist", strlist);

    if (!p.waitForFinished()) {
        return false;
    }
    QByteArray rst = p.readAllStandardOutput();

    if (rst.isEmpty()) {
        return false;
    }
    QString rstlocal = QString::fromLocal8Bit(rst);

    if (rstlocal.contains(sExename)) {
        // taskkill -im a.exe -f
        QString cmd = "taskkill -im ";
        cmd += sExename + " -f";
        p.execute(cmd);

        // p.start(cmd);p.waitForFinished()
        // p.readAllStandardOutput();
        // p.readAllStandardError();
    }
    p.close();
#else // ifdef s__WIN32
    QStringList strlist;
    QProcess    p;
    strlist << sExename;
    p.start("pidof", strlist);
    p.waitForFinished();

    if (!p.waitForFinished()) {
        return false;
    }
    QByteArray rst = p.readAllStandardOutput();

    if (rst.isEmpty()) {
        return false;
    }
    QString rstlocal = QString::fromLocal8Bit(rst);

    QStringList lpid = rstlocal.split(" ");

    for (int i = 0; i < lpid.size(); i++) {
        QString cmd = "kill -9 ";
        cmd += lpid[i];
        p.execute(cmd);

        // p.readAllStandardOutput();
        // p.readAllStandardError();
    }
    p.close();
#endif // ifdef __WIN32
    return true;
}

void FuncHelper::getALLFilePath(const QString& sPath,
                                QStringList  & fileList,
                                bool           isRecursion)
{
    QDirIterator *it;

    if (isRecursion) {
        it = new QDirIterator(sPath, QDirIterator::Subdirectories); // 支持递归遍历
    } else {
        it = new QDirIterator(sPath, QDirIterator::NoIteratorFlags);
    }

    while (it->hasNext()) {
        fileList.append(it->next());
    }
    delete it;
}

#if defined(Q_OS_WINDOWS)
# include <psapi.h>

// 根据类名 获取HWND,窗口
HWND FuncHelper::getWindowHWNDFromClassname(QString className, QRect& rect)
{
    HWND hwnd = FindWindowA(className.toLocal8Bit(), NULL);

    // FindWindowW((LPCWSTR)className.utf16(), NULL);

    // if(!IsWindowVisible(hwnd))return 0;
    RECT mrect;

    // 获得控件相对与对话框或者（设备屏幕）的坐标大小
    if (GetWindowRect(hwnd, &mrect)) {
        int w, h;
        w = mrect.right - mrect.left;
        h = mrect.bottom - mrect.top;
        rect.setRect(mrect.left, mrect.top, w, h);
    }
    return hwnd;
}

// 根据窗口名 获取HWND,窗口
HWND FuncHelper::getWindowHWNDFromTitle(QString title, QRect& rect)
{
    HWND hwnd = FindWindowA(NULL, title.toLocal8Bit());
    RECT mrect;

    // if(!IsWindowVisible(hwnd))return 0;
    if (GetWindowRect(hwnd, &mrect)) {
        int w, h;
        w = mrect.right - mrect.left;
        h = mrect.bottom - mrect.top;
        rect.setRect(mrect.left, mrect.top, w, h);
    }
    return hwnd;
}

QRect FuncHelper::getRectFromHwnd(HWND wind)
{
    QRect rect;

    // if(!IsWindowVisible(hwnd))return rect;
    RECT mrect;

    if (GetWindowRect(wind, &mrect)) {
        int w, h;
        w = mrect.right - mrect.left;
        h = mrect.bottom - mrect.top;
        rect.setRect(mrect.left, mrect.top, w, h);
    }
    return rect;
}

// 获取窗口rect,无边框
QRect FuncHelper::getRectNoBorderFromHwnd(HWND wind)
{
    // if(!IsWindowVisible(hwnd))return rect;
    QRect rect;

    RECT mrect1, mrect2;

    if (!GetWindowRect(wind, &mrect1)) {
        return rect;
    }

    if (!GetClientRect(wind, &mrect2)) {
        return rect;
    }
    int x, y, w, h, borderx, bordery;
    x = mrect1.left;
    y = mrect1.top;
    borderx = mrect1.right - mrect1.left - mrect2.right;
    bordery = mrect1.bottom - mrect1.top - mrect2.bottom;
    w = mrect2.right;
    h = mrect2.bottom;

    x = x + borderx / 2;
    y = y + bordery - borderx / 2;

    rect.setRect(x, y, w, h);

    return rect;
}

// 根据HWND 获取进程ID
DWORD FuncHelper::getWindowProcessID(HWND wind)
{
    DWORD process_ID = 0;

    GetWindowThreadProcessId(wind, &process_ID);

    return process_ID;
}

// 根据HWND 获取进程文件名(含路径)
QString FuncHelper::getWindowFilePathName(HWND wind)
{
    QString str;
    char    pszFileName[4096] = { 0 };
    DWORD   process_ID = 0;
    BOOL    ERR = GetWindowThreadProcessId(wind, &process_ID);

    if (!ERR) return str;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                  FALSE,
                                  process_ID);

    if (!hProcess) return str;

    int ret = GetModuleFileNameExA(hProcess, NULL, (LPSTR)pszFileName,
                                   sizeof(pszFileName));

    if (!ret) return str;

    str = QString::fromLocal8Bit(pszFileName);
    str.replace("\\", "/");
    return str;
}

// 根据HWND 获取进程窗口名
QString FuncHelper::getWindowName(HWND wind)
{
    QString str;
    char    pszFileName[4096] = { 0 };

    GetWindowTextA(wind, (LPSTR)pszFileName,
                   sizeof(pszFileName));
    str = QString::fromLocal8Bit(pszFileName);
    return str;
}

// 根据HWND 获取进程窗口类名
QString FuncHelper::getWindowClass(HWND wind)
{
    QString str;
    char    pszFileName[4096] = { 0 };

    GetClassNameA(wind, (LPSTR)pszFileName,
                  sizeof(pszFileName));
    str = QString::fromLocal8Bit(pszFileName);
    return str;
}

struct FindWindowData {
    DWORD processId;
    HWND  hWnd;
};

// 根据进程ID 获取显示的窗口的hwnd
HWND FuncHelper::getWindowByPID(DWORD pid)
{
    FindWindowData data{ pid, nullptr };
    auto EnumWindowsProc =
        [](HWND hWnd, LPARAM lParam) {
            DWORD pid = 0;

            GetWindowThreadProcessId(hWnd, &pid);
            auto data =
                reinterpret_cast<FindWindowData *>(lParam);

            if ((pid == data->processId) &&
                IsWindowVisible(hWnd)) {
                data->hWnd = hWnd;
                return FALSE; // 找到后停止枚举
            }
            return TRUE;
        };

    // 枚举系统中所有顶级窗口，并将每个窗口的句柄传递给用户定义的回调函数
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

    // if (data.hWnd) {
    //     ShowWindow(data.hWnd, SW_MAXIMIZE);
    //     SetForegroundWindow(data.hWnd);
    // }

    return data.hWnd;
}

// 根据进程ID 获取显示的窗口的hwnd等信息
QList<HWND>FuncHelper::getWindowHwndList()
{
    QList<HWND> list;
    auto EnumWindowsProc =
        [](HWND hWnd, LPARAM lParam) {
            DWORD pid = 0;

            GetWindowThreadProcessId(hWnd, &pid);
            auto data =
                reinterpret_cast<QList<HWND> *>(lParam);

            if (IsWindowVisible(hWnd)) {
                data->append(hWnd);

                // return FALSE; // 停止枚举
            }
            return TRUE;
        };

    // 枚举系统中所有顶级窗口，并将每个窗口的句柄传递给用户定义的回调函数
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&list));

    return list;
}

// 根据进程ID 获取显示的窗口的hwnd等信息
QList<FuncHelper::WindowInfo>FuncHelper::getWindowInfoList()
{
    QList<FuncHelper::WindowInfo> list;
    auto EnumWindowsProc =
        [](HWND hWnd, LPARAM lParam) {
            DWORD pid = 0;

            GetWindowThreadProcessId(hWnd, &pid);
            auto data =
                reinterpret_cast<QList<FuncHelper::WindowInfo> *>(lParam);

            if (IsWindowVisible(hWnd)) {
                WindowInfo winfo;
                winfo.hwnd = hWnd;
                winfo.processID = FuncHelper::getInstance().
                                  getWindowProcessID(hWnd);
                winfo.className = FuncHelper::getInstance().
                                  getWindowClass(hWnd);
                winfo.windowName = FuncHelper::getInstance().
                                   getWindowName(hWnd);
                winfo.filePathName = FuncHelper::getInstance().
                                     getWindowFilePathName(hWnd);
                data->append(winfo);

                // return FALSE; // 停止枚举
            }
            return TRUE;
        };

    // 枚举系统中所有顶级窗口，并将每个窗口的句柄传递给用户定义的回调函数
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&list));

    return list;
}

double FuncHelper::getSystemZoom()
{
    HWND hwd = ::GetDesktopWindow();

    // HDC hdc = ::GetDC(hwd);
    // // 每英寸像素点数
    // int zoom = GetDeviceCaps(hdc, LOGPIXELSX);
    int zoom = GetDpiForWindow(hwd);
    double dpi = 1;

    switch (zoom) {
    case 96:
        dpi = 1;
        break;

    case 120:
        dpi = 1.25;
        break;

    case 144:
        dpi = 1.5;
        break;

    case 192:
        dpi = 2;
        break;

    default:
        break;
    }
    return dpi;
}

double FuncHelper::getSystemPixZoom()
{
    // 获取窗口当前显示的监视器
    HWND hWnd = GetDesktopWindow();
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

    // 获取监视器逻辑宽度
    MONITORINFOEX monitorInfo;

    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(hMonitor, &monitorInfo);
    int cxLogical = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);

    // 获取监视器物理宽度
    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    int cxPhysical = dm.dmPelsWidth;

    return cxPhysical * 1.0 / cxLogical;
}

// 获取wind的窗口到image
bool FuncHelper::getScreenImage(HWND wind, QImage& image, bool hasBorder)
{
    RECT   rect;
    int    m_width;
    int    m_height;
    double zoom = getSystemPixZoom();

    if (!hasBorder) {
        // 获得控件相自身的坐标大小，获取的区域不包含边框，top和left都为0
        if (GetClientRect(wind, &rect)) {
            m_width = rect.right * zoom;
            m_height = rect.bottom * zoom;
        } else {
            // return false;
            m_width = GetSystemMetrics(SM_CXSCREEN) * zoom;
            m_height = GetSystemMetrics(SM_CYSCREEN) * zoom;
        }
    } else {
        // 获得控件相对与对话框或者（设备屏幕）的坐标大小，获取的区域包含边框
        if (GetWindowRect(wind, &rect)) {
            m_width = (rect.right - rect.left) * zoom;
            m_height = (rect.bottom - rect.top) * zoom;
        } else {
            m_width = GetSystemMetrics(SM_CXSCREEN) * zoom;
            m_height = GetSystemMetrics(SM_CYSCREEN) * zoom;
        }
    }
    HDC hdcScreen = GetDC(wind);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // 创建位图
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, m_width, m_height);
    SelectObject(hdcMem, hBitmap);

    // 开始捕获，获取捕获数据
# if 0

    // 通过向目标窗口发送 WM_PRINT 或 WM_PRINTCLIENT 消息，让窗口自身将内容绘制到指定的 DC。
    // 这个过程在目标应用程序内部完成，因此能够捕获窗口的真实绘制内容，
    // 包括 DWM（桌面窗口管理器）效果和部分复杂控件
    // PrintWindow 是同步阻塞调用，如果目标窗口未响应消息，调用可能会挂起
    PrintWindow(wind, hdcMem, PW_CLIENTONLY);
# else // if 0
    // 源设备上下文（DC）复制像素到目标 DC
    // 它完全在调用线程中执行，不依赖目标窗口的绘制逻辑，因此对隐藏窗口或桌面窗口通常有效，
    // 但对于使用 透明度、双缓冲或硬件加速（如DirectX/OpenGL） 的窗口，
    // 可能会出现黑屏或无法正确捕获的情况
    BitBlt(hdcMem, rect.left, rect.top, rect.right, rect.bottom,
           hdcScreen, 0, 0, SRCCOPY);
# endif // if 0
    QVector<unsigned char> m_screenshotData(m_width * m_height * 4, 0);
    GetBitmapBits(hBitmap,
                  m_screenshotData.size(),
                  m_screenshotData.data());
    image = QImage(m_screenshotData.data(), m_width, m_height
                   , QImage::Format_ARGB32).copy();

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(wind, hdcScreen);
    return true;
}

////////////DXGI截屏/////////////////
# include <d3d11.h>
# include <dxgi1_2.h>
# include <comdef.h> // 用于 COM 错误处理
# pragma comment(lib, "d3d11.lib")
# pragma comment(lib, "dxgi.lib")
# pragma comment(lib, "User32.lib")


/**
 * 截取指定窗口并保存为 QImage
 * @param hWnd 目标窗口句柄
 * @return 截取到的 QImage，失败则返回空 QImage
 */
QImage FuncHelper::getWindowScreenImageFromDXGI(HWND hWnd)
{
    HRESULT hr = S_OK;
    ID3D11Device *pDevice = nullptr;
    ID3D11DeviceContext *pContext = nullptr;
    IDXGIOutputDuplication *pDuplication = nullptr;

    // 1. 初始化 D3D11 设备和上下文
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &pDevice,
        nullptr,
        &pContext
        );

    if (FAILED(hr)) {
        qDebug() << "D3D11CreateDevice failed:" << hr;
        return QImage();
    }

    // 2. 获取 DXGI 设备
    IDXGIDevice *pDxgiDevice = nullptr;
    hr = pDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDxgiDevice);

    if (FAILED(hr)) {
        qDebug() << "QueryInterface for IDXGIDevice failed:" << hr;
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 3. 获取 DXGI 适配器（显卡）
    IDXGIAdapter *pAdapter = nullptr;
    hr = pDxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pAdapter);
    pDxgiDevice->Release();

    if (FAILED(hr)) {
        qDebug() << "GetParent for IDXGIAdapter failed:" << hr;
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 4. 获取主显示器输出
    IDXGIOutput *pOutput = nullptr;
    hr = pAdapter->EnumOutputs(0, &pOutput);
    pAdapter->Release();

    if (FAILED(hr)) {
        qDebug() << "EnumOutputs failed:" << hr;
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 5. 获取 IDXGIOutput1 接口
    IDXGIOutput1 *pOutput1 = nullptr;
    hr = pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void **)&pOutput1);
    pOutput->Release();

    if (FAILED(hr)) {
        qDebug() << "QueryInterface for IDXGIOutput1 failed:" << hr;
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 6. 创建桌面复制对象
    hr = pOutput1->DuplicateOutput(pDevice, &pDuplication);
    pOutput1->Release();

    if (FAILED(hr)) {
        qDebug() << "DuplicateOutput failed:" << hr;
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 7. 获取目标窗口的位置和尺寸
    RECT windowRect;
    int  left = 0;
    int  top = 0;
    int  width = GetSystemMetrics(SM_CXSCREEN);
    int  height = GetSystemMetrics(SM_CYSCREEN);

    if (!GetWindowRect(hWnd, &windowRect)) {
        // qDebug() << "GetWindowRect failed";
        // pDuplication->Release();
        // pDevice->Release();
        // pContext->Release();
        // return QImage();
    } else {
        left = windowRect.left;
        top = windowRect.top;
        width = windowRect.right - windowRect.left;
        height = windowRect.bottom - windowRect.top;
    }

    // 如果窗口大小为 0 或负值，则返回空
    if ((width <= 0) || (height <= 0)) {
        qDebug() << "Invalid window size";
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 8. 捕获一帧桌面图像
    IDXGIResource *pDesktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    hr = pDuplication->AcquireNextFrame(500, &frameInfo, &pDesktopResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        qDebug() << "AcquireNextFrame timeout";
        pDuplication->ReleaseFrame();
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    if (FAILED(hr)) {
        qDebug() << "AcquireNextFrame failed:" << hr;
        pDuplication->ReleaseFrame();
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 9. 从资源获取纹理
    ID3D11Texture2D *pDesktopTexture = nullptr;
    hr = pDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D),
                                          (void **)&pDesktopTexture);
    pDesktopResource->Release();

    if (FAILED(hr)) {
        qDebug() << "QueryInterface for ID3D11Texture2D failed:" << hr;
        pDuplication->ReleaseFrame();
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 10. 获取桌面纹理描述
    D3D11_TEXTURE2D_DESC desc;
    pDesktopTexture->GetDesc(&desc);

    // 11. 创建用于 CPU 读取的 Staging 纹理
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.BindFlags = 0;
    stagingDesc.MiscFlags = 0;

    ID3D11Texture2D *pStagingTexture = nullptr;
    hr = pDevice->CreateTexture2D(&stagingDesc, nullptr, &pStagingTexture);

    if (FAILED(hr)) {
        qDebug() << "CreateTexture2D for staging failed:" << hr;
        pDesktopTexture->Release();
        pDuplication->ReleaseFrame();
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 12. 将桌面纹理拷贝到 Staging 纹理
    pContext->CopyResource(pStagingTexture, pDesktopTexture);
    pDesktopTexture->Release();

    // 13. 映射 Staging 纹理以读取像素数据
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = pContext->Map(pStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    if (FAILED(hr)) {
        qDebug() << "Map failed:" << hr;
        pStagingTexture->Release();
        pDuplication->ReleaseFrame();
        pDuplication->Release();
        pDevice->Release();
        pContext->Release();
        return QImage();
    }

    // 14. 从映射的数据中裁剪出窗口区域
    // 桌面数据格式为 DXGI_FORMAT_B8G8R8A8_UNORM (32位 BGRA)[reference:4][reference:5]
    const int bytesPerPixel = 4;
    QImage    result(width, height, QImage::Format_ARGB32);

    // mapped.RowPitch 是桌面图像的每行字节数（含填充）
    // 从桌面数据中提取窗口区域
    for (int y = 0; y < height; ++y) {
        // 计算窗口区域在桌面数据中的起始地址
        const quint8 *srcRow = static_cast<const quint8 *>(mapped.pData) +
                               (top + y) * mapped.RowPitch +
                               left * bytesPerPixel;

        // 获取 QImage 当前行的写入指针
        quint8 *dstRow = result.scanLine(y);

        // 复制一行像素数据（BGRA -> ARGB 转换）
        for (int x = 0; x < width; ++x) {
            const quint8 *srcPixel = srcRow + x * bytesPerPixel;
            quint8 *dstPixel = dstRow + x * bytesPerPixel;

            // BGRA 转换为 ARGB (Qt 使用 ARGB32 格式)
            dstPixel[0] = srcPixel[2]; // R
            dstPixel[1] = srcPixel[1]; // G
            dstPixel[2] = srcPixel[0]; // B
            dstPixel[3] = srcPixel[3]; // A
        }
    }

    // 15. 清理资源
    pContext->Unmap(pStagingTexture, 0);
    pStagingTexture->Release();
    pDuplication->ReleaseFrame();
    pDuplication->Release();
    pDevice->Release();
    pContext->Release();

    return result;
}

bool FuncHelper::getWindowScreenImageFromDXGI(QImage& image, const QRect& rect)
{
    // 支持的驱动程序类型
    D3D_DRIVER_TYPE DriverTypes[] =
    { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // 支持的功能级别
    D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1 };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    // 创建D3D设备
    D3D_FEATURE_LEVEL FeatureLevel;
    ID3D11Device     *_pDX11Dev = nullptr;
    ID3D11DeviceContext *_pDX11DevCtx = nullptr;
    HRESULT hr = 0;

    for (UINT index = 0; index < NumDriverTypes; index++) {
        hr = D3D11CreateDevice(nullptr,
                               DriverTypes[index],
                               nullptr,
                               0,
                               FeatureLevels,
                               NumFeatureLevels,
                               D3D11_SDK_VERSION,
                               &_pDX11Dev,
                               &FeatureLevel,
                               &_pDX11DevCtx);

        if (SUCCEEDED(hr)) break;
    }

    // 获取DXGITest设备
    IDXGIDevice *_pDXGITestDev = nullptr;
    hr = _pDX11Dev->QueryInterface(__uuidof(IDXGIDevice),
                                   reinterpret_cast<void **>(&_pDXGITestDev));

    if (FAILED(hr)) {
        return false;
    }

    // 获取DXGITest适配器
    IDXGIAdapter *_pDXGITestAdapter = nullptr;
    hr = _pDXGITestDev->GetParent(__uuidof(IDXGIAdapter),
                                  reinterpret_cast<void **>(&_pDXGITestAdapter));

    if (FAILED(hr)) {
        return false;
    }

    // 获取输出
    UINT i = 0;
    IDXGIOutput *_pDXGIOutput = nullptr;
    hr = _pDXGITestAdapter->EnumOutputs(i, &_pDXGIOutput);

    if (FAILED(hr)) {
        return false;
    }

    // 获取输出描述结构
    DXGI_OUTPUT_DESC DesktopDesc;
    _pDXGIOutput->GetDesc(&DesktopDesc);

    // 请求接口给Output1
    IDXGIOutput1 *_pDXGIOutput1 = nullptr;
    hr =
        _pDXGIOutput->QueryInterface(__uuidof(IDXGIOutput1),
                                     reinterpret_cast<void **>(&_pDXGIOutput1));

    if (FAILED(hr)) {
        return false;
    }

    // 创建桌面副本
    IDXGIOutputDuplication *_pDXGIOutputDup = nullptr;
    hr = _pDXGIOutput1->DuplicateOutput(_pDX11Dev, &_pDXGIOutputDup);

    if (FAILED(hr)) {
        return false;
    }

    for (int i = 0; i < 100; i++) {
        IDXGIResource *desktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        hr = _pDXGIOutputDup->AcquireNextFrame(20, &frameInfo, &desktopResource);

        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
                if (desktopResource) {
                    desktopResource->Release();
                    desktopResource = nullptr;
                    qWarning() << " AcquireNextFrame超时";
                    --i;
                    continue;
                }
                _pDXGIOutputDup->ReleaseFrame();
            } else {
                qCritical() << " AcquireNextFrame错误: " << hr;
                return false;
            }
        }

        if (desktopResource == nullptr) {
            qWarning() << " AcquireNextFrame超时";
            --i;
            continue;
        }

        // 查询下一帧暂存缓冲区
        ID3D11Texture2D *_pDX11Texture = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(
                                                 ID3D11Texture2D),
                                             reinterpret_cast<void **>(&
                                                                       _pDX11Texture));
        desktopResource->Release();
        desktopResource = nullptr;

        if (FAILED(hr)) {
            return false;
        }

        // 复制旧描述
        ID3D11Texture2D *_pCopyBuffer = nullptr;
        D3D11_TEXTURE2D_DESC desc;

        if (_pDX11Texture) {
            _pDX11Texture->GetDesc(&desc);
        } else if (_pCopyBuffer) {
            _pCopyBuffer->GetDesc(&desc);
        } else {
            qCritical() << " GetDesc错误";
            return false;
        }

        // 为填充帧图像创建一个新的暂存缓冲区
        if (_pCopyBuffer == nullptr) {
            D3D11_TEXTURE2D_DESC CopyBufferDesc;
            CopyBufferDesc.Width = desc.Width;
            CopyBufferDesc.Height = desc.Height;
            CopyBufferDesc.MipLevels = 1;
            CopyBufferDesc.ArraySize = 1;
            CopyBufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            CopyBufferDesc.SampleDesc.Count = 1;
            CopyBufferDesc.SampleDesc.Quality = 0;
            CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
            CopyBufferDesc.BindFlags = 0;
            CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            CopyBufferDesc.MiscFlags = 0;

            hr = _pDX11Dev->CreateTexture2D(&CopyBufferDesc,
                                            nullptr,
                                            &_pCopyBuffer);

            if (FAILED(hr)) {
                qCritical() << " CreateTexture2D错误: " << hr;
                return false;
            }
        }

        if (_pDX11Texture) {
            // 将下一个暂存缓冲区复制到新的暂存缓冲区
            _pDX11DevCtx->CopyResource(_pCopyBuffer, _pDX11Texture);
        }

        // 为映射位创建暂存缓冲区
        IDXGISurface *CopySurface = nullptr;
        hr = _pCopyBuffer->QueryInterface(__uuidof(IDXGISurface),
                                          (void **)&CopySurface);

        if (FAILED(hr)) {
            qCritical() << " QueryInterface错误: " << hr;
            return false;
        }

        // 将位复制到用户空间
        DXGI_MAPPED_RECT MappedSurface;
        hr = CopySurface->Map(&MappedSurface, DXGI_MAP_READ);

        if (hr >= 0) {
            // 获取屏幕尺寸
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            image = QImage(MappedSurface.pBits,
                           screenWidth,
                           screenHeight,
                           QImage::Format_ARGB32
                           ).copy(rect);
        }

        qDebug() << " 完成: " << i;

        CopySurface->Unmap();
        CopySurface->Release();
        CopySurface = nullptr;

        if (_pDXGIOutputDup) {
            _pDXGIOutputDup->ReleaseFrame();
        }
    }
    return true;
}

/////////////////////////////

#endif // if defined(Q_OS_WINDOWS)

#include <stdio.h>
#if defined(Q_OS_WINDOWS)
# include <windows.h>
# include <winbase.h>
#endif // if defined(Q_OS_WINDOWS)
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
# include <pthread.h>
# include <sched.h>
# include <unistd.h>
#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

bool FuncHelper::setThreadPriorityToTopPriority(const QString& sThreadName)
{
    pthread_attr_t attr; // 创建一个用于存储线程属性的结构体

    pthread_attr_init(&attr);

    int policy = SCHED_FIFO;
    pthread_t threadId = pthread_self();               // 获取当前线程的ID
    pthread_attr_setschedpolicy(&attr, policy);        // 设置线程调度策略为实时FIFO

    int iMaxPriority = sched_get_priority_max(policy); // 获取当前系统最大的优先级值

    struct sched_param param;                          // 创建一个sched_param结构体设置线程优先级
    pthread_attr_getschedparam(&attr, &param);         // 获取默认的线程属性
    auto iNowPriority = param.sched_priority;
    param.sched_priority = iMaxPriority;               // 设置线程的优先级为最高优先级
    pthread_setschedparam(threadId, policy, &param);   // 将线程的调度参数和优先级应用到线程上
    pthread_attr_destroy(&attr);                       // 销毁线程属性
    INFO_LOG_CXX(
        " ************ thread:0x%x_%lu, name:%s setPriority to topPriority:%d => %d ************\n",
        threadId,
        threadId,
        sThreadName.toStdString().c_str(),
        iNowPriority,
        iMaxPriority);
    return true;
}

int FuncHelper::setProcessCPU(int pid, int cpu)
{
    int cpukercount = sysconf(_SC_NPROCESSORS_ONLN); // CPU核心总数

    if (cpu >= cpukercount) cpu = cpukercount - 1;
    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    // pid == 0 表示当前进程
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &mask) < 0) {
        perror("sched_setaffinity");
        return -1;
    }
    return 0;
}

int FuncHelper::getProcessCPU(int pid)
{
    int cpukercount = sysconf(_SC_NPROCESSORS_ONLN); // CPU核心总数
    cpu_set_t mask;

    CPU_ZERO(&mask);

    if (sched_getaffinity(pid, sizeof(mask), &mask) < 0) {
        perror("sched_getaffinity");
        return -1;
    }

    for (int i = 0; i < cpukercount; i++)
        if (CPU_ISSET(i, &mask)) return i;

    return -1;
}

int FuncHelper::setCurrentProcessCPU(int cpu)
{
    int pid = 0; // getpid();

    return setProcessCPU(pid, cpu);
}

int FuncHelper::getCurrentProcessCPU()
{
    int pid = 0; // getpid();

    return getProcessCPU(pid);
}

#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)

int FuncHelper::setThreadCPU(const void *tid, int cpu)
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int threadid = *(int *)tid;
    int cpukercount = sysconf(_SC_NPROCESSORS_ONLN); // CPU核心总数

    if (cpu >= cpukercount) return -1;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    if (pthread_setaffinity_np(threadid, sizeof(mask), &mask) < 0) {
        perror("pthread_setaffinity_np");
        return -1;
    }
    return 0;

#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if defined Q_OS_WINDOWS
    HANDLE thread = *(HANDLE *)tid;

    if (SetThreadAffinityMask(thread, 1 << cpu) == 0) {
        perror("SetThreadAffinityMask");
        return -1;
    }
    return 0;

#endif // if defined Q_OS_WINDOWS
    Q_UNUSED(tid)
    Q_UNUSED(cpu)
    return -1;
}

int FuncHelper::getThreadCPU(const void *tid)
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int threadid = *(int *)tid;
    int cpukercount = sysconf(_SC_NPROCESSORS_ONLN); // CPU核心总数
    cpu_set_t mask;
    CPU_ZERO(&mask);

    if (pthread_getaffinity_np(threadid, sizeof(mask), &mask) < 0) {
        perror("pthread_getaffinity_np");
        return -1;
    }

    for (int i = 0; i < cpukercount; i++)
        if (CPU_ISSET(i, &mask)) return i;

    return -1;

#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if 0 // defined Q_OS_WINDOWS
    GetCurrentProcessorNumber();
    HANDLE   thread = *(HANDLE *)tid;
    uint64_t threadAffinityMask;

    if (GetThreadAffinityMask(thread,
                              &threadAffinityMask) != c_partyErrorSuccess) {
        // GetErrorMessage(errcode, errmsg, sizeof(errmsg));
        perror("GetThreadAffinityMask");
        return -1;
    }

    for (uint64_t i = 0; i < sizeof(threadAffinityMask) * 8; i++) {
        if (threadAffinityMask & (1 << i)) {
            return i;
        }
    }
    return -1;

#endif // if defined Q_OS_WINDOWS
    Q_UNUSED(tid)
    return -1;
}

int FuncHelper::setCurrentThreadCPU(int cpu)
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int tid = pthread_self(); // syscall(SYS_gettid);
    return setThreadCPU(&tid, cpu);

#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if defined Q_OS_WINDOWS
    HANDLE tid = GetCurrentThread();
    return setThreadCPU(&tid, cpu);

#endif // if defined Q_OS_WINDOWS
    Q_UNUSED(cpu)
    return -1;
}

int FuncHelper::getCurrentThreadCPU()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    int tid = pthread_self();
    return getThreadCPU(&tid);

#endif // if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if defined Q_OS_WINDOWS
    return GetCurrentProcessorNumber();

    // HANDLE tid = GetCurrentThread();
    // return getThreadCPU(&tid);

#endif // if defined Q_OS_WINDOWS
    return -1;
}

bool FuncHelper::getLinkInfo(const QString& sfilename,
                             QString      & filename,
                             QPixmap      & pixmap)
{
    QFileInfo finfo(sfilename);

    // if ((finfo.exists() == false) ||
    //     (!finfo.isSymLink() &&
    //      !finfo.isShortcut())) return false;

    if (!finfo.exists() || !finfo.isFile()) return false;

#if defined(Q_OS_WINDOWS)

    // 不是.lnk文件
    if (!finfo.isShortcut()) return false;

#else // ifdef Q_OS_WINDOWS

    if (!finfo.isSymLink()) return false;

#endif // ifdef Q_OS_WINDOWS

    QFileIconProvider ficon;
    pixmap = ficon.icon(finfo).pixmap(32, 32);
    filename = finfo.symLinkTarget();
    return true;
}
