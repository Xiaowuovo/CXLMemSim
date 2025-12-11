/**
 * @file main.cpp
 * @brief Qt GUI application entry point
 */

#include <QApplication>
#include "mainwindow.h"
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setApplicationName("CXLMemSim");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("CXLMemSim Project");

    std::cout << "CXLMemSim GUI - Starting..." << std::endl;

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}
