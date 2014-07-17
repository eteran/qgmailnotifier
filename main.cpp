
#include "QGmailNotifier.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSslSocket>
#include <QTranslator>

//------------------------------------------------------------------------------
// Name: main(int argc, char *argv[])
// Desc: entry point of the application
//------------------------------------------------------------------------------
int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

	// setup organization info so settings go in right place
	QApplication::setOrganizationName("codef00.com");
	QApplication::setOrganizationDomain("codef00.com");
	QApplication::setApplicationName("QGmailNotifier");

	// if system tray's aren't available on the system, then forget it
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(0, QMessageBox::tr("QGmailNotifier"), QMessageBox::tr("I couldn't detect any system tray on this system."));
		return 1;
	}

	// also, we only support the SSL version
	if(!QSslSocket::supportsSsl()) {
		QMessageBox::critical(0, QMessageBox::tr("QGmailNotifier"), QMessageBox::tr("I couldn't detect any SSL supporton this system. Perhaps you need to rebuild QT with SSL support."));
		return 1;
	}

	// we want to stay loaded, even when all windows are closed
	QApplication::setQuitOnLastWindowClosed(false);

	// load some translations
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load("qgmailnotifier_" + QLocale::system().name());
	app.installTranslator(&myappTranslator);

	// start it up
	QGmailNotifier gmail;

	return app.exec();
}
