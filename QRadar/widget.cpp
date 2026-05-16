#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtMath>
#include <QPen>
#include <QBrush>
#include <QConicalGradient>
#include <QRadialGradient>

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    setStyleSheet("background-color: #050505;");
    setFixedSize(960, 960);
    setWindowTitle("STM32 Ultrasonic Radar");

    m_port = new QSerialPort(this);
    connect(m_port, &QSerialPort::readyRead, this, &Widget::readData);
    // Qt 5.4 uses "error" signal (renamed to "errorOccurred" in Qt 5.8)
    connect(m_port,
            static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, [this](QSerialPort::SerialPortError err) {
        if (err != QSerialPort::NoError) {
            m_probeTimer->stop();
            m_connected = false;
            if (!m_currentProbePort.isEmpty())
                m_failedPorts.insert(m_currentProbePort);
            if (m_port->isOpen())
                m_port->close();
            QTimer::singleShot(0, this, &Widget::tryConnect);
        }
    });

    // 60fps animation timer for smooth scan line
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &Widget::animate);
    m_animTimer->start(16); // ~60fps

    // Port probe: 1.5s single-shot, fires if no valid radar data arrives
    m_probeTimer = new QTimer(this);
    m_probeTimer->setSingleShot(true);
    connect(m_probeTimer, &QTimer::timeout, this, &Widget::onProbeTimeout);

    // Auto-reconnect every 2 seconds
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Widget::tryConnect);
    m_reconnectTimer->start(2000);

    tryConnect();
}

Widget::~Widget() { delete ui; }

void Widget::setPreferredPort(const QString &port)
{
    if (!port.isEmpty())
        m_preferredPort = port;
}

// ─── Serial Port ───────────────────────────────────────────────

void Widget::tryConnect()
{
    if (m_port->isOpen()) return;      // already connected / probing

    m_connected = false;
    m_probeCount = 0;
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        m_failedPorts.clear();
        m_blockedPorts.clear();
        return;
    }

    // Only connect to preferred port
    m_failedPorts.clear();
    m_blockedPorts.clear();
    QString target = m_preferredPort.isEmpty() ? "COM8" : m_preferredPort;
    for (const QSerialPortInfo &info : ports) {
        if (info.portName() == target) {
            m_port->setPort(info);
            if (m_port->open(QIODevice::ReadWrite)) {
                m_port->setBaudRate(QSerialPort::Baud115200);
                m_currentProbePort = target;
                m_probeTimer->start(1500);
                return;
            }
        }
    }
}

void Widget::readData()
{
    while (m_port->canReadLine()) {
        QString line = QString::fromUtf8(m_port->readLine()).trimmed();
        parseLine(line);
    }
}

void Widget::parseLine(const QString &line)
{
    // Expected format: "angle,distance,range"  (e.g. "90,45,3")
    QStringList parts = line.split(',');
    if (parts.size() != 3) return;

    bool okA, okD, okR;
    float a = parts[0].toFloat(&okA);
    float d = parts[1].toFloat(&okD);
    int   r = parts[2].toInt(&okR);

    // Strict validation: all fields must parse, angle 0-180, range 1-10
    if (!okA || !okD || !okR) return;
    if (a < 0 || a > 180) return;
    if (r < 1 || r > 10)  return;

    // ── Probe confirmation: need 3 valid lines before locking onto this port ──
    if (!m_connected) {
        m_probeCount++;
        if (m_probeCount >= 3) {
            m_connected = true;
            m_probeTimer->stop();
            m_reconnectTimer->stop();
        }
    }

    m_targetAngle = 180.0f - a;
    m_range = r;
    m_maxRange = r * 64.0f;

    // Filter: distance must be valid (2cm < d < maxRange)
    // STM32 sends 500/505/510 for invalid readings
    if (d > 2 && d < m_maxRange && d < 500) {
        m_curDist = d;

        // Update closest threat in current sweep
        if (d < m_sweepMinDist) {
            m_sweepMinDist = d;
            m_sweepMinAngle = m_targetAngle;
            m_closestDist = d;
            m_closestAngle = m_targetAngle;
        }

        RadarPoint pt;
        pt.angle = 180.0f - a;
        pt.dist = d;
        pt.timestamp = QDateTime::currentMSecsSinceEpoch();
        m_points.append(pt);
    }

    // ── Sweep edge detection: clear threat when scan reaches 0° or 180° ──
    static constexpr float EDGE = 4.0f;  // edge zone width in degrees
    bool atEdge = (m_targetAngle <= EDGE || m_targetAngle >= 180.0f - EDGE);
    if (atEdge && !m_nearEdge) {
        m_sweepMinDist = 9999.0f;
        m_closestAngle = -1;
        m_closestDist = 0;
    }
    m_nearEdge = atEdge;
}

