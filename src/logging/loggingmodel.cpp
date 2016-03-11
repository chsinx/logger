#include "loggingmodel.h"

#include "logger.h"

#include <QBrush>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace logging {


void logMessageHandler(void * data, const std::string &prefix, const std::string &msg)
{
    static_cast<LoggingModel*>(data)->logMessageHandler(prefix, msg);
}

LoggingModel::LoggingModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    Logger *logger = &Logger::instance();

    QDir logsDir( QString::fromStdString(logger->getLogDir()) );
    logsDir.setNameFilters( QStringList( QString::fromStdString(logger->getFilePrefix()) + "_*.log") );

    QStringList files = logsDir.entryList(QDir::Files, QDir::Name);

    while(!files.empty() && messages_.size() < kMaxMessages) {
        QString file = files.back();
        files.pop_back();

        QFile logFile(logsDir.absolutePath() + "/" + file);
        QStringList currentFileMessages;

        if( logFile.open( QIODevice::ReadOnly ) ) {
            QTextStream is(&logFile);
            is.setCodec("UTF-8");
            while(!is.atEnd())
                currentFileMessages.append(is.readLine());
        }
        currentFileMessages.append( messages_ );

        messages_.swap(currentFileMessages);

        logFile.close();
    }

    connect(this, &LoggingModel::newMessage, &LoggingModel::onNewMessage);
    logger->setMessageHandler(&logging::logMessageHandler, this);
}

void LoggingModel::onNewMessage(QString m) {
        int messagesCount = messages_.size();
        beginInsertRows(QModelIndex(), messagesCount, messagesCount );
        messages_.append( m );
        endInsertRows();

        if(messagesCount > kMaxMessages) {
            beginRemoveRows(QModelIndex(), 0,0);
            messages_.removeFirst();
            endRemoveRows();
        }
}

void LoggingModel::logMessageHandler(const std::string &prefix, const std::string &msg)
{
    emit newMessage(QString::fromStdString(prefix + msg));
}

int LoggingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return messages_.size();
}

int LoggingModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant LoggingModel::data(const QModelIndex &index, int role) const
{
    if ( index.isValid() &&
         index.row() < messages_.size() &&
         index.row() >= 0
         )
    {
        if(role == Qt::DisplayRole) {

            return messages_.at( index.row() );

        } else if (role == Qt::ForegroundRole) {

            const QString& str = messages_.at( index.row() );
            return QBrush( (!str.isEmpty() && str.at(0) == 'E') ? Qt::red : Qt::black );
        }
    }

    return QVariant();
}

QVariant LoggingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)

    return QVariant();
}

Qt::ItemFlags LoggingModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index);
}

} //ns logging
