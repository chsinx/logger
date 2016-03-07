#include "loggerwidget.h"

#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>
#include <QScrollBar>

#include "loggingmodel.h"

namespace logging {

LoggingModel* LoggerWidget::model() {

    static LoggingModel model;
    return &model;
}

LoggerWidget::LoggerWidget(QWidget *parent) : QWidget(parent)
{
    textTable_ = new QTableView;
    textTable_->setModel( model() );
    textTable_->horizontalHeader()->setStretchLastSection(true);
    textTable_->horizontalHeader()->hide();
    textTable_->scrollToBottom();

    connect( model(), SIGNAL(rowsInserted(QModelIndex,int,int)), textTable_, SLOT(scrollToBottom()) );

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->setMargin(0);
    vlayout->addWidget( textTable_ );

    setLayout(vlayout);
}

LoggerWidget::~LoggerWidget()
{
}

} //ns logging