// ─── Port Probe ────────────────────────────────────────────────

void Widget::onProbeTimeout()
{
    // No valid radar data arrived within 1.5s → wrong port
    m_failedPorts.insert(m_currentProbePort);
    if (m_port->isOpen())
        m_port->close();
    tryConnect();  // try the next available port
}

// ─── Animation ─────────────────────────────────────────────────

void Widget::animate()
{
    // Smooth follow: exponentially move scan angle towards target
    float diff = m_targetAngle - m_scanAngle;
    // Handle angle wrap at boundaries (0° ↔ 180°)
    if (diff > 90)  diff -= 180;
    if (diff < -90) diff += 180;
    m_scanAngle += diff * 0.35f;
    update();
}

// ─── Paint ─────────────────────────────────────────────────────

void Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Origin at bottom-center of radar area
    p.translate(width() / 2, height() - MARGIN_BTM);

    drawBackground(p);
    drawGrid(p);
    drawPoints(p);
    drawScanLine(p);

    // Reset transform for screen-space status overlay
    p.resetTransform();
    drawStatus(p);
}

// ─── Background ────────────────────────────────────────────────

void Widget::drawBackground(QPainter &p)
{
    // Subtle radial glow around origin
    QRadialGradient glow(0, 0, R * 1.2);
    glow.setColorAt(0.0, QColor(0, 30, 0, 50));
    glow.setColorAt(0.5, QColor(0, 10, 0, 15));
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setBrush(glow);
    p.setPen(Qt::NoPen);
    p.drawRect(-width(), -height(), 2 * width(), 2 * height());
}

// ─── Grid ──────────────────────────────────────────────────────

