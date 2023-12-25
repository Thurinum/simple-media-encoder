#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
    a.setApplicationName("Efficient Media Encoder");

    MainWindow w;
	w.setWindowIcon(QIcon("appicon.png"));
	w.show();

	return a.exec();
}
