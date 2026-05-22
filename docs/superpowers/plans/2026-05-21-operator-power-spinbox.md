# Operator Power SpinBox Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace only the operator page `powerSpinBox` with a Qt Widgets pill-style custom SpinBox that resembles the approved QML reference.

**Architecture:** Add a focused `PillSpinBox` subclass of `QSpinBox` for drawing the rounded background and side `- / +` hit areas while preserving normal `QSpinBox` validation and editing behavior. Promote only `operatorform.ui`'s `powerSpinBox` to this subclass, and add a light drop shadow in `operatorform.cpp`.

**Tech Stack:** Qt 5 Widgets, qmake, C++11, Qt Designer `.ui`, `QSpinBox`, `QPainter`, `QGraphicsDropShadowEffect`.

---

## File Structure

- Create `laser-control/serialhelper/pillspinbox.h`: declares the custom `PillSpinBox` widget and its state helpers.
- Create `laser-control/serialhelper/pillspinbox.cpp`: implements painting, hit testing, mouse interaction, editor margins, and focus/resize refresh behavior.
- Modify `laser-control/serialhelper/operatorform.ui`: promotes only `powerSpinBox` from `QSpinBox` to `PillSpinBox` and removes the old `QSpinBox#powerSpinBox` styling that would conflict with custom painting.
- Modify `laser-control/serialhelper/operatorform.cpp`: applies a subtle shadow effect to `powerSpinBox`.
- Modify `laser-control/serialhelper/serialhelper.pro`: adds `pillspinbox.cpp` and `pillspinbox.h` to the qmake project.

Repository note: the worktree may contain unrelated dirty files. Every `git add` command below must stage only the exact files listed for the task.

---

### Task 1: Add `PillSpinBox`

**Files:**
- Create: `laser-control/serialhelper/pillspinbox.h`
- Create: `laser-control/serialhelper/pillspinbox.cpp`

- [ ] **Step 1: Create the header**

Create `laser-control/serialhelper/pillspinbox.h` with this content:

```cpp
#ifndef PILLSPINBOX_H
#define PILLSPINBOX_H

#include <QSpinBox>

class PillSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit PillSpinBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

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
    QRect downRect() const;
    QRect upRect() const;
    void updateEditorMargins();
    void drawButtonText(QPainter &painter, const QRect &rect, const QString &text, bool pressed) const;

    PressedSide m_pressedSide = NoSide;
};

#endif // PILLSPINBOX_H
```

- [ ] **Step 2: Create the implementation**

Create `laser-control/serialhelper/pillspinbox.cpp` with this content:

```cpp
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
const QColor COLOR_BORDER(183, 198, 232);
const QColor COLOR_FOCUS_BORDER(33, 150, 243);
const QColor COLOR_BUTTON_TEXT(117, 117, 117);
const QColor COLOR_BUTTON_TEXT_PRESSED(25, 118, 210);
const QColor COLOR_BUTTON_TEXT_DISABLED(170, 170, 170);
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

int PillSpinBox::sideWidth() const
{
    const int fromHeight = qMax(52, height() - 8);
    return qMin(64, fromHeight);
}

QRect PillSpinBox::downRect() const
{
    return QRect(0, 0, sideWidth(), height());
}

QRect PillSpinBox::upRect() const
{
    return QRect(width() - sideWidth(), 0, sideWidth(), height());
}

void PillSpinBox::updateEditorMargins()
{
    if (!lineEdit()) return;
    const int margin = sideWidth() + 4;
    lineEdit()->setTextMargins(margin, 0, margin, 0);
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

void PillSpinBox::paintEvent(QPaintEvent *event)
{
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF box = rect().adjusted(2, 2, -2, -2);
        const qreal radius = box.height() / 2.0;
        const QColor background = isEnabled() ? COLOR_BACKGROUND : COLOR_DISABLED_BACKGROUND;

        QPainterPath pillPath;
        pillPath.addRoundedRect(box, radius, radius);

        painter.fillPath(pillPath, background);

        if (m_pressedSide != NoSide && isEnabled()) {
            painter.save();
            painter.setClipPath(pillPath);
            const QRect pressedRect = (m_pressedSide == DownSide) ? downRect() : upRect();
            painter.fillRect(pressedRect, COLOR_PRESSED);
            painter.restore();
        }

        const QColor border = hasFocus() ? COLOR_FOCUS_BORDER : COLOR_BORDER;
        const int borderWidth = hasFocus() ? 2 : 1;
        painter.setPen(QPen(border, borderWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box, radius, radius);
    }

    QSpinBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawButtonText(painter, downRect(), QStringLiteral("-"), m_pressedSide == DownSide);
    drawButtonText(painter, upRect(), QStringLiteral("+"), m_pressedSide == UpSide);
}

void PillSpinBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isEnabled()) {
        if (downRect().contains(event->pos())) {
            m_pressedSide = DownSide;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }
        if (upRect().contains(event->pos())) {
            m_pressedSide = UpSide;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }
    }

    QSpinBox::mousePressEvent(event);
}

void PillSpinBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressedSide != NoSide) {
        const PressedSide releasedSide = m_pressedSide;
        m_pressedSide = NoSide;
        update();

        if (releasedSide == DownSide && downRect().contains(event->pos())) {
            stepDown();
        } else if (releasedSide == UpSide && upRect().contains(event->pos())) {
            stepUp();
        }

        event->accept();
        return;
    }

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
```

