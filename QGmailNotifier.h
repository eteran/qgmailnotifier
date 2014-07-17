
#ifndef QGMAILNOTIFIER_20080725_H_
#define QGMAILNOTIFIER_20080725_H_

#include <QSystemTrayIcon>
#include <QDialog>
#include <QTimer>
#include <QVector>
#include <QSet>
#include "GMailFeed.h"
#include "ui_dialog_options.h"

class QGmailNotifier : public QDialog {
	Q_OBJECT
	
public:
	QGmailNotifier(QWidget *parent = 0, Qt::WindowFlags f = 0);

private slots:
	void doCheck();
	void doView();
	void doTell();
	void doTellReal();
	void doOptions();
	void doAbout();
	void doAnim();
	void doCompose();
	
	void activated(QSystemTrayIcon::ActivationReason reason);
	void fetchComplete(bool);
	void messageClicked();
	void readConversation();
	
private:
	virtual void accept();
	virtual void showEvent(QShowEvent *event);
	
private:
	QMenu *createMenu();
	void readMessage(int index);

private:
	QSystemTrayIcon		*trayIcon;
	QTimer				*m_Timer;
	QTimer				*m_AnimTimer;
	GMailFeed			*m_GmailFeed;
	int					m_AlertIndex;
	int					m_AnimIndex;
	QVector<GMailEntry>	m_CurrentMails;
	QSet<QString>		m_Seen;
	Ui::DialogOptions	ui;
	
	
};

#endif

