#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QVector>
#include <QSet>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

struct RadarPoint {
    float angle;
    float dist;
    qint64 timestamp;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void setPreferredPort(const QString &port);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void tryConnect();
    void readData();
    void animate();
    void onProbeTimeout();

private:
    void drawBackground(QPainter &p);
    void drawGrid(QPainter &p);
    void drawPoints(QPainter &p);
    void drawScanLine(QPainter &p);
    void drawStatus(QPainter &p);
    void parseLine(const QString &line);

    Ui::Widget *ui;
    QSerialPort *m_port;
    QTimer *m_animTimer;       // 60fps smooth animation
    QTimer *m_reconnectTimer;  // auto-reconnect every 2s
    QTimer *m_probeTimer;      // single-shot: wait for valid radar data after port open

    float m_scanAngle = 90;     // animated scan line angle (smooth)
    float m_targetAngle = 90;   // latest angle from STM32 data
    float m_maxRange = 64;      // max display range in cm (range * 64)
    int m_range = 1;            // current range value from STM32
    float m_curDist = 0;        // current distance reading
    bool m_connected = false;

    // Closest threat target in current sweep
    float m_closestAngle = -1;
    float m_closestDist = 0;
    float m_sweepMinDist = 9999;
    float m_sweepMinAngle = 0;
    float m_prevTargetAngle = 90;
    int m_sweepDir = 0;             // 1=increasing, -1=decreasing
    bool m_nearEdge = false;        // true when scan angle is within edge zone
    QString m_preferredPort;    // user-specified COM port (via command line)
    int m_probeCount = 0;           // valid lines received on current probe port
    QSet<QString> m_failedPorts;    // ports that opened but sent no radar data
    QSet<QString> m_blockedPorts;   // ports that failed to open at all
    QString m_currentProbePort;     // port currently being probed
    QString m_rawLine;              // last raw line for debug display

    QVector<RadarPoint> m_points;

    static constexpr int R = 400;          // radar radius in pixels
    static constexpr int MARGIN_BTM = 85;  // bottom margin
};
#endif
