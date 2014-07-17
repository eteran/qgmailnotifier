
#ifndef GMAILFEED_20080725_H_
#define GMAILFEED_20080725_H_

#include <QObject>
#include <QHttpResponseHeader>
#include <QHttp>
#include <QXmlStreamReader>
#include <QString>
#include <QDebug>
#include <QVector>

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

class GMailFeed : public QObject {
	Q_OBJECT
	
public:
	GMailFeed(QObject *parent = 0);
	
public:
	void fetch(const QString &user, const QString &pass);
	const QVector<GMailEntry> &mail() const { return m_Entries; }
	const QHttp *http() const { return &m_HTTP; }
public slots:
	void done(bool error);
	void readData(const QHttpResponseHeader &);
	
signals:
	void fetchComplete(bool);
	
private:
	void parseXml();
	
private:
	QHttp				m_HTTP;
	QXmlStreamReader	m_XML;
	
	QString linkString;
	QString currentTag;
	QString titleString;
	bool m_InEntry;
	bool m_InAuthor;
	
	QVector<GMailEntry> m_Entries;
};

#endif

