
#include "QGmailNotifier.h"
#include <QMenu>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QSettings>
#include <QPainter>
#include "version.h"

//#define USE_NEW_POPUP

//------------------------------------------------------------------------------
// Name: QGmailNotifier(QWidget *parent, Qt::WindowFlags f)
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

#ifdef USE_NEW_POPUP
	m_PopupWindow = new PopupWindow(trayIcon, this);
	m_PopupWindow->resize(300, 100);
	connect(m_PopupWindow, SIGNAL(messageClicked()), this, SLOT(messageClicked()));
#else
	connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));
#endif
	
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
// Name: doAnim()
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
// Name: doCompose()
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::doCompose() {
	openURL("http://mail.google.com/mail/#compose");
}


//------------------------------------------------------------------------------
// Name: readConversation()
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::readConversation() {
	QAction *const action = qobject_cast<QAction *>(sender());
	const int index = action->data().toInt();
	readMessage(index);
}

//------------------------------------------------------------------------------
// Name: createMenu()
// Desc: 
//------------------------------------------------------------------------------
QMenu *QGmailNotifier::createMenu() {
	QMenu *const menu = new QMenu(this);
	QAction *const view = menu->addAction(tr("View Inbox"), this, SLOT(doView()));
	menu->addAction(tr("Compose a Message"), this, SLOT(doCompose()));
	menu->addAction(tr("Check Mail Now"), this, SLOT(doCheck()));
	menu->addAction(tr("Tell me Again..."), this, SLOT(doTell()));
	menu->addAction(tr("Options"), this, SLOT(doOptions()));
	menu->addAction(tr("About"), this, SLOT(doAbout()));
	
	if(m_CurrentMails.size() != 0) {

		menu->addSeparator();
		QMenu *const convsations = menu->addMenu(tr("Unread Conversations"));
		
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
	menu->addAction(tr("Exit"), qApp, SLOT(quit()));
	
	menu->setDefaultAction(view);
	return menu;
}

//------------------------------------------------------------------------------
// Name: doCheck()
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
		trayIcon->setToolTip(tr("Reading feed"));
		m_GmailFeed->fetch(user, pass);
	}
}

//------------------------------------------------------------------------------
// Name: showEvent(QShowEvent *event)
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
// Name: accept()
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
// Name: doView()
// Desc: loads the gmail main page in the user's default browser
//------------------------------------------------------------------------------
void QGmailNotifier::doView() {
	openURL("http://mail.google.com/mail");
}

//------------------------------------------------------------------------------
// Name: activated(QSystemTrayIcon::ActivationReason reason)
// Desc: lets us know when the user has double clicked the tray icon
//------------------------------------------------------------------------------
void QGmailNotifier::activated(QSystemTrayIcon::ActivationReason reason) {
	if(reason == QSystemTrayIcon::DoubleClick) {
		doView();
	}
}

//------------------------------------------------------------------------------
// Name: doTell()
// Desc: tells the user about new mails, even if they are all "old news"
//------------------------------------------------------------------------------
void QGmailNotifier::doTell() {
	m_Seen.clear();
	m_AlertIndex = -1;
	if(m_CurrentMails.size() == 0) {
		QSettings settings;
		const int time = settings.value("popup_time_span").value<int>();
		
#ifdef USE_NEW_POPUP
		m_PopupWindow->show("", tr("Your inbox contains no unread conversations."), time);
#else
		trayIcon->showMessage("", "Your inbox contains no unread conversations." , QSystemTrayIcon::Information, time);
#endif
	} else {
		doTellReal();
	}
}

//------------------------------------------------------------------------------
// Name: doTellReal()
// Desc: actual implemenation of tell
//------------------------------------------------------------------------------
void QGmailNotifier::doTellReal() {
	QSettings settings;
	const int time = settings.value("popup_time_span").value<int>();

	m_AlertIndex = 0;
	GMailEntry entry = m_CurrentMails[0];

	//foreach(GMailEntry entry, m_CurrentMails) {
		if(!m_Seen.contains(entry.id)) {
#ifdef USE_NEW_POPUP
			m_PopupWindow->show(entry.author_name + " : " + entry.title, entry.summary, time);
#else
			trayIcon->showMessage(entry.author_name + " : " + entry.title, entry.summary, QSystemTrayIcon::Information, time);
#endif
			m_Seen.insert(entry.id);
		}
	//	++m_AlertIndex;
	//}
}

