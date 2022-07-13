#include <QApplication>
#include <QDebug>
#include <QTime>
#include "loghandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LogHandler::GetInstance()->InstallMessageHandler();
    qDebug() << "当前时间是: " << QTime::currentTime().toString("hh:mm:ss");
    int nRet = a.exec();
    LogHandler::GetInstance()->UninstallMessageHandler();

    return nRet;
}
