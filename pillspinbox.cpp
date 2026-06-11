#include "pillspinbox.h"

#include <QEvent>
#include <QFocusEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

namespace {
const QColor COLOR_BACKGROUND(255, 255, 255);
const QColor COLOR_DISABLED_BACKGROUND(240, 240, 240);
const QColor COLOR_PRESSED(224, 224, 224);
const QColor COLOR_BORDER(183, 201, 242);
const QColor COLOR_FOCUS_BORDER(46, 120, 255);
const QColor COLOR_BUTTON_TEXT(117, 117, 117);
const QColor COLOR_BUTTON_TEXT_PRESSED(25, 118, 210);
const QColor COLOR_BUTTON_TEXT_DISABLED(170, 170, 170);
const QColor COLOR_UNIT_TEXT(46, 92, 184);
}

PillSpinBox::PillSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setAlignment(Qt::AlignCenter);
    setMinimumHeight(68);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setStyleSheet(QStringLiteral("PillSpinBox { background: transparent; border: none; }"));

    if (lineEdit()) {
        lineEdit()->setFrame(false);
        lineEdit()->setAlignment(Qt::AlignCenter);
        // The embedded editor can cover the side areas, so filter its mouse events too.
        lineEdit()->installEventFilter(this);
        lineEdit()->setStyleSheet(QString::fromUtf8(
            "QLineEdit {"
            " background: transparent;"
            " border: none;"
            " color: #212121;"
            " selection-background-color: #2196F3;"
            " selection-color: white;"
            " font-family: 'Microsoft YaHei';"
            " font-size: 28px;"
            " font-weight: 600;"
            "}"));
    }

    updateEditorMargins();
}

QSize PillSpinBox::sizeHint() const
{
    return QSize(220, 68);
}

QSize PillSpinBox::minimumSizeHint() const
{
    return QSize(180, 68);
}

bool PillSpinBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == lineEdit()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            const QPoint parentPos = lineEdit()->mapTo(this, mouseEvent->pos());
            return handleMousePress(mouseEvent, parentPos);
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            const QPoint parentPos = lineEdit()->mapTo(this, mouseEvent->pos());
            return handleMouseRelease(mouseEvent, parentPos);
        }
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            update();
        }
    }

    return QSpinBox::eventFilter(watched, event);
}

int PillSpinBox::sideWidth() const
{
    const int fromHeight = qMax(52, height() - 8);
    return qMin(64, fromHeight);
}

int PillSpinBox::unitWidth() const
{
    return 24;
}

QRect PillSpinBox::downRect() const
{
    return QRect(0, 0, sideWidth(), height());
}

QRect PillSpinBox::upRect() const
{
    return QRect(width() - sideWidth(), 0, sideWidth(), height());
}

QRect PillSpinBox::unitRect() const
{
    // Position the painted percent sign near the numeric center instead of anchoring it to the + button.
    // Lower this offset if "%" still feels too far from the number; raise it if it feels crowded.
    const int unitOffsetFromCenter = 34;
    return QRect(width() / 2 + unitOffsetFromCenter, 0, unitWidth(), height());
}

void PillSpinBox::updateEditorMargins()
{
    if (!lineEdit()) return;
    // Keep the editor mostly centered while reserving enough room for the + button and painted unit.
    // The right margin is deliberately smaller than sideWidth() + unitWidth() so "96" and "%" stay close.
    const int leftMargin = sideWidth() + 0;
    const int rightMargin = sideWidth() + 0;
    lineEdit()->setTextMargins(leftMargin, 0, rightMargin, 0);
}

bool PillSpinBox::handleMousePress(QMouseEvent *event, const QPoint &pos)
{
    if (event->button() == Qt::LeftButton && isEnabled()) {
        if (downRect().contains(pos)) {
            m_pressedSide = DownSide;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return true;
        }
        if (upRect().contains(pos)) {
            m_pressedSide = UpSide;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return true;
        }
    }

    return false;
}

