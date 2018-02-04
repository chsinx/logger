#ifndef LOGGERWIDGET_H
#define LOGGERWIDGET_H

#include <QWidget>

class QTableView;

class LoggingModel;

class LoggerWidget : public QWidget
{
    Q_OBJECT

    QTableView* textTable_;

    ///все виджеты логгеры работают с одной моделью
    static LoggingModel* model();

public:
    explicit LoggerWidget(QWidget *parent = 0);
    ~LoggerWidget();

};



#endif // LOGGERWIDGET_H
