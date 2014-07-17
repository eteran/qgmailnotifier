
#include "QGmailNotifier.h"
#include <QMenu>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QSettings>

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
QGmailNotifier::QGmailNotifier(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), trayIcon(0), m_AlertIndex(-1), m_AnimIndex(0) {
	
	ui.setupUi(this);
	
	m_GmailFeed = new GMailFeed(this);
	connect(m_GmailFeed, SIGNAL(fetchComplete(bool)), this, SLOT(fetchComplete(bool)));

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(createMenu());
	trayIcon->setIcon(QIcon(":/img/normal.ico"));
	
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)));
	connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));
	
	trayIcon->show();
	
	// start the check timer
	m_Timer = new QTimer(this);
	connect(m_Timer, SIGNAL(timeout()), this, SLOT(doCheck()));
	m_Timer->start(0);
	
	// setup the anim timer
	m_AnimTimer = new QTimer(this);
	connect(m_AnimTimer, SIGNAL(timeout()), this, SLOT(doAnim()));
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doAnim() {
	
	if(m_AnimIndex == 0) {
		trayIcon->setIcon(QIcon(":/img/green.ico"));
	} else {
		trayIcon->setIcon(QIcon(":/img/normal.ico"));
	}
	
	m_AnimIndex = (m_AnimIndex + 1) % 2;
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doCompose() {
	QDesktopServices::openUrl(QUrl("http://mail.google.com/mail/#compose"));
	qDebug() << "opening URL: " << "http://mail.google.com/mail/#compose";
	
}


//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::readConversation() {
	QAction *const action = qobject_cast<QAction *>(sender());
	const int index = action->data().toInt();
	readMessage(index);
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
QMenu *QGmailNotifier::createMenu() {
	QMenu *const menu = new QMenu(this);
	QAction *const view		= menu->addAction(QObject::tr("View Inbox"), this, SLOT(doView()));
	menu->addAction(QObject::tr("Compose a Message"), this, SLOT(doCompose()));
	menu->addAction(QObject::tr("Check Mail Now"), this, SLOT(doCheck()));
	menu->addAction(QObject::tr("Tell me Again..."), this, SLOT(doTell()));
	menu->addAction(QObject::tr("Options"), this, SLOT(doOptions()));
	menu->addAction(QObject::tr("About"), this, SLOT(doAbout()));
	
	if(m_CurrentMails.size() != 0) {

		menu->addSeparator();
		QMenu *const convsations = menu->addMenu(QObject::tr("Unread Conversations"));
		
		QSettings settings;
		
		const int maxCons = settings.value("max_conversations", 10).value<int>();
		
		int index = 0;
		foreach(GMailEntry entry, m_CurrentMails) {
			QAction *const action = convsations->addAction(entry.author_name + " : " + entry.title, this, SLOT(readConversation()));
			action->setData(index++);
			if(index >= maxCons) {
				break;
			}
		}
	}
	
	menu->addSeparator();
	menu->addAction(QObject::tr("Exit"), qApp, SLOT(quit()));
	
	menu->setDefaultAction(view);
	return menu;
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doCheck() {
	// once a minute
	QSettings settings;
	QString user = settings.value("username", "").value<QString>();
	const QString pass = settings.value("password", "").value<QString>();
	const int interval = settings.value("check_interval", 60000).value<int>();
	
	m_Timer->setInterval(interval);
	
	if(user.isEmpty() || pass.isEmpty()) {
		show();
	} else {
		if(!user.endsWith("@gmail.com")) {
			user.append("@gmail.com");
		}
		m_AnimTimer->start(200);
		m_GmailFeed->fetch(user, pass);
		
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
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
// Name: 
// Desc: 
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
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doView() {
	QDesktopServices::openUrl(QUrl("http://mail.google.com/mail"));
	qDebug() << "opening URL: " << "http://mail.google.com/mail";
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::activated(QSystemTrayIcon::ActivationReason reason) {
	if(reason == QSystemTrayIcon::DoubleClick) {
		doView();
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doTell() {
	m_Seen.clear();
	m_AlertIndex = -1;
	if(m_CurrentMails.size() == 0) {
		QSettings settings;
		const int time = settings.value("popup_time_span").value<int>();
		trayIcon->showMessage("", "Your inbox contains no unread conversations." , QSystemTrayIcon::Information, time);
	} else {
		doTellReal();
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doTellReal() {
	QSettings settings;
	const int time = settings.value("popup_time_span").value<int>();

	foreach(GMailEntry entry, m_CurrentMails) {
		if(!m_Seen.contains(entry.id)) {
			trayIcon->showMessage(entry.author_name + " : " + entry.title, entry.summary, QSystemTrayIcon::Information, time);
			m_Seen.insert(entry.id);
		}
		++m_AlertIndex;
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doOptions() {
	show();
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doAbout() {
	QMessageBox::about(
		this, 
		"QGmailNotifier", 
		"QGmailNotifier version 2008.1. Written by <a href='mailto:eteran@alum.rit.edu'>Evan Teran</a><br>Icons are the trademark of <a href='http://www.google.com/'>Google</a><br><br>"
		"Google, Gmail and Google Mail are registered trademarks of Google Inc.<br>"
		"QGmailNotifier nor its author are in any way affiliated nor endorsed by Google Inc.<br>"
		);				
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::fetchComplete(bool error) {
	
	m_AnimTimer->stop();
	m_AnimIndex = 0;
	
	if(error) {
		QSettings settings;
		const int time = settings.value("popup_time_span").value<int>();
		
		trayIcon->setIcon(QIcon(":/img/error.ico"));
		trayIcon->showMessage("Error", m_GmailFeed->http()->errorString(), QSystemTrayIcon::Critical, time);
	} else {
	
		m_CurrentMails = m_GmailFeed->mail();

		if(m_CurrentMails.size() == 1) {
			trayIcon->setToolTip(QString(QObject::tr("%1 unread conversation")).arg(m_CurrentMails.size()));
		} else {
			trayIcon->setToolTip(QString(QObject::tr("%1 unread conversations")).arg(m_CurrentMails.size()));
		}

		m_AlertIndex = -1;

		if(m_CurrentMails.size() != 0) {
			doTellReal();
			trayIcon->setIcon(QIcon(":/img/blue.ico"));
		} else {
			trayIcon->setIcon(QIcon(":/img/normal.ico"));
		}
		
		trayIcon->setContextMenu(createMenu());
	}
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::messageClicked() {
	readMessage(m_AlertIndex);
}

//------------------------------------------------------------------------------
// Name: 
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::readMessage(int index) {
	if(index != -1) {
		if(index < m_CurrentMails.size()) {
			GMailEntry entry = m_CurrentMails[index];
			
			// work around a bug in QT 4.4.0's QUrl
			entry.link.replace("%40", "@");
			QDesktopServices::openUrl(QUrl(entry.link));
			qDebug() << "opening URL: " << entry.link;
		}
	}
}

