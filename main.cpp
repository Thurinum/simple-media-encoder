#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("Video Compressor");

	MainWindow w;
	w.setWindowIcon(QIcon("appicon.png"));
	w.show();

	return a.exec();
}
