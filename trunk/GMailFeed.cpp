
#include "GMailFeed.h"
#include <QAuthenticator>
#include <QBuffer>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

//------------------------------------------------------------------------------
// Name: GMailFeed
// Desc:
//------------------------------------------------------------------------------
GMailFeed::GMailFeed(QObject *parent) : QObject(parent), networkManager_(0) {
	networkManager_ = new QNetworkAccessManager(this);
	connect(networkManager_, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(onAuthenticationRequestSlot(QNetworkReply*, QAuthenticator*)));
	connect(networkManager_, SIGNAL(finished(QNetworkReply *)), this, SLOT(requestFinished(QNetworkReply *)));	
}

//------------------------------------------------------------------------------
// Name: onAuthenticationRequestSlot
// Desc:
//------------------------------------------------------------------------------
void GMailFeed::onAuthenticationRequestSlot(QNetworkReply *reply, QAuthenticator *authenticator) {
	
	Credentials creds = credentials_.take(reply);
	
	authenticator->setUser(creds.user);
	authenticator->setPassword(creds.pass);
}

//------------------------------------------------------------------------------
// Name: fetch
// Desc:
//------------------------------------------------------------------------------
void GMailFeed::fetch(const QString &user, const QString &pass) {

	entries_.clear();
	xmlReader_.clear();

	QNetworkRequest request;
	request.setUrl(QUrl("https://mail.google.com/mail/feed/atom"));
	QNetworkReply *const r = networkManager_->get(request);
	
	Credentials creds;
	creds.user = user;
	creds.pass = pass;
	
	credentials_.insert(r, creds);
}

//------------------------------------------------------------------------------
// Name: requestFinished
// Desc:
//------------------------------------------------------------------------------
void GMailFeed::requestFinished(QNetworkReply *reply) {

	if(reply->error() != QNetworkReply::NoError) {
		qWarning() << "Aborting: got status code: " << reply->error();
	} else {
		const QByteArray data = reply->readAll();
		xmlReader_.addData(data);
		parseXml();
	}
	
	Q_EMIT fetchComplete(reply);
}

//------------------------------------------------------------------------------
// Name: parseXml
// Desc:
//------------------------------------------------------------------------------
void GMailFeed::parseXml(){

	QString currentTag;
	bool inEntry = false;
	bool inAuthor = false;

	while (!xmlReader_.atEnd()) {

		xmlReader_.readNext();

		if (xmlReader_.isStartElement()) {

			if (xmlReader_.name() == "entry") {
				inEntry = true;
				entries_.push_back(GMailEntry());
			} else if (xmlReader_.name() == "author") {
				inAuthor = true;
			} else if(inEntry && xmlReader_.name() == "link") {
				entries_.last().link = xmlReader_.attributes().value("href").toString();
			}

			currentTag = xmlReader_.name().toString();
		} else if (xmlReader_.isEndElement()) {
			if (xmlReader_.name() == "entry") {
				inEntry = false;
			} else if (xmlReader_.name() == "author") {
				inAuthor = false;

			}
		} else if (xmlReader_.isCharacters() && !xmlReader_.isWhitespace()) {

			if(inEntry) {
				if (currentTag == "title") {
					entries_.last().title += xmlReader_.text().toString();
				} else if (currentTag == "summary") {
					entries_.last().summary += xmlReader_.text().toString();
				} else if (currentTag == "link") {
					entries_.last().link = xmlReader_.attributes().value("href").toString();
				} else if (currentTag == "modified") {
					entries_.last().modified = xmlReader_.text().toString();
				} else if (currentTag == "issued") {
					entries_.last().issued = xmlReader_.text().toString();
				} else if (currentTag == "id") {
					entries_.last().id = xmlReader_.text().toString();
				} else {
					if(inAuthor) {
						if (currentTag == "name") {
							entries_.last().author_name += xmlReader_.text().toString();
						} else if (currentTag == "email") {
							entries_.last().author_email = xmlReader_.text().toString();
						}
					}
				}
			}
		}
	}

	if (xmlReader_.error() && xmlReader_.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
		qWarning() << "XML ERROR:" << xmlReader_.lineNumber() << ": " << xmlReader_.errorString();
	}
}
