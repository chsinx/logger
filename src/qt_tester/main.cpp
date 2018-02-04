#include <ctime>
#include <QApplication>
#include <QMainWindow>

#include <logger.h>
#include "loggerwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0) :
        QMainWindow(parent)
    {
        setCentralWidget(new LoggerWidget());
    }
    ~MainWindow()
    {}

};

struct TestRunner {

    TestRunner() {
        test_hello_world();
        test_float();
        test_loglevel();
    }

    void test_hello_world()
    {
        logging::info("Hello world!");
    }

    void test_float()
    {
        qsrand(std::time(0));
        for(int i = 0; i < 3; ++i) logging::trace(" i = ") << i << " x = " << i*qrand()/1000.0;
    }

    void test_loglevel()
    {
        logging::Logger::instance().setMinLevel(logging::Level::LERROR);
        logging::info() << "Truncated invisible message 1";
        logging::trace() << "Truncated invisible message 2";
        logging::error() << "TEST Error!";
        logging::Logger::instance().setMinLevel(logging::Level::LDEBUG);
        logging::info() << "Visible message";
    }

};

int main(int argc, char *argv[])
{
    logging::Logger::instance().setOutputDirectory(".");
    logging::Logger::instance().setBaseFileName("test");

    logging::info("--- Application started ---");

    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    TestRunner runner;
    (void)runner;

    return a.exec();
}

#include "main.moc"