- [ ] **Step 3: Review the new files for accidental non-ASCII outside required UI strings**

Run:

```powershell
Get-Content -Raw -Encoding UTF8 laser-control/serialhelper/pillspinbox.h
Get-Content -Raw -Encoding UTF8 laser-control/serialhelper/pillspinbox.cpp
```

Expected: files match the code above; non-ASCII is limited to the `Microsoft YaHei` font name.

- [ ] **Step 4: Commit the new control**

Run:

```powershell
git add laser-control/serialhelper/pillspinbox.h laser-control/serialhelper/pillspinbox.cpp
git commit -m "feat: add pill spinbox control"
```

Expected: commit succeeds and includes only `pillspinbox.h` and `pillspinbox.cpp`.

---

### Task 2: Wire `PillSpinBox` Into The Operator UI

**Files:**
- Modify: `laser-control/serialhelper/operatorform.ui`
- Modify: `laser-control/serialhelper/operatorform.cpp`
- Modify: `laser-control/serialhelper/serialhelper.pro`

- [ ] **Step 1: Add the new files to qmake**

In `laser-control/serialhelper/serialhelper.pro`, update the project lists to include the new files:

```pro
SOURCES += \
        main.cpp \
        operatorform.cpp \
        widget.cpp \
        laserchart.cpp \
        pillspinbox.cpp

HEADERS += \
        operatorform.h \
        widget.h \
        laserchart.h \
        pillspinbox.h
```

- [ ] **Step 2: Promote only `powerSpinBox` in the UI**

In `laser-control/serialhelper/operatorform.ui`, change:

```xml
<widget class="QSpinBox" name="powerSpinBox">
```

to:

```xml
<widget class="PillSpinBox" name="powerSpinBox">
```

Keep the existing `alignment`, `suffix`, `minimum`, `maximum`, and `value` properties.

- [ ] **Step 3: Remove the old conflicting `QSpinBox#powerSpinBox` stylesheet block**

In `operatorform.ui`, remove this block from the top-level `styleSheet` property:

```css
QSpinBox#powerSpinBox {
  background: #f8fbff;
  color: #2b5fb7;
  border: 1px solid #b7c6e8;
  border-radius: 8px;
  padding: 8px 12px;
  font-family: 'Microsoft YaHei';
  font-size: 28px;
  font-weight: 600;
  min-height: 68px;
}
```

Do not remove any `QPushButton#developerButton`, `QLabel`, or page background styles.

- [ ] **Step 4: Register the custom widget in the UI file**

Add this block before `<resources/>` in `operatorform.ui`:

```xml
 <customwidgets>
  <customwidget>
   <class>PillSpinBox</class>
   <extends>QSpinBox</extends>
   <header>pillspinbox.h</header>
  </customwidget>
 </customwidgets>
```

- [ ] **Step 5: Add the shadow effect in `operatorform.cpp`**

Add these includes near the existing Qt includes:

```cpp
#include <QColor>
#include <QGraphicsDropShadowEffect>
```

