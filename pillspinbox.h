#ifndef PILLSPINBOX_H
#define PILLSPINBOX_H

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QSpinBox>

class QPainter;
class QEvent;
class QFocusEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

class PillSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit PillSpinBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    enum PressedSide {
        NoSide,
        DownSide,
        UpSide
    };

    int sideWidth() const;
    int unitWidth() const;
    QRect downRect() const;
    QRect upRect() const;
    QRect unitRect() const;
    void updateEditorMargins();
    bool handleMousePress(QMouseEvent *event, const QPoint &pos);
    bool handleMouseRelease(QMouseEvent *event, const QPoint &pos);
    void drawButtonText(QPainter &painter, const QRect &rect, const QString &text, bool pressed) const;
    void drawUnitText(QPainter &painter) const;

    PressedSide m_pressedSide = NoSide;
};

#endif // PILLSPINBOX_H
