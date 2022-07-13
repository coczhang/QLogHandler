#include "loghandler.h"

// 初始化 static 变量
QMutex LogHandler::m_fileMutex;
QMutex LogHandler::m_LogMutex;
QTextStream* LogHandler::m_pLogOut = nullptr;
LogHandler* LogHandler::m_pLogHandler = nullptr;

LogHandler::LogHandler():m_pLogFile(nullptr)
{
    //设置日志文件夹
    m_logPath = QDir::currentPath() + "/log";
    //获取日志的路径
    m_logName = m_logPath + "/today.log";
    //获取日志文件最后修改的时间
    m_fileCreatedDate = QFileInfo(m_logName).lastModified().date(); // 若日志文件不存在，返回nullptr

    //打开日志文件，如果不是当天创建的，备份已有日志文件
    CreateLogFile();

    //十分钟检查一次日志文件创建时间
    m_pRenameFileTimer = new QTimer(this);
    connect(m_pRenameFileTimer, &QTimer::timeout, this, &LogHandler::OnRenameFile);
    m_pRenameFileTimer->start(RENAME_FILE_INTERVAL);

    //定时刷新日志输出到文件，在日志文件里写入最新的日志
    m_pFlushFileTimer = new QTimer(this);
    connect(m_pFlushFileTimer, &QTimer::timeout, this, &LogHandler::OnFlushFile);
    m_pFlushFileTimer->start(FLUSH_FILE_INTERVAL);
}

LogHandler::~LogHandler()
{
    if (m_pLogHandler != nullptr) {
        delete m_pLogHandler;
        m_pLogHandler = nullptr;
    }


    if (nullptr != m_pLogFile) {
        m_pLogFile->flush();
        m_pLogFile->close();
        delete m_pLogFile;
        m_pLogFile = nullptr;

        delete m_pLogOut;
        m_pLogOut  = nullptr;
    }

    if (m_pRenameFileTimer->isActive()) {
        m_pRenameFileTimer->stop();
    }

    if (m_pFlushFileTimer->isActive()) {
        m_pFlushFileTimer->stop();
    }
}

void LogHandler::OnRenameFile()
{
    QMutexLocker locker(&LogHandler::m_fileMutex);
    CreateLogFile();  //创建日志文件
    CheckLogFile();   //检测当前日志文件大小
    DeleteLogFile();  //自动删除30天前的日志
}

void LogHandler::OnFlushFile()
{
    //qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"); // 测试不停的写入内容到日志文件
    QMutexLocker locker(&LogHandler::m_fileMutex);
    if (nullptr != m_pLogOut) {
        m_pLogOut->flush();
    }
}

LogHandler* LogHandler::GetInstance()
{
    if (nullptr == m_pLogHandler) {
        m_LogMutex.lock();
        if (nullptr == m_pLogHandler) {
            m_pLogHandler = new LogHandler();
        }
        m_LogMutex.unlock();
    }

    return m_pLogHandler;
}

// 打开日志文件 log.txt，如果不是当天创建的，则使用创建日期把其重命名为 yyyy-MM-dd.log，并重新创建一个 log.txt
void LogHandler::CreateLogFile()
{
    if (!m_logDir.exists(m_logPath)) {
        m_logDir.mkpath(m_logPath);
    }

    //程序每次启动时 logFile 为 nullptr
    if (nullptr == m_pLogFile) {
        m_pLogFile = new QFile(m_logName);
        m_pLogOut  = (m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) ?  new QTextStream(m_pLogFile) : nullptr;
        if (m_pLogOut != nullptr)
            m_pLogOut->setCodec("UTF-8");

        //如果文件是第一次创建，则创建日期是无效的，把其设置为当前日期
        if (m_fileCreatedDate.isNull()) {
            m_fileCreatedDate = QDate::currentDate();
        }
    }

    //程序运行时如果创建日期不是当前日期，则使用创建日期重命名，并生成一个新的 log.txt
    if (m_fileCreatedDate != QDate::currentDate()) {
        m_pLogFile->flush();
        m_pLogFile->close();
        delete m_pLogOut;
        delete m_pLogFile;

        QString newLogName = m_logPath + "/" + m_fileCreatedDate.toString("yyyy-MM-dd.log");
        QFile::copy(m_logName, newLogName); //日志文件重命名
        QFile::remove(m_logName); // 删除重新创建，改变创建时间

        // 重新创建 log.txt
        m_pLogFile = new QFile(m_logName);
        m_pLogOut  = (m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) ?  new QTextStream(m_pLogFile) : nullptr;
        if (m_pLogOut != nullptr)
            m_pLogOut->setCodec("UTF-8");

        m_fileCreatedDate = QDate::currentDate();
    }
}

