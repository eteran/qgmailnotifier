######################################################################
# Automatically generated by qmake (2.01a) Fri Jul 25 23:08:19 2008
######################################################################

TEMPLATE = app
TARGET = qgmailnotifier
DEPENDPATH += .
INCLUDEPATH += .
target.path	= /bin/
INSTALLS	+= target 
QT += xml network

# Input
HEADERS += GMailFeed.h QGmailNotifier.h
FORMS += dialog_options.ui
SOURCES += GMailFeed.cpp main.cpp QGmailNotifier.cpp
RESOURCES += QGmailNotifier.qrc
