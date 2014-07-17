
#include "QGmailNotifier.h"
#include <QApplication>
#include <QtGui>

int main(int argc, char *argv[]) {

	Q_INIT_RESOURCE(QGmailNotifier);

	QApplication app(argc, argv);
	
	// setup organization info so settings go in right place
	QApplication::setOrganizationName("codef00.com");
	QApplication::setOrganizationDomain("codef00.com");
	QApplication::setApplicationName("QGmailNotifier");

	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(0, QObject::tr("QGmailNotifier"), QObject::tr("I couldn't detect any system tray on this system."));
		return 1;
	}

	QApplication::setQuitOnLastWindowClosed(false);

	QGmailNotifier gmail;
	//gmail.show();

	return app.exec();
}
