#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("KamKontroller");
    application.setOrganizationName("KamKontroller");

    MainWindow window;
    window.show();
    return application.exec();
}