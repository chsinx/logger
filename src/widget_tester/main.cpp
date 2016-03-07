#include "mainwindow.h"
#include <QApplication>

#include <logging/loggerwidget.h>
#include <logging/logger.h>

int main(int argc, char *argv[])
{
    logging::info("start");

    QApplication a(argc, argv);
    MainWindow w;

    w.setCentralWidget(new logging::LoggerWidget());
    w.show();

    logging::info("Hello world!");

    return a.exec();
}
