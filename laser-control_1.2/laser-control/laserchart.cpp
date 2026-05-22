#include "laserchart.h"
#include <QPainter>
#include <QDateTime>
#include <QtMath>

LaserChart::LaserChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void LaserChart::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void LaserChart::setYRange(int min, int max)
{
    m_yMin = min;
    m_yMax = max;
    update();
}

void LaserChart::setTimeWindow(int seconds)
{
    m_timeWindowSec = seconds;
    update();
}

void LaserChart::setLineColor(const QColor &c)
{
    m_lineColor = c;
    update();
}

void LaserChart::setMeasuredColor(const QColor &c)
{
    m_measuredColor = c;
    update();
}

void LaserChart::setThreshold(int value, bool enabled)
{
    m_threshold = value;
    m_thresholdEnabled = enabled && value > 0;
    update();
}

void LaserChart::addDataPoint(int current_mA)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_data.append(QPointF(now, current_mA));
    while (m_data.size() > 600)
        m_data.removeFirst();
    update();
}

void LaserChart::addMeasuredPoint(double current_mA)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_measured.append(QPointF(now, current_mA));
    while (m_measured.size() > 600)
        m_measured.removeFirst();
    update();
}

void LaserChart::reset()
{
    m_data.clear();
    m_measured.clear();
    update();
}

void LaserChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();


    p.fillRect(rect(), QColor(30, 30, 30));

    int marginLeft = 55;
    int marginRight = 15;
    int marginTop = 22;
    int marginBottom = 26;
    int plotW = w - marginLeft - marginRight;
    int plotH = h - marginTop - marginBottom;

    if (plotW <= 0 || plotH <= 0) return;


    p.setPen(Qt::white);
    QFont titleFont = font();
    titleFont.setPointSize(9);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(0, 2, w, marginTop - 4, Qt::AlignHCenter | Qt::AlignVCenter, m_title);


    QFont axisFont = font();
    axisFont.setPointSize(7);
    p.setFont(axisFont);

    int ySteps = 4;
    for (int i = 0; i <= ySteps; i++) {
        int y = marginTop + plotH * i / ySteps;
        p.setPen(QColor(60, 60, 60));
        p.drawLine(marginLeft, y, marginLeft + plotW, y);

        int val = m_yMax - (m_yMax - m_yMin) * i / ySteps;
        p.setPen(QColor(180, 180, 180));
        p.drawText(0, y - 8, marginLeft - 4, 16, Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
    }

    // Y轴单位
    p.setPen(QColor(180, 180, 180));
    p.drawText(2, 2, 50, 14, Qt::AlignLeft, "mA");

    // 阈值线
    if (m_thresholdEnabled && m_threshold > m_yMin && m_threshold < m_yMax) {
        double frac = (m_threshold - m_yMin) / double(m_yMax - m_yMin);
        int ty = marginTop + plotH - int(frac * plotH);
        QPen thPen(QColor(255, 80, 80, 180), 1, Qt::DashLine);
        p.setPen(thPen);
        p.drawLine(marginLeft, ty, marginLeft + plotW, ty);
        p.setPen(QColor(255, 120, 120));
        p.drawText(marginLeft + 4, ty - 12, 80, 12,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("阈值 %1").arg(m_threshold));
    }


    if (m_data.isEmpty() && m_measured.isEmpty()) {
        p.setPen(QColor(120, 120, 120));
        QFont hintFont = font();
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.drawText(marginLeft, marginTop, plotW, plotH, Qt::AlignCenter, "等待数据...");
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 windowStart = now - m_timeWindowSec * 1000LL;


    p.setFont(axisFont);
    int xSteps = 4;
    for (int i = 0; i <= xSteps; i++) {
        qint64 t = windowStart + m_timeWindowSec * 1000LL * i / xSteps;
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(t);
        QString label = dt.toString("mm:ss");
        int x = marginLeft + plotW * i / xSteps;
        p.setPen(QColor(180, 180, 180));
        p.drawText(x - 25, marginTop + plotH + 2, 50, marginBottom - 4, Qt::AlignHCenter, label);
    }

    p.setClipRect(marginLeft, marginTop, plotW, plotH);

    auto drawSeries = [&](const QVector<QPointF> &series, const QColor &color, Qt::PenStyle style, double penWidth) {
        if (series.isEmpty()) return;
        int startIdx = 0;
        for (; startIdx < series.size(); startIdx++) {
            if (series[startIdx].x() >= windowStart) break;
        }
        if (startIdx >= series.size()) return;

        p.setPen(QPen(color, penWidth, style));
        bool first = true;
        int lastX = 0, lastY = 0;
        for (int i = startIdx; i < series.size(); i++) {
            double xFrac = (series[i].x() - windowStart) / double(m_timeWindowSec * 1000LL);
            xFrac = qBound(0.0, xFrac, 1.0);
            int px = marginLeft + int(xFrac * plotW);
            double yFrac = (series[i].y() - m_yMin) / double(m_yMax - m_yMin);
            yFrac = qBound(0.0, yFrac, 1.0);
            int py = marginTop + plotH - int(yFrac * plotH);
            if (first) first = false;
            else p.drawLine(lastX, lastY, px, py);
            lastX = px;
            lastY = py;
        }
    };


    drawSeries(m_data, m_lineColor, Qt::DashLine, 1.5);

    drawSeries(m_measured, m_measuredColor, Qt::SolidLine, 2.0);

    p.setClipping(false);


    QFont valFont = font();
    valFont.setPointSize(8);
    valFont.setBold(true);
    p.setFont(valFont);

    int textY = marginTop + 2;
    if (!m_data.isEmpty()) {
        p.setPen(m_lineColor);
        QString s = QString("设定 %1").arg(int(m_data.last().y()));
        p.drawText(marginLeft, textY, plotW - 4, 14, Qt::AlignRight, s);
        textY += 14;
    }
    if (!m_measured.isEmpty()) {
        p.setPen(m_measuredColor);
        QString s = QString("实测 %1").arg(m_measured.last().y(), 0, 'f', 0);
        p.drawText(marginLeft, textY, plotW - 4, 14, Qt::AlignRight, s);
    }
}
