
#include "QGmailNotifier.h"
#include "version.h"
#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QNetworkReply>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QUrl>

//------------------------------------------------------------------------------
// Name: QGmailNotifier
// Desc:
//------------------------------------------------------------------------------
QGmailNotifier::QGmailNotifier(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), trayIcon_(0), alertIndex_(-1), animationIndex_(0) {

	ui.setupUi(this);

	gmailFeed_ = new GMailFeed(this);
	connect(gmailFeed_, SIGNAL(fetchComplete(QNetworkReply *)), this, SLOT(fetchComplete(QNetworkReply *)));

	trayIcon_ = new QSystemTrayIcon(this);
	trayIcon_->setContextMenu(createMenu());
	trayIcon_->setIcon(QIcon(":/img/normal.svgz"));

	connect(trayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
	connect(trayIcon_, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

	trayIcon_->show();

	// start the check timer
	timer_ = new QTimer(this);
	connect(timer_, SIGNAL(timeout()), this, SLOT(doCheck()));
	timer_->start(0);

	// setup the anim timer
	animationTimer_ = new QTimer(this);
	connect(animationTimer_, SIGNAL(timeout()), this, SLOT(doAnim()));
}

//------------------------------------------------------------------------------
// Name: doAnim
// Desc:
//------------------------------------------------------------------------------
void QGmailNotifier::doAnim() {

	if(animationIndex_ == 0) {
		trayIcon_->setIcon(QIcon(":/img/green.svgz"));
	} else {
		trayIcon_->setIcon(QIcon(":/img/normal.svgz"));
	}

	animationIndex_ = (animationIndex_ + 1) % 2;
}

//------------------------------------------------------------------------------
// Name: doCompose
// Desc:
//------------------------------------------------------------------------------
void QGmailNotifier::doCompose() {
	openURL("http://mail.google.com/mail/#compose");
}


//------------------------------------------------------------------------------
// Name: readConversation
// Desc:
//------------------------------------------------------------------------------
void QGmailNotifier::readConversation() {
	QAction *const action = qobject_cast<QAction *>(sender());
	const int index = action->data().toInt();
	readMessage(index);
}

//------------------------------------------------------------------------------
// Name: createMenu
// Desc:
//------------------------------------------------------------------------------
QMenu *QGmailNotifier::createMenu() {
	QMenu *const menu = new QMenu(this);
	QAction *const view		= menu->addAction(tr("View Inbox"), this, SLOT(doView()));
	menu->addAction(tr("Compose a Message"), this, SLOT(doCompose()));
	menu->addAction(tr("Check Mail Now"), this, SLOT(doCheck()));
	menu->addAction(tr("Tell me Again..."), this, SLOT(doTell()));
	menu->addAction(tr("Options"), this, SLOT(doOptions()));
	menu->addAction(tr("About"), this, SLOT(doAbout()));

	if(currentMails_.size() != 0) {

		menu->addSeparator();
		QMenu *const convsations = menu->addMenu(tr("Unread Conversations"));

		QSettings settings;

		const int maxCons = settings.value("max_conversations", 10).value<int>();

		int index = 0;
		Q_FOREACH(GMailEntry entry, currentMails_) {
			QAction *const action = convsations->addAction(entry.author_name + " : " + entry.title, this, SLOT(readConversation()));
			action->setData(index++);
			if(index >= maxCons) {
				break;
			}
		}
	}

	menu->addSeparator();
	menu->addAction(tr("Exit"), qApp, SLOT(quit()));

	menu->setDefaultAction(view);
	return menu;
}

//------------------------------------------------------------------------------
// Name: doCheck
// Desc:
//------------------------------------------------------------------------------
void QGmailNotifier::doCheck() {
	// once a minute
	QSettings settings;
	QString user = settings.value("username", "").value<QString>();
	const QString pass = settings.value("password", "").value<QString>();
	const int interval = settings.value("check_interval", 60000).value<int>();

	timer_->setInterval(interval);

	if(user.isEmpty() || pass.isEmpty()) {
		show();
	} else {
		if(!user.endsWith("@gmail.com")) {
			user.append("@gmail.com");
		}
		animationTimer_->start(200);
		trayIcon_->setToolTip(tr("Reading feed"));
		gmailFeed_->fetch(user, pass);
	}
}

//------------------------------------------------------------------------------
// Name: showEvent
// Desc: sets all of the widgets values based on the settings
//------------------------------------------------------------------------------
void QGmailNotifier::showEvent(QShowEvent *event) {
	Q_UNUSED(event);
	QSettings settings;
	ui.txtUser->setText(settings.value("username", "").value<QString>());
	ui.txtPass->setText(settings.value("password", "").value<QString>());
	ui.spnInterval->setValue(settings.value("check_interval", 60000).value<int>());
	ui.spnPopup->setValue(settings.value("popup_time_span", 4000).value<int>());
	ui.spnConversation->setValue(settings.value("max_conversations", 10).value<int>());
}

//------------------------------------------------------------------------------
// Name: accept
// Desc: stores settings when the options dialog box is accepted
//------------------------------------------------------------------------------
void QGmailNotifier::accept() {
	QSettings settings;
	settings.setValue("username", ui.txtUser->text());
	settings.setValue("password", ui.txtPass->text());
	settings.setValue("check_interval", ui.spnInterval->value());
	settings.setValue("popup_time_span", ui.spnPopup->value());
	settings.setValue("max_conversations", ui.spnConversation->value());
	QDialog::accept();
}

//------------------------------------------------------------------------------
// Name: doView
// Desc: loads the gmail main page in the user's default browser
//------------------------------------------------------------------------------
void QGmailNotifier::doView() {
	openURL("http://mail.google.com/mail");
}

//------------------------------------------------------------------------------
// Name: activated
// Desc: lets us know when the user has double clicked the tray icon
//------------------------------------------------------------------------------
void QGmailNotifier::activated(QSystemTrayIcon::ActivationReason reason) {
	if(reason == QSystemTrayIcon::DoubleClick) {
		doView();
	}
}

//------------------------------------------------------------------------------
// Name: doTell
// Desc: tells the user about new mails, even if they are all "old news"
//------------------------------------------------------------------------------
void QGmailNotifier::doTell() {
	seen_.clear();
	alertIndex_ = -1;
	if(currentMails_.empty()) {
		QSettings settings;
		const int time = settings.value("popup_time_span").value<int>();

		trayIcon_->showMessage("", "Your inbox contains no unread conversations." , QSystemTrayIcon::Information, time);
	} else {
		doTellReal();
	}
}

//------------------------------------------------------------------------------
// Name: doTellReal
// Desc: actual implemenation of tell
//------------------------------------------------------------------------------
void QGmailNotifier::doTellReal() {
	QSettings settings;
	const int time = settings.value("popup_time_span").value<int>();

	Q_FOREACH(GMailEntry entry, currentMails_) {
		if(!seen_.contains(entry.id)) {
			trayIcon_->showMessage(entry.author_name + " : " + entry.title, entry.summary, QSystemTrayIcon::Information, time);
			seen_.insert(entry.id);
		}
		++alertIndex_;
	}
}

//------------------------------------------------------------------------------
// Name: doOptions
// Desc: shows the options dialog
//------------------------------------------------------------------------------
void QGmailNotifier::doOptions() {
	show();
}

//------------------------------------------------------------------------------
// Name: doAbout
// Desc: shows the about dialog
//------------------------------------------------------------------------------
void QGmailNotifier::doAbout() {
	alertIndex_ = -1;

	trayIcon_->showMessage(
		tr("QGmailNotifier"),
		tr("QGmailNotifier version %1. Written by Evan Teran\nIcons are the trademark of Google\n\n"
		"Google, Gmail and Google Mail are registered trademarks of Google Inc.\n"
		"QGmailNotifier nor its author are in any way affiliated nor endorsed by Google Inc.\n").arg(version),
		QSystemTrayIcon::Information,
		0);
}

//------------------------------------------------------------------------------
// Name: fetchComplete
// Desc: lets us know that the main module is finished fetching
//------------------------------------------------------------------------------
void QGmailNotifier::fetchComplete(QNetworkReply *reply) {

	animationTimer_->stop();
	animationIndex_ = 0;

	if(reply->error() != QNetworkReply::NoError) {
		QSettings settings;
		const int time = settings.value("popup_time_span").value<int>();

		trayIcon_->setIcon(QIcon(":/img/error.svgz"));
		trayIcon_->showMessage("Error", reply->errorString(), QSystemTrayIcon::Critical, time);
	} else {

		currentMails_ = gmailFeed_->mail();

		if(currentMails_.size() == 1) {
			trayIcon_->setToolTip(tr("%1 unread conversation").arg(currentMails_.size()));
		} else {
			trayIcon_->setToolTip(tr("%1 unread conversations").arg(currentMails_.size()));
		}

		alertIndex_ = -1;

		if(currentMails_.size() != 0) {
			doTellReal();
#if 0
			QFont f;
			f.setPixelSize(14);
			f.setWeight(QFont::Black);

			QPixmap pm(":/img/blue.svgz");
			QPainter p(&pm);

			p.setFont(f);
			p.setPen(Qt::black);
			p.drawText(0, 0, pm.width(), pm.height(), Qt::AlignCenter, QString::number(currentMails_.size()));

			trayIcon_->setIcon(pm);
#else
			trayIcon_->setIcon(QIcon(":/img/blue.svgz"));
#endif
		} else {
			trayIcon_->setIcon(QIcon(":/img/normal.svgz"));
		}

		trayIcon_->setContextMenu(createMenu());
	}
}

//------------------------------------------------------------------------------
// Name: openURL
// Desc: opens a URL using the default browser
//------------------------------------------------------------------------------
void QGmailNotifier::openURL(const QString &url) {
	QDesktopServices::openUrl(QUrl::fromEncoded(url.toLatin1()));
	qDebug() << "opening URL: " << url;
}

//------------------------------------------------------------------------------
// Name: messageClicked
// Desc: the user has clicked on our message
//------------------------------------------------------------------------------
void QGmailNotifier::messageClicked() {
	readMessage(alertIndex_);
}

//------------------------------------------------------------------------------
// Name: readMessage
// Desc:
//------------------------------------------------------------------------------
void QGmailNotifier::readMessage(int index) {
	if(index != -1) {
		if(index < currentMails_.size()) {
			GMailEntry entry = currentMails_[index];
			// work around a bug in QT 4.4.0's QUrl
			//entry.link.replace("%40", "@");

			openURL(entry.link);
		}
	}
}

