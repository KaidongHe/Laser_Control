#ifndef LASERCHART_H
#define LASERCHART_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QColor>

class LaserChart : public QWidget
{
    Q_OBJECT

public:
    explicit LaserChart(QWidget *parent = 0);
    ~LaserChart();

    void setTitle(const QString &title);
    void setYRange(int min, int max);
    void setTimeWindow(int seconds);
    void setLineColor(const QColor &c);
    void setMeasuredColor(const QColor &c);
    void setThreshold(int value, bool enabled = true);
    void addDataPoint(int current_mA);
    void addMeasuredPoint(double current_mA);
    void reset();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QVector<QPointF> m_data;           // setpoint
    QVector<QPointF> m_measured;       // measured
    int m_yMin = 0;
    int m_yMax = 1000;
    int m_timeWindowSec = 60;
    QColor m_lineColor = QColor(0, 220, 100);
    QColor m_measuredColor = QColor(255, 200, 40);
    int m_threshold = -1;
    bool m_thresholdEnabled = false;
};

#endif // LASERCHART_H
