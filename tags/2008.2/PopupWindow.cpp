
#include "PopupWindow.h"
#include <QApplication>
#include <QPainterPath>
#include <QtGui>
#include <cmath>

PopupWindow::PopupWindow(QSystemTrayIcon *owner, QWidget *parent) : QWidget(parent, static_cast<Qt::WindowFlags>(Qt::WA_Hover | Qt::WA_MouseTracking | Qt::Popup | Qt::WA_DeleteOnClose | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint)), m_Owner(owner), m_Direction(Up) {

	ui.setupUi(this);
	
	m_Timer = new QTimer(this);
	m_HideTimer = new QTimer(this);
	m_HideTimer->setSingleShot(true);
	
	connect(m_Timer, SIGNAL(timeout()), this, SLOT(updateLocation()));
	connect(m_HideTimer, SIGNAL(timeout()), this, SLOT(killTimer()));
	connect(ui.btnClose, SIGNAL(clicked()), this, SLOT(close()));
	
	// set the background colour of the widget
	QPalette palette;
	palette.setColor(backgroundRole(), 0xffffe1);
	setPalette(palette);
	
	ui.content->installEventFilter(this);
	ui.title->installEventFilter(this);
}

void PopupWindow::close() {
	emit closed();
	QWidget::close();
}

void PopupWindow::killTimer() {
	if(!underMouse()) {
		close();
	}
}
	
void PopupWindow::mousePressEvent(QMouseEvent *event) {
	if(event->button() == Qt::LeftButton) {
		emit messageClicked();
	}
	close();
}

void PopupWindow::paintEvent(QPaintEvent *event) {
	// draw the border of the windows (thin black line)
	const int border = 1;
	QWidget::paintEvent(event);
    QPixmap pixmap(size());
    QPainter painter2(this);
    painter2.setPen(QPen(Qt::black, border));
    painter2.setBrush(palette().color(QPalette::Window));
    painter2.drawRoundedRect(rect().adjusted(+2, +2, -4, -4), 4, 4);
}

void PopupWindow::resizeEvent(QResizeEvent *event) {
	
	// whenever we are resized, adjust the round-ness of the window
	// to match
	const int border = 1;
    QBitmap bitmap = QBitmap(size());
    bitmap.fill(Qt::color0);
    QPainter painter(&bitmap);
    painter.setPen(QPen(Qt::color1, border));
    painter.setBrush(QBrush(Qt::color1));
    painter.drawRoundedRect(rect().adjusted(+2, +2, -4, -4), 4, 4);
    setMask(bitmap);
	
	QWidget::resizeEvent(event);
}


void PopupWindow::show(const QString &title, const QString &message, int msecs) {

	ui.title->setText(title);
	ui.content->setText(message);
	
	ui.title->adjustSize();
	ui.content->adjustSize();
	adjustSize();
	
	m_HideTimer->setInterval(msecs);
	m_HideTimer->stop();

	const QRect desktopRect = qApp->desktop()->availableGeometry();
	const QRect ownerRect = m_Owner->geometry();

	const int diffs[4] = {
		abs(ownerRect.top() - desktopRect.top()),
		abs(ownerRect.bottom() - desktopRect.bottom()),
		abs(ownerRect.left() - desktopRect.left()),
		abs(ownerRect.right() - desktopRect.right())
		};
		
	int min = diffs[0];
	m_Direction = Down;
	
	for(int i = 1; i < 4; ++i) {
		if(diffs[i] < min) {
			min = diffs[i];
			m_Direction = static_cast<Direction>(i);
		}
	}
	
	int x = 0;
	int y = 0;
	
	switch(m_Direction) {
	case Up:
		x = std::min(desktopRect.width() - width(), ownerRect.left());
		y = desktopRect.height();
		break;
	case Down:
		x = std::min(desktopRect.width() - width(), ownerRect.left());
		y = -height();
		break;
	case Left:
		x = desktopRect.width();
		y = std::min(desktopRect.height() - height(), ownerRect.top());
		break;
	case Right:
		x = -width();
		y = std::min(desktopRect.height() - height(), ownerRect.top());
		break;
	}
	
	move(x, y);
		
	m_Timer->start(1);
	emit animationStart();
	QWidget::show();
}

void PopupWindow::updateLocation() {

	const QRect desktopRect = qApp->desktop()->availableGeometry();
	const QRect ownerRect = m_Owner->geometry();
	
	switch(m_Direction) {
	case Up:
		do {
			const int targetY = (ownerRect.top() - height());
			if(y() > targetY) {
				move(x(), y() - 1);
			} else {
				m_Timer->stop();
				if(m_HideTimer->interval() != 0) {
					m_HideTimer->start();
				}
				emit animationStop();
			}
		} while(0);
		break;
	case Down:
		do {
			const int targetY = (ownerRect.bottom());
			if(y() < targetY) {
				move(x(), y() + 1);
			} else {
				m_Timer->stop();
				if(m_HideTimer->interval() != 0) {
					m_HideTimer->start();
				}
				emit animationStop();
			}
		} while(0);
		break;
	case Left:
		do {
			const int targetX = (ownerRect.left() - width());
			if(x() > targetX) {
				move(x() - 1, y());
			} else {
				m_Timer->stop();
				if(m_HideTimer->interval() != 0) {
					m_HideTimer->start();
				}
				emit animationStop();
			}
		} while(0);
		break;
	case Right:
		do {
			const int targetX = (ownerRect.right());
			if(x() < targetX) {
				move(x() + 1, y());
			} else {
				m_Timer->stop();
				if(m_HideTimer->interval() != 0) {
					m_HideTimer->start();
				}
				emit animationStop();
			}
		} while(0);
		break;
	}
	

}

