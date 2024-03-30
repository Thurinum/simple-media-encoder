#include "mainwindow.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
    a.setApplicationName("Efficient Media Encoder");
    a.setStyle("Fusion");

    MainWindow w;
    w.setWindowIcon(QIcon("appicon.ico"));
    w.show();

    return a.exec();
}
