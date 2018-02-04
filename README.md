## Simple logger with Qt test example

  Header-only simple c++ logger that uses a worker to avoid I/O operation delays in main thread. 
  Designed for small projects that need simple but fast logging.
  Files are split every 24 hours.

  Use at your own risk :)

## Usage

Just include

    #include <logger.h>


Do some setup

    logging::Logger::instance().setMinLevel(logging::Level::LERROR);
    logging::Logger::instance().setOutputDirectory(".");
    logging::Logger::instance().setBaseFileName("test");


Log messages like

    ...
    logging::info() << "The answer is " << 42;
    
    ...
    logging::error() << "Oops!";


PS See Logger class for public methods available


