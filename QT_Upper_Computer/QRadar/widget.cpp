#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtMath>
#include <QRegularExpression>

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: #0A0A0A;"); // 深色背景
    setFixedSize(700, 750);

    m_port = new QSerialPort(this);
    m_timer = new QTimer(this);
connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Widget::openPort);
    m_reconnectTimer->start(2000);
}

Widget::~Widget() { delete ui; }

void Widget::openPort() {
    if (m_port->isOpen()) return;
    // 这里加上 QSerialPortInfo
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        m_port->setPort(info);
        // 这里加上 QIODevice::ReadWrite
        if (m_port->open(QIODevice::ReadWrite)) {
            // 这里加上 QSerialPort::Baud115200
            m_port->setBaudRate(QSerialPort::Baud115200);
            connect(m_port, &QSerialPort::readyRead, this, &Widget::readData);
            m_reconnectTimer->stop();
            break;
        }
    }
}

void Widget::readData() {
    while (m_port->canReadLine()) {
        QString line = QString::fromUtf8(m_port->readLine()).trimmed();
        parseLine(line);
    }
}

void Widget::parseLine(const QString &line) {
    // 兼容格式 "A:180,D:50" 或增加 range 的格式
    // 使用正则或简单的字符串切分
    QStringList parts = line.split(',');
    float a = -1, d = -1;

    for (const QString &s : parts) {
        if (s.startsWith("A:")) a = s.mid(2).toFloat();
        else if (s.startsWith("D:")) d = s.mid(2).toFloat();
    }

    if (a != -1) {
        m_currentAngle = a;
        if (d > 2 && d < 500) { // 有效距离过滤
            RadarPoint p;
            p.angle = a;
            p.dist = d;
            p.timestamp = QDateTime::currentMSecsSinceEpoch();
            m_points.append(p);
        }
    }
}

void Widget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(width() / 2, height() - 50); // 将原点移至下方中央

    drawRadarGrid(p);
    drawPoints(p);
    drawScanLine(p);
}

void Widget::drawRadarGrid(QPainter &p) {
    p.setPen(QPen(QColor(0, 100, 0, 150), 1));
    // 画半圆刻度
    for (int i = 1; i <= 5; ++i) {
        int r = (R / 5) * i;
        p.drawArc(-r, -r, 2 * r, 2 * r, 0, 180 * 16);
    }
    // 画放射线
    for (int i = 0; i <= 180; i += 30) {
        float rad = qDegreesToRadians((float)i);
        p.drawLine(0, 0, -R * cos(rad), -R * sin(rad));
    }
}

void Widget::drawPoints(QPainter &p) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = m_points.begin();
    while (it != m_points.end()) {
        qint64 age = now - it->timestamp;
        if (age > 2000) { // 超过 2 秒的点删除
            it = m_points.erase(it);
        } else {
            // 计算透明度实现余辉效应
            int alpha = 255 - (age * 255 / 2000);
            p.setBrush(QColor(0, 255, 0, alpha));
            p.setPen(Qt::NoPen);

            float rad = qDegreesToRadians(it->angle);
            // 假设显示的比例尺，1:1 映射 R
            float displayDist = (it->dist / 50.0) * R; // 50cm 为满量程，可根据 range 变量调整
            float px = -displayDist * cos(rad);
            float py = -displayDist * sin(rad);

            p.drawEllipse(QPointF(px, py), 3, 3);
            ++it;
        }
    }
}

void Widget::drawScanLine(QPainter &p) {
    p.save();
    p.rotate(-(m_currentAngle - 90)); // 修正角度偏移

    // 绘制扫描扇形渐变
    QConicalGradient grad(0, 0, 0);
    grad.setColorAt(0, QColor(0, 255, 0, 100));
    grad.setColorAt(0.1, QColor(0, 255, 0, 0));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawPie(-R, -R, 2 * R, 2 * R, 0, 45 * 16); // 45度宽度的扫描阴影

    // 绘制中心指针线
    p.setPen(QPen(Qt::green, 2));
    p.drawLine(0, 0, 0, -R);
    p.restore();
}
