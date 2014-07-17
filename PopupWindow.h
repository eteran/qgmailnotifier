
#ifndef POPUPWINDOW_20080726_H_
#define POPUPWINDOW_20080726_H_

#include <QWidget>
#include <QTimer>
#include "ui_widget_popup.h"

class QSystemTrayIcon;

class PopupWindow : public QWidget {
	Q_OBJECT
	
public:
	PopupWindow(QSystemTrayIcon *owner, QWidget *parent = 0);
	
public slots:
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void show(const QString &title, const QString &message, int msecs = 0);
	void updateLocation();
	void close();
	void killTimer();
	
signals:
	void messageClicked();
	void closed();
	void animationStart();
	void animationStop();
	
private:
	enum Direction {
		Down = 0,
		Up,
		Right,
		Left,
	};
	
	QTimer				*m_Timer;
	QTimer				*m_HideTimer;
	QSystemTrayIcon		*m_Owner;
	Direction			m_Direction;
	Ui::PopupWindow		ui;
};

#endif

