#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QVector>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

// 定义扫描点结构体，用于实现余辉效果
struct RadarPoint {
    float angle;
    float dist;
    qint64 timestamp; // 记录产生时间
};

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void openPort();
    void readData();
    void updateAnimation(); // 刷新定时器槽函数

private:
    void drawRadarGrid(QPainter &p);
    void drawScanLine(QPainter &p);
    void drawPoints(QPainter &p);
    void parseLine(const QString &line);

    Ui::Widget *ui;
    QSerialPort *m_port;
    QTimer *m_timer;           // 用于平滑动画刷新
    QTimer *m_reconnectTimer;  // 自动重连

    float m_currentAngle = 0;  // 当前指针角度
    float m_maxRange = 100.0;  // 雷达量程（根据编码器 range 调整）
    QVector<RadarPoint> m_points;

    const int R = 300;         // 雷达绘制半径
};
#endif
