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
    explicit LaserChart(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setYRange(int min, int max);
    void setTimeWindow(int seconds);
    void setLineColor(const QColor &c);          // 设定值曲线颜色
    void setMeasuredColor(const QColor &c);      // 实测值曲线颜色
    void setThreshold(int value, bool enabled = true); // 阈值线，<=0 表示不画
    void addDataPoint(int current_mA);           // 设定值
    void addMeasuredPoint(double current_mA);    // 实测值
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
