#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QObject>
#include <QSettings>
#include <QString>

class Settings : public QObject
{
	Q_OBJECT
public:
	void Init(const QString& fileName);

	QVariant get(const QString& key);
	void Set(const QString& key, const QVariant& value);

	QStringList keysInGroup(const QString& group);
	QString fileName();

signals:
	void configNotFound(QString fileName);
	void keyNotFound(QString key);
	void keyFallbackUsed(QString key, QVariant value);
	void keyCreated(QString key);
	void notInitialized();

private:
	QSettings* m_settings;
	QSettings* m_defaultSettings;

	bool m_isInit = false;
};

#endif // SETTINGS_HPP
