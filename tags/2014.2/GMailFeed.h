
#ifndef GMAILFEED_20080725_H_
#define GMAILFEED_20080725_H_

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <QXmlStreamReader>

class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;

struct GMailEntry {
	QString title;
	QString summary;
	QString link;
	QString modified;
	QString issued;
	QString id;
	QString author_name;
	QString author_email;
};

struct Credentials {
	QString user;
	QString pass;
};

class GMailFeed : public QObject {
	Q_OBJECT

public:
	GMailFeed(QObject *parent = 0);

public:
	void fetch(const QString &user, const QString &pass);
	const QVector<GMailEntry> &mail() const { return entries_; }

public Q_SLOTS:
	void onAuthenticationRequestSlot(QNetworkReply *reply, QAuthenticator *authenticator);
	void requestFinished(QNetworkReply *reply);

Q_SIGNALS:
	void fetchComplete(QNetworkReply *reply);

private:
	void parseXml();

private:
	QXmlStreamReader	               xmlReader_;
	QNetworkAccessManager             *networkManager_;
	QMap<QNetworkReply *, Credentials> credentials_;
	QVector<GMailEntry>	               entries_;
};

#endif

