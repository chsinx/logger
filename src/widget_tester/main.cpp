#include <ctime>
#include <QApplication>
#include <QMainWindow>

#include <logging/loggerwidget.h>
#include <logging/logger.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0) :
        QMainWindow(parent)
    {
        setCentralWidget(new logging::LoggerWidget());
    }
    ~MainWindow()
    {}

};

int main(int argc, char *argv[])
{
    logging::Logger::instance().setLogDir("./logs");
    logging::Logger::instance().setLogFilePrefix("test");

    logging::info("start");

    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    logging::info("Hello world!");

    qsrand(std::time(0));
    for(int i = 0; i < 10; ++i) {
        logging::trace(" i = ") << i << " x = " << i*qrand()/1000.0;
    }

    logging::Logger::instance().setMinLevel(logging::l_error);
    logging::info() << "Invisible message";
    logging::error() << "TEST Error!";

    logging::Logger::instance().setMinLevel(logging::l_debug);
    logging::info() << "Visible message";

    return a.exec();
}

#include "main.moc"