//------------------------------------------------------------------------------
// Name: doOptions()
// Desc: shows the options dialog
//------------------------------------------------------------------------------
void QGmailNotifier::doOptions() {
	show();
}

//------------------------------------------------------------------------------
// Name: doAbout()
// Desc: shows the about dialog
//------------------------------------------------------------------------------
void QGmailNotifier::doAbout() {
	m_AlertIndex = -1;
	
#ifdef USE_NEW_POPUP
	m_PopupWindow->show(
		tr("QGmailNotifier"), 
		QString(tr("QGmailNotifier version %1. Written by <a href='mailto:eteran@alum.rit.edu'>Evan Teran</a><br>Icons are the trademark of <a href='http://www.google.com/'>Google</a><br><br>"
		"Google, Gmail and Google Mail are registered trademarks of Google Inc.<br>"
		"QGmailNotifier nor its author are in any way affiliated nor endorsed by Google Inc.<br>")).arg(version),
		0);
#else
	trayIcon->showMessage(
		tr("QGmailNotifier"), 
		QString(tr("QGmailNotifier version %1. Written by Evan Teran\nIcons are the trademark of Google\n\n"
		"Google, Gmail and Google Mail are registered trademarks of Google Inc.\n"
		"QGmailNotifier nor its author are in any way affiliated nor endorsed by Google Inc.\n")).arg(version),
		QSystemTrayIcon::Information,
		0);			
#endif
}

//------------------------------------------------------------------------------
// Name: fetchComplete(bool error)
// Desc: lets us know that the main module is finished fetching
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
			trayIcon->setToolTip(QString(tr("%1 unread conversation")).arg(m_CurrentMails.size()));
		} else {
			trayIcon->setToolTip(QString(tr("%1 unread conversations")).arg(m_CurrentMails.size()));
		}

		m_AlertIndex = -1;

		if(m_CurrentMails.size() != 0) {
			doTellReal();
#if 0
			QFont f;
			f.setPixelSize(14);
			f.setWeight(QFont::Black);
									
			QPixmap pm(":/img/blue.ico");
			QPainter p(&pm);
			
			p.setFont(f);
			p.setPen(Qt::black);
			p.drawText(0, 0, pm.width(), pm.height(), Qt::AlignCenter, QString::number(m_CurrentMails.size()));
			
			trayIcon->setIcon(pm);
#else
			trayIcon->setIcon(QIcon(":/img/blue.ico"));
#endif
		} else {
			trayIcon->setIcon(QIcon(":/img/normal.ico"));
		}
		
		trayIcon->setContextMenu(createMenu());
	}
}

//------------------------------------------------------------------------------
// Name: openURL(const QString &url)
// Desc: opens a URL using the default browser
//------------------------------------------------------------------------------
void QGmailNotifier::openURL(const QString &url) {
	QDesktopServices::openUrl(QUrl::fromEncoded(url.toAscii()));
	qDebug() << "opening URL: " << url;
}

//------------------------------------------------------------------------------
// Name: messageClicked()
// Desc: the user has clicked on our message
//------------------------------------------------------------------------------
void QGmailNotifier::messageClicked() {
	readMessage(m_AlertIndex);
}

//------------------------------------------------------------------------------
// Name: readMessage(int index)
// Desc: 
//------------------------------------------------------------------------------
void QGmailNotifier::readMessage(int index) {
	if(index != -1) {
		if(index < m_CurrentMails.size()) {
			GMailEntry entry = m_CurrentMails[index];
			// work around a bug in QT 4.4.0's QUrl
			//entry.link.replace("%40", "@");
		
			openURL(entry.link);
		}
	}
}