void Widget::drawGrid(QPainter &p)
{
    // ── Concentric range arcs ──
    QColor gridColor(0, 140, 0, 100);
    p.setPen(QPen(gridColor, 1, Qt::DashLine));

    for (int i = 1; i <= 5; ++i) {
        int r = (R / 5) * i;
        p.drawArc(QRect(-r, -r, 2 * r, 2 * r), 0, 180 * 16);
    }

    // ── Radial angle lines ──
    QColor radialColor(0, 120, 0, 80);
    p.setPen(QPen(radialColor, 1));

    for (int deg = 0; deg <= 180; deg += 30) {
        float rad = qDegreesToRadians(static_cast<float>(deg));
        int ex = static_cast<int>(-R * cos(rad));
        int ey = static_cast<int>(-R * sin(rad));
        p.drawLine(0, 0, ex, ey);
    }

    // ── Scale labels (on the 90° center line) ──
    p.save();
    p.resetTransform();
    p.translate(width() / 2, height() - MARGIN_BTM);

    QFont labelFont("Consolas", 8);
    p.setFont(labelFont);
    p.setPen(QColor(0, 160, 0, 180));

    for (int i = 1; i <= 5; ++i) {
        float r = (R / 5.0f) * i;
        float distCm = (m_maxRange / 5.0f) * i;

        // Show label near the center vertical line (angle ~90°), offset slightly right
        QString text;
        if (distCm >= 100)
            text = QString("%1m").arg(distCm / 100.0, 0, 'f', 1);
        else
            text = QString("%1cm").arg(static_cast<int>(distCm));

        // Draw label at (5, -r + labelOffset) — slightly to the right of center
        p.drawText(QRectF(5, -r - 8, 50, 16), Qt::AlignLeft | Qt::AlignVCenter, text);

        // Draw small tick mark
        p.setPen(QPen(QColor(0, 160, 0, 120), 1));
        p.drawLine(-4, -r, 4, -r);
        p.setPen(QColor(0, 160, 0, 180));
    }

    // ── Angle labels ──
    QFont angleFont("Consolas", 9);
    p.setFont(angleFont);
    p.setPen(QColor(0, 180, 0, 200));

    for (int deg = 0; deg <= 180; deg += 30) {
        float rad = qDegreesToRadians(static_cast<float>(deg));
        float lr = R + 18;
        float lx = -lr * cos(rad);
        float ly = -lr * sin(rad);

        QString text = QString("%1°").arg(deg);

        // Adjust text position based on angle for better readability
        if (deg == 0) {
            p.drawText(QRectF(lx - 50, ly - 8, 50, 16), Qt::AlignRight | Qt::AlignVCenter, text);
        } else if (deg == 180) {
            p.drawText(QRectF(lx + 2, ly - 8, 50, 16), Qt::AlignLeft | Qt::AlignVCenter, text);
        } else if (deg < 90) {
            p.drawText(QRectF(lx - 50, ly - 20, 50, 16), Qt::AlignRight | Qt::AlignBottom, text);
        } else if (deg > 90) {
            p.drawText(QRectF(lx + 2, ly - 20, 50, 16), Qt::AlignLeft | Qt::AlignBottom, text);
        } else {
            // deg == 90 (straight up)
            p.drawText(QRectF(lx - 25, ly - 26, 50, 16), Qt::AlignCenter, text);
        }
    }

    p.restore();
}

// ─── Points (Afterglow) ────────────────────────────────────────

void Widget::drawPoints(QPainter &p)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    auto it = m_points.begin();
    while (it != m_points.end()) {
        qint64 age = now - it->timestamp;

        // Remove points older than 2.5 seconds
        if (age > 2500) {
            it = m_points.erase(it);
            continue;
        }

        // Afterglow: alpha fades from 255 (fresh) to 0 (old)
        int alpha = 255 - static_cast<int>(age * 255 / 2500);
        // Size also shrinks with age
        float size = 3.5f - (age * 2.5f / 2500); // 3.5 → 1.0

        // Core brightness
        int green = 100 + (age * 100 / 2500); // fades from 200 to 100

        p.setPen(Qt::NoPen);

        // Outer glow (larger, more transparent)
        QColor glowC(0, green + 55, 0, alpha / 3);
        p.setBrush(glowC);
        float rad = qDegreesToRadians(it->angle);
        float displayDist = (it->dist / m_maxRange) * R;
        float px = -displayDist * cos(rad);
        float py = -displayDist * sin(rad);
        p.drawEllipse(QPointF(px, py), size + 2, size + 2);

        // Core point
        QColor coreC(0, 255, 0, alpha);
        p.setBrush(coreC);
        p.drawEllipse(QPointF(px, py), size, size);

        ++it;
    }

    // ── Closest threat marker (red cross) ──
    if (m_closestAngle >= 0) {
        float rad = qDegreesToRadians(m_closestAngle);
        float displayDist = (m_closestDist / m_maxRange) * R;
        float cx = -displayDist * cos(rad);
        float cy = -displayDist * sin(rad);

        // Pulsing outer ring
        int pulse = static_cast<int>((QDateTime::currentMSecsSinceEpoch() % 800) * 255 / 800);
        QColor ringC(255, 0, 0, 100 + pulse / 4);
        p.setPen(Qt::NoPen);
        p.setBrush(ringC);
        p.drawEllipse(QPointF(cx, cy), 12, 12);

        // Inner core
        p.setBrush(QColor(255, 0, 0, 255));
        p.drawEllipse(QPointF(cx, cy), 4, 4);

        // Crosshair lines
        p.setPen(QPen(QColor(255, 0, 0, 200), 2));
        p.drawLine(QPointF(cx - 9, cy), QPointF(cx + 9, cy));
        p.drawLine(QPointF(cx, cy - 9), QPointF(cx, cy + 9));
    }
}

