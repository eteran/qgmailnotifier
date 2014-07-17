
#include "GMailFeed.h"
#include <QHttp>
#include <QBuffer>
#include <QXmlStreamReader>

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
GMailFeed::GMailFeed(QObject *parent) : QObject(parent), m_HTTP(0), m_InEntry(false), m_InAuthor(false) {
	m_HTTP = new QHttp(this);
	connect(m_HTTP, SIGNAL(done(bool)), this, SLOT(done(bool)));
	connect(m_HTTP, SIGNAL(readyRead(const QHttpResponseHeader &)), this, SLOT(readData(const QHttpResponseHeader &)));
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void GMailFeed::fetch(const QString &user, const QString &pass) {
	
	m_Entries.clear();
	m_XML.clear();

	m_HTTP->setHost("mail.google.com", QHttp::ConnectionModeHttps);
	m_HTTP->setUser(user, pass);
	m_HTTP->get("/mail/feed/atom");
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void GMailFeed::done(bool error) {
	emit fetchComplete(error);
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void GMailFeed::readData(const QHttpResponseHeader &resp) {

	QByteArray data = m_HTTP->readAll();

	if (resp.statusCode() != 200)
		m_HTTP->abort();
	else {
		m_XML.addData(data);
		parseXml();
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void GMailFeed::parseXml(){

	while (!m_XML.atEnd()) {
		
		m_XML.readNext();
		
		if (m_XML.isStartElement()) {
			
			if (m_XML.name() == "entry") {
				m_InEntry = true;
				m_Entries.push_back(GMailEntry());
			} else if (m_XML.name() == "author") {
				m_InAuthor = true;
			} else if(m_InEntry && m_XML.name() == "link") {
				m_Entries.last().link = m_XML.attributes().value("href").toString();
			}
				
			currentTag = m_XML.name().toString();
		} else if (m_XML.isEndElement()) {
			if (m_XML.name() == "entry") {
				m_InEntry = false;
			} else if (m_XML.name() == "author") {
				m_InAuthor = false;

			}
		} else if (m_XML.isCharacters() && !m_XML.isWhitespace()) {
						
			if(m_InEntry) {
				if (currentTag == "title") {
					m_Entries.last().title += m_XML.text().toString();
				} else if (currentTag == "summary") {
					m_Entries.last().summary += m_XML.text().toString();
				} else if (currentTag == "link") {
					m_Entries.last().link = m_XML.attributes().value("href").toString();
				} else if (currentTag == "modified") {
					m_Entries.last().modified = m_XML.text().toString();
				} else if (currentTag == "issued") {
					m_Entries.last().issued = m_XML.text().toString();
				} else if (currentTag == "id") {
					m_Entries.last().id = m_XML.text().toString();
				} else {
					if(m_InAuthor) {
						if (currentTag == "name") {
							m_Entries.last().author_name += m_XML.text().toString();
						} else if (currentTag == "email") {
							m_Entries.last().author_email = m_XML.text().toString();
						}
					}
				}
			}
		}
	}

	if (m_XML.error() && m_XML.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
		qWarning() << "XML ERROR:" << m_XML.lineNumber() << ": " << m_XML.errorString();
		m_HTTP->abort();
	}
}
