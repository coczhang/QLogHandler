#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QObject>
#include <QDir>
#include <QTimer>
#include <QDate>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QTextStream>

const int LOG_FILE_MAX_SIZE = 10*1024;

#define RENAME_FILE_INTERVAL  1000*60*10
#define FLUSH_FILE_INTERVAL   1000

class LogHandler : public QObject
{
    Q_OBJECT
public:
    static LogHandler *GetInstance();
    ~LogHandler();

private slots:
    void OnRenameFile();
    void OnFlushFile();

private:
    explicit LogHandler();

    void CreateLogFile();    //创建日志文件
    void CheckLogFile();     //检测当前日志文件大小
    void DeleteLogFile();    //自动删除30天前的日志

    static void MsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg); //消息处理函数

private:
    QDir   m_logDir;                //日志文件夹
    QTimer *m_pRenameFileTimer;     //重命名日志文件使用的定时器
    QTimer *m_pFlushFileTimer;      //刷新输出到日志文件的定时器
    QDate  m_fileCreatedDate;       //日志文件创建的时间
    QFile *m_pLogFile;              //日志文件

    QString m_logPath;
    QString m_logName;

    static QTextStream *m_pLogOut;    //输出日志的 QTextStream，使用静态对象就是为了减少函数调用的开销
    static QMutex m_fileMutex;        //文件写入同步使用的mutex
    static QMutex m_LogMutex;
    static LogHandler *m_pLogHandler;

public:
    void InstallMessageHandler();    //给Qt安装消息处理函数
    void UninstallMessageHandler(); //取消安装消息处理函数并释放资源
};

#endif