// 检测当前日志文件大小
void LogHandler::CheckLogFile()
{
    // 如果 protocal.log 文件大小超过10M，重新创建一个日志文件，原文件存档为yyyy-MM-dd_hhmmss.log
    if (m_pLogFile->size() > LOG_FILE_MAX_SIZE) {
        m_pLogFile->flush();
        m_pLogFile->close();
        delete m_pLogOut;
        delete m_pLogOut;

        QString newLogName =m_logPath + "/" + m_fileCreatedDate.toString("yyyy-MM-dd.log");
        QFile::copy(m_logName, newLogName);
        QFile::remove(m_logName); // 删除重新创建，改变创建时间

        m_pLogFile = new QFile(m_logName);
        m_pLogOut  = (m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) ?  new QTextStream(m_pLogFile) : NULL;
        if (m_pLogOut != nullptr)
            m_pLogOut->setCodec("UTF-8");

        m_fileCreatedDate = QDate::currentDate();
    }
}

// 自动删除7天前的日志
void LogHandler::DeleteLogFile()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime dateTime1 = now.addDays(-7);
    QDateTime dateTime2;

    QDir dir(m_logName);
    QFileInfoList fileList = dir.entryInfoList();
    foreach (QFileInfo f, fileList ) {
        // "."和".."跳过
        if (f.baseName() == "")
            continue;

        dateTime2 = QDateTime::fromString(f.baseName(), "yyyy-MM-dd");
        if (dateTime2 < dateTime1) { //只要日志时间小于前7天的时间就删除
            dir.remove(f.absoluteFilePath());
        }
    }
}

// 消息处理函数
void LogHandler::MsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&LogHandler::m_fileMutex);
    QString msglevel;

    switch (type) {
    case QtDebugMsg:
        msglevel = "INFO";
        break;
    case QtWarningMsg:
        msglevel = "WARN ";
        break;
    case QtCriticalMsg:
        msglevel = "ERROR";
        break;
    case QtFatalMsg:
        msglevel = "FATAL";
        break;
    default:
        break;
    }

    if (nullptr == LogHandler::m_pLogOut) {
        return;
    }

    // 输出到日志文件, 格式: 时间 - [Level] (文件名:行数, 函数): 消息
    QString fileName = context.file;
    int index = fileName.lastIndexOf(QDir::separator());
    fileName = fileName.mid(index + 1);

    (*LogHandler::m_pLogOut) << QString("%1 - [%2] (%3:%4, %5): %6\n")
                                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(msglevel)
                                    .arg(fileName).arg(context.line).arg(context.function).arg(msg);
}

// 给Qt安装消息处理函数
void LogHandler::InstallMessageHandler()
{
    QMutexLocker locker(&LogHandler::m_fileMutex);

    qInstallMessageHandler(LogHandler::MsgHandler); // 给 Qt 安装自定义消息处理函数
}

// 取消安装消息处理函数并释放资源
void LogHandler::UninstallMessageHandler()
{
    QMutexLocker locker(&LogHandler::m_fileMutex);

    qInstallMessageHandler(nullptr);
}

