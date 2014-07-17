
#ifndef QGMAILNOTIFIER_20080725_H_
#define QGMAILNOTIFIER_20080725_H_

#include "GMailFeed.h"
#include <QDialog>
#include <QSet>
#include <QSystemTrayIcon>
#include <QVector>
#include "ui_dialog_options.h"

class QTimer;

class QGmailNotifier : public QDialog {
	Q_OBJECT

public:
	QGmailNotifier(QWidget *parent = 0, Qt::WindowFlags f = 0);

private Q_SLOTS:
	void doCheck();
	void doView();
	void doTell();
	void doTellReal();
	void doOptions();
	void doAbout();
	void doAnim();
	void doCompose();

	void activated(QSystemTrayIcon::ActivationReason reason);
	void fetchComplete(QNetworkReply *reply);
	void readConversation();
	void messageClicked();

private:
	virtual void accept();
	virtual void showEvent(QShowEvent *event);

private:
	QMenu *createMenu();
	void readMessage(int index);
	void openURL(const QString &url);

private:
	QSystemTrayIcon     *trayIcon_;
	QTimer              *timer_;
	QTimer              *animationTimer_;
	GMailFeed           *gmailFeed_;
	int                 alertIndex_;
	int                 animationIndex_;
	QVector<GMailEntry> currentMails_;
	QSet<QString>       seen_;
	Ui::DialogOptions   ui;

};

#endif

