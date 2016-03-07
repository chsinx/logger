#ifndef LOGGINGMODEL_H
#define LOGGINGMODEL_H

#include <QAbstractTableModel>
#include <QVariant>
#include <QStringList>

namespace logging {

class LoggingModel : public QAbstractTableModel
{
    Q_OBJECT

    QStringList messages_;
    static const int kMaxMessages = 10000;

signals:
    void newMessage(QString);
private slots:
    void onNewMessage(QString m);

public:

    explicit LoggingModel(QObject *parent = 0);

    ~LoggingModel(){}

    // ошибки выводятся красным, обычные сообщения синим, отладочные сообщения - черным
    void logMessageHandler(const std::string &prefix, const std::string &msg);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

} //ns logging

#endif // LOGGINGMODEL_H
