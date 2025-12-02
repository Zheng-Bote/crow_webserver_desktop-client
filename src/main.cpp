#include <QApplication>

#include "MainWindow.h"

#include "includes/rz_config.hpp"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    QCoreApplication::setApplicationVersion(PROJECT_VERSION.c_str());
    QApplication::setWindowIcon(QIcon("img/logo-36x36.png"));

    w.show();
    return a.exec();
}
