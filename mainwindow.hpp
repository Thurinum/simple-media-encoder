#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QSettings>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

private:
	Ui::MainWindow *ui;
	QUrl selectedUrl;
	QSettings settings = QSettings("config.ini", QSettings::IniFormat);

	QVariant setting(QString key);
	void setSetting(QString key, QVariant value);

public slots:
	void compressionEnded(int statusCode);

protected:
	void closeEvent(QCloseEvent* event) override;
};
#endif // MAINWINDOW_HPP
