#include "src/shouyinw.h"
#include "src/cxxlog.h"
#include "src/funchelper.h"
#include <QApplication>

void ExitRoutine1(void) {
    INFO_LOG_CXX("while exit\n");
}

void ExitRoutine2(void) {
    INFO_LOG_CXX("exit\n");
}

void exitAT() {
    // 注册顺序和执行顺序相反
    // 对于ctrl+c之类的不起作用，只有exit()或关闭窗口时才会起作用
    atexit(ExitRoutine2);
    atexit(ExitRoutine1);

    // exit(0);
}

int main(int argc, char *argv[])
{
#if defined(_WIN32)
    FuncHelper::AttachConsoleAndRedirect();
#endif // if defined(_WIN32)

    REDIRECT_QTMESSAGE_LOG_CXX(nullptr);

    exitAT();

    QApplication a(argc, argv);

    QString logdir = QCoreApplication::applicationDirPath() + "/logs";
    qDebug() << logdir << logdir.toLocal8Bit().data();
    std::string logdirstring = logdir.toLocal8Bit().toStdString();
    CxxLog::getInstance().initLog(logdirstring,
                                  1000, 1024 * 1024 * 10);
    CxxLog::getInstance().setLogLevel(CxxLog::LOG_TYPE_DEBUG);
    CxxLog::getInstance().setFileLogLevel(CxxLog::LOG_TYPE_DEBUG);
    CxxLog::getInstance().setPrint(true);
    CxxLog::getInstance().setColorLog(false);

    ShouYinW w;

    w.show();
    return QApplication::exec();
}