// ─── Scan Line ─────────────────────────────────────────────────

void Widget::drawScanLine(QPainter &p)
{
    p.save();

    // Rotate so "up" (0, -R) aligns with the scan angle
    // m_scanAngle=0 → left, 90 → top, 180 → right of display
    p.rotate(m_scanAngle - 90);

    // ── Radar beam fan ──
    // In the rotated frame, the scan line points UP (= 90° in Qt arc coords).
    // The fan is a symmetric pie slice centered on 90°.
    int fanHalf = 18; // half-width in degrees
    int qtStartAngle = 90 - fanHalf;       // 72° → leading edge
    int qtSpanAngle  = fanHalf * 2;        // 36° total

    // Conical gradient: starts at the leading edge, wraps 360° CCW.
    // Only the pie region (72°–108°) is visible.
    QConicalGradient grad(0, 0, qtStartAngle);
    grad.setColorAt(0.0,  QColor(0, 255, 0, 5));   // leading edge (faint)
    grad.setColorAt(0.05, QColor(0, 255, 0, 35));   // beam core
    grad.setColorAt(0.1,  QColor(0, 255, 0, 5));   // trailing edge (faint)
    grad.setColorAt(1.0,  QColor(0, 0, 0, 0));     // rest (hidden)

    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawPie(QRect(-R, -R, 2 * R, 2 * R), qtStartAngle * 16, qtSpanAngle * 16);

    // ── Bright scan line ──
    p.setPen(QPen(QColor(0, 255, 0, 220), 2));
    p.drawLine(0, 0, 0, -R);

    // ── Center hub ──
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 255, 0, 230));
    p.drawEllipse(QPointF(0, 0), 5, 5);
    p.setBrush(QColor(0, 60, 0, 200));
    p.drawEllipse(QPointF(0, 0), 3, 3);

    p.restore();
}

// ─── Status Overlay ────────────────────────────────────────────

void Widget::drawStatus(QPainter &p)
{
    QFont statusFont("Consolas", 11);
    p.setFont(statusFont);

    int y = 30;
    int xLeft = 12;
    int lineH = 22;

    // Connection indicator
    if (m_connected) {
        p.setPen(QColor(0, 255, 0, 220));
        p.drawText(xLeft, y, QString("Connected  %1").arg(m_currentProbePort));
    } else if (m_port->isOpen()) {
        p.setPen(QColor(255, 200, 0, 200));
        p.drawText(xLeft, y, QString("Probing %1  (%2/3)")
                   .arg(m_currentProbePort)
                   .arg(m_probeCount));
    } else {
        p.setPen(QColor(255, 80, 80, 200));
        p.drawText(xLeft, y, "No COM port - retrying...");
    }

    y += lineH + 4;

    // Range
    p.setPen(QColor(0, 200, 0, 200));
    p.drawText(xLeft, y, QString("Range:  %1").arg(m_range));

    y += lineH;

    // Current angle & distance
    p.drawText(xLeft, y, QString("Angle:  %1°").arg(m_targetAngle, 0, 'f', 0));
    y += lineH;
    p.drawText(xLeft, y, QString("Dist:   %1 cm").arg(m_curDist, 0, 'f', 1));

    // Closest threat target (top-right, always shown)
    {
        QFont threatFont("Consolas", 12, QFont::Bold);
        p.setFont(threatFont);
        if (m_closestAngle >= 0) {
            p.setPen(QColor(255, 60, 60, 220));
            p.drawText(width() - 310, y + 30, 300, 24, Qt::AlignRight | Qt::AlignVCenter,
                       QString("THREAT %1cm angle %2°")
                       .arg(static_cast<int>(m_closestDist))
                       .arg(m_closestAngle, 0, 'f', 0));
        } else {
            p.setPen(QColor(255, 255, 0, 180));
            p.drawText(width() - 310, y + 30, 300, 24, Qt::AlignRight | Qt::AlignVCenter,
                       "THREAT: --");
        }
    }
}