bool PillSpinBox::handleMouseRelease(QMouseEvent *event, const QPoint &pos)
{
    if (event->button() == Qt::LeftButton && m_pressedSide != NoSide) {
        const PressedSide releasedSide = m_pressedSide;
        m_pressedSide = NoSide;
        update();

        if (releasedSide == DownSide && downRect().contains(pos)) {
            stepDown();
        } else if (releasedSide == UpSide && upRect().contains(pos)) {
            stepUp();
        }

        event->accept();
        return true;
    }

    return false;
}

void PillSpinBox::drawButtonText(QPainter &painter, const QRect &rect, const QString &text, bool pressed) const
{
    QFont f = font();
    f.setFamily(QStringLiteral("Microsoft YaHei"));
    f.setPointSize(18);
    f.setBold(true);
    painter.setFont(f);

    if (!isEnabled()) {
        painter.setPen(COLOR_BUTTON_TEXT_DISABLED);
    } else {
        painter.setPen(pressed ? COLOR_BUTTON_TEXT_PRESSED : COLOR_BUTTON_TEXT);
    }

    painter.drawText(rect, Qt::AlignCenter, text);
}

void PillSpinBox::drawUnitText(QPainter &painter) const
{
    QFont f = font();
    f.setFamily(QStringLiteral("Arial"));
    f.setPointSize(19);
    f.setBold(true);
    painter.setFont(f);
    painter.setPen(isEnabled() ? COLOR_UNIT_TEXT : COLOR_BUTTON_TEXT_DISABLED);
    painter.drawText(unitRect(), Qt::AlignCenter, QStringLiteral("%"));
}

void PillSpinBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = rect().adjusted(2, 2, -2, -2);
    const qreal radius = box.height() / 2.0;
    const QColor background = isEnabled() ? COLOR_BACKGROUND : COLOR_DISABLED_BACKGROUND;

    QPainterPath pillPath;
    pillPath.addRoundedRect(box, radius, radius);

    painter.fillPath(pillPath, background);

    // Keep the pressed feedback clipped to the pill so the side buttons feel integrated.
    if (m_pressedSide != NoSide && isEnabled()) {
        painter.save();
        painter.setClipPath(pillPath);
        const QRect pressedRect = (m_pressedSide == DownSide) ? downRect() : upRect();
        painter.fillRect(pressedRect, COLOR_PRESSED);
        painter.restore();
    }

    const bool activeFocus = hasFocus() || (lineEdit() && lineEdit()->hasFocus());
    const QColor border = activeFocus ? COLOR_FOCUS_BORDER : COLOR_BORDER;
    const int borderWidth = activeFocus ? 3 : 2;
    painter.setPen(QPen(border, borderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, radius, radius);

    drawButtonText(painter, downRect(), QStringLiteral("-"), m_pressedSide == DownSide);
    // Draw the unit outside the editable text so the cursor stops after the number, not after "%".
    drawUnitText(painter);
    drawButtonText(painter, upRect(), QStringLiteral("+"), m_pressedSide == UpSide);
}

void PillSpinBox::mousePressEvent(QMouseEvent *event)
{
    if (handleMousePress(event, event->pos())) return;

    QSpinBox::mousePressEvent(event);
}

void PillSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (handleMouseRelease(event, event->pos())) return;

    QSpinBox::mouseReleaseEvent(event);
}

void PillSpinBox::leaveEvent(QEvent *event)
{
    if (m_pressedSide != NoSide) {
        m_pressedSide = NoSide;
        update();
    }
    QSpinBox::leaveEvent(event);
}

void PillSpinBox::resizeEvent(QResizeEvent *event)
{
    QSpinBox::resizeEvent(event);
    updateEditorMargins();
}

void PillSpinBox::focusInEvent(QFocusEvent *event)
{
    QSpinBox::focusInEvent(event);
    update();
}

void PillSpinBox::focusOutEvent(QFocusEvent *event)
{
    QSpinBox::focusOutEvent(event);
    update();
}