After `ui->setupUi(this);` in the constructor, add:

```cpp
    QGraphicsDropShadowEffect *spinShadow = new QGraphicsDropShadowEffect(ui->powerSpinBox);
    spinShadow->setBlurRadius(12);
    spinShadow->setOffset(0, 3);
    spinShadow->setColor(QColor(0, 0, 0, 35));
    ui->powerSpinBox->setGraphicsEffect(spinShadow);
```

The constructor should start like this:

```cpp
operatorForm::operatorForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::operatorForm)
{
    ui->setupUi(this);

    QGraphicsDropShadowEffect *spinShadow = new QGraphicsDropShadowEffect(ui->powerSpinBox);
    spinShadow->setBlurRadius(12);
    spinShadow->setOffset(0, 3);
    spinShadow->setColor(QColor(0, 0, 0, 35));
    ui->powerSpinBox->setGraphicsEffect(spinShadow);

    updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"));
    updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"));
```

- [ ] **Step 6: Verify only operator form files changed**

Run:

```powershell
git diff -- laser-control/serialhelper/operatorform.ui laser-control/serialhelper/operatorform.cpp laser-control/serialhelper/serialhelper.pro
```

Expected: diff shows only `powerSpinBox` promotion, custom widget registration, the shadow include/effect, and qmake file list updates.

- [ ] **Step 7: Commit the UI wiring**

Run:

```powershell
git add laser-control/serialhelper/operatorform.ui laser-control/serialhelper/operatorform.cpp laser-control/serialhelper/serialhelper.pro
git commit -m "feat: style operator power spinbox"
```

Expected: commit succeeds and includes only `operatorform.ui`, `operatorform.cpp`, and `serialhelper.pro`.

---

### Task 3: Build And Manual Verification

**Files:**
- Verify: `laser-control/serialhelper/pillspinbox.h`
- Verify: `laser-control/serialhelper/pillspinbox.cpp`
- Verify: `laser-control/serialhelper/operatorform.ui`
- Verify: `laser-control/serialhelper/operatorform.cpp`
- Verify: `laser-control/serialhelper/serialhelper.pro`

- [ ] **Step 1: Run qmake**

Run:

```powershell
Set-Location laser-control/serialhelper
qmake serialhelper.pro
```

Expected: command exits with code `0` and generates or refreshes the qmake build files. If `qmake` is not on PATH, open `serialhelper.pro` in Qt Creator and run qmake from there.

- [ ] **Step 2: Build the project**

If qmake generated a MinGW Makefile, run:

```powershell
mingw32-make
```

Expected for MinGW kits: command exits with code `0` and produces the application binary.

If qmake generated an MSVC Makefile, run this instead from a Qt/MSVC developer shell:

```powershell
nmake
```

Expected for MSVC kits: command exits with code `0` and produces the application binary. In both cases, there must be no compiler errors involving `PillSpinBox`, `ui_operatorform.h`, or `QGraphicsDropShadowEffect`.

- [ ] **Step 3: Run the app and inspect the operator page**

Run the built application from Qt Creator or the generated binary.

Expected:
- Program starts on the ordinary operator page.
- `powerSpinBox` is a white pill-shaped control with centered percentage value.
- Left side displays `-`; right side displays `+`.
- The control has a subtle shadow.
- The two operator toggle buttons still work.
- The developer password flow still opens the existing developer window.

- [ ] **Step 4: Verify value behavior**

Manual checks:
- Default value is `2 %` or an equivalent percent display.
- Clicking `-` at `2%` keeps the value at `2%`.
- Clicking `+` increases the value by one step.
- Repeated clicking cannot exceed `100%`.
- Clicking the center allows editing the numeric value.
- Keyboard input outside `2-100` is constrained by the spin box.

- [ ] **Step 5: Verify developer window is unaffected**

Open the developer window with password `laser2026`.

Expected:
- `laser1spinbox`, `laser2spinbox`, and `laser3spinbox` keep their previous dark developer-window style.
- No developer-window control is promoted to `PillSpinBox`.

- [ ] **Step 6: Final status check**

Run:

```powershell
git status --short
```

Expected: any remaining dirty files are unrelated pre-existing worktree changes, not accidental edits to additional SpinBox files.
