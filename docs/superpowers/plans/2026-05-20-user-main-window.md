# User Main Window Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a clean operator-facing startup window with three main controls and a password-protected path into the existing developer `Widget`.

**Architecture:** Add a focused `OperatorWindow` QWidget that owns the ordinary user UI and developer access flow. Keep the existing `Widget` unchanged as the developer window, and change `main.cpp` so the application starts with `OperatorWindow`.

**Tech Stack:** Qt 5 Widgets, qmake, C++11, existing `Widget` developer UI.

---

## File Structure

- Create `laser-control/serialhelper/operatorwindow.h`: declaration for the new operator startup window.
- Create `laser-control/serialhelper/operatorwindow.cpp`: hand-built Qt Widgets UI, password dialog, developer window lifecycle.
- Modify `laser-control/serialhelper/main.cpp`: include and show `OperatorWindow` instead of `Widget`.
- Modify `laser-control/serialhelper/serialhelper.pro`: add `operatorwindow.cpp` and `operatorwindow.h` to the qmake project.
- No changes to `laser-control/serialhelper/widget.*`, `widget.ui`, `laserchart.*`, or `laser-control/stm32/main.c` in this first pass.

## Implementation Notes

- Use a hand-coded QWidget rather than a new `.ui` file. This keeps the operator UI small, readable, and separate from the large existing `widget.ui`.
- Use a hardcoded password constant in `operatorwindow.cpp`: `laser2026`.
- The three operator controls are UI-only in this first pass. They should display state, but they must not send serial commands until the firmware protocol mapping is confirmed.
- When developer access succeeds, call `hide()` on the operator window. When the developer window closes, show the operator window again.
- Prevent duplicate developer windows by reusing the same `Widget *developerWindow` pointer.

---

### Task 1: Add Operator Window To Project

**Files:**
- Create: `laser-control/serialhelper/operatorwindow.h`
- Create: `laser-control/serialhelper/operatorwindow.cpp`
- Modify: `laser-control/serialhelper/serialhelper.pro`

- [ ] **Step 1: Create `operatorwindow.h`**

Create `laser-control/serialhelper/operatorwindow.h` with this content:

```cpp
#ifndef OPERATORWINDOW_H
#define OPERATORWINDOW_H

#include <QWidget>

class QPushButton;
class QSpinBox;
class Widget;

class OperatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorWindow(QWidget *parent = nullptr);
    ~OperatorWindow();

private slots:
    void toggleSeed();
    void togglePreRelease();
    void openDeveloperWindow();
    void handleDeveloperWindowClosed();

private:
    void setupUi();
    void updateToggleButton(QPushButton *button, bool enabled, const QString &label);

    QPushButton *seedButton = nullptr;
    QPushButton *preReleaseButton = nullptr;
    QSpinBox *powerSpinBox = nullptr;
    QPushButton *developerButton = nullptr;
    Widget *developerWindow = nullptr;
    bool seedEnabled = false;
    bool preReleaseEnabled = false;
};

#endif // OPERATORWINDOW_H
```

- [ ] **Step 2: Create `operatorwindow.cpp` skeleton**

Create `laser-control/serialhelper/operatorwindow.cpp` with this content:

```cpp
#include "operatorwindow.h"
#include "widget.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
const char *DEVELOPER_PASSWORD = "laser2026";
}

OperatorWindow::OperatorWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

OperatorWindow::~OperatorWindow()
{
    if (developerWindow) {
        developerWindow->deleteLater();
        developerWindow = nullptr;
    }
}

void OperatorWindow::setupUi()
{
}

void OperatorWindow::toggleSeed()
{
}

void OperatorWindow::togglePreRelease()
{
}

void OperatorWindow::openDeveloperWindow()
{
}

void OperatorWindow::handleDeveloperWindowClosed()
{
}

void OperatorWindow::updateToggleButton(QPushButton *button, bool enabled, const QString &label)
{
    if (!button) return;
    button->setText(QString("%1\n%2").arg(label, enabled ? QStringLiteral("已开启") : QStringLiteral("已关闭")));
    button->setProperty("active", enabled);
    button->style()->unpolish(button);
    button->style()->polish(button);
}
```

- [ ] **Step 3: Register new files in qmake project**

Modify `laser-control/serialhelper/serialhelper.pro`:

```pro
SOURCES += \
        main.cpp \
        widget.cpp \
        laserchart.cpp \
        operatorwindow.cpp

HEADERS += \
        widget.h \
        laserchart.h \
        operatorwindow.h
```

- [ ] **Step 4: Verify qmake recognizes the files**

Run in Qt Creator:

```text
Right-click serialhelper.pro -> Run qmake
```

Expected:

```text
No "Cannot find file operatorwindow.cpp" or "Cannot find file operatorwindow.h" error.
```

- [ ] **Step 5: Commit Task 1**

```powershell
git add laser-control/serialhelper/operatorwindow.h laser-control/serialhelper/operatorwindow.cpp laser-control/serialhelper/serialhelper.pro
git commit -m "Add operator window shell"
```

---

### Task 2: Build Clean Operator UI

**Files:**
- Modify: `laser-control/serialhelper/operatorwindow.cpp`

- [ ] **Step 1: Implement `setupUi()`**

Replace the empty `setupUi()` in `operatorwindow.cpp` with:

```cpp
void OperatorWindow::setupUi()
{
    setWindowTitle(QStringLiteral("激光控制"));
    resize(760, 620);
    setMinimumSize(640, 520);

    setStyleSheet(
        "OperatorWindow { background: #eef2f7; }"
        "QFrame#panel {"
        "  background: #ffffff;"
        "  border: 1px solid #cbd6e6;"
        "  border-radius: 8px;"
        "}"
        "QLabel#titleLabel {"
        "  color: #22304f;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 24px;"
        "  font-weight: 600;"
        "}"
        "QLabel#subtitleLabel {"
        "  color: #6d7890;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 13px;"
        "}"
        "QPushButton#mainButton {"
        "  background: #2f6fd6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 24px;"
        "  font-weight: 600;"
        "  min-height: 86px;"
        "}"
        "QPushButton#mainButton:hover { background: #245fbd; }"
        "QPushButton#mainButton[active='true'] { background: #1f8a5b; }"
        "QSpinBox#powerSpinBox {"
        "  background: #f8fbff;"
        "  color: #2b5fb7;"
        "  border: 1px solid #b7c6e8;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 28px;"
        "  font-weight: 600;"
        "  min-height: 68px;"
        "}"
        "QPushButton#developerButton {"
        "  background: transparent;"
        "  color: #7b879b;"
        "  border: 1px solid #d1d8e5;"
        "  border-radius: 4px;"
        "  padding: 5px 12px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "}"
        "QPushButton#developerButton:hover {"
        "  color: #2f6fd6;"
        "  border-color: #9db7e8;"
        "}"
    );

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(36, 36, 36, 36);
    rootLayout->setAlignment(Qt::AlignCenter);

    QFrame *panel = new QFrame(this);
    panel->setObjectName("panel");
    panel->setMinimumSize(430, 500);
    panel->setMaximumWidth(520);

    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(42, 34, 42, 24);
    panelLayout->setSpacing(22);

    QLabel *titleLabel = new QLabel(QStringLiteral("激光控制"), panel);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel(QStringLiteral("普通操作界面"), panel);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    seedButton = new QPushButton(panel);
    seedButton->setObjectName("mainButton");
    seedButton->setCheckable(false);
    seedButton->setCursor(Qt::PointingHandCursor);

    preReleaseButton = new QPushButton(panel);
    preReleaseButton->setObjectName("mainButton");
    preReleaseButton->setCheckable(false);
    preReleaseButton->setCursor(Qt::PointingHandCursor);

    QLabel *powerLabel = new QLabel(QStringLiteral("功率调整"), panel);
    powerLabel->setObjectName("subtitleLabel");
    powerLabel->setAlignment(Qt::AlignCenter);

    powerSpinBox = new QSpinBox(panel);
    powerSpinBox->setObjectName("powerSpinBox");
    powerSpinBox->setRange(2, 100);
    powerSpinBox->setValue(2);
    powerSpinBox->setSuffix(QStringLiteral(" %"));
    powerSpinBox->setAlignment(Qt::AlignCenter);
    powerSpinBox->setButtonSymbols(QAbstractSpinBox::PlusMinus);

    developerButton = new QPushButton(QStringLiteral("开发者"), panel);
    developerButton->setObjectName("developerButton");
    developerButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *developerRow = new QHBoxLayout();
    developerRow->addStretch();
    developerRow->addWidget(developerButton);

    panelLayout->addWidget(titleLabel);
    panelLayout->addWidget(subtitleLabel);
    panelLayout->addSpacing(8);
    panelLayout->addWidget(seedButton);
    panelLayout->addWidget(preReleaseButton);
    panelLayout->addWidget(powerLabel);
    panelLayout->addWidget(powerSpinBox);
    panelLayout->addStretch();
    panelLayout->addLayout(developerRow);

    rootLayout->addWidget(panel);

    updateToggleButton(seedButton, seedEnabled, QStringLiteral("L1 开/关（种子）"));
    updateToggleButton(preReleaseButton, preReleaseEnabled, QStringLiteral("预放开/关"));

    connect(seedButton, &QPushButton::clicked, this, &OperatorWindow::toggleSeed);
    connect(preReleaseButton, &QPushButton::clicked, this, &OperatorWindow::togglePreRelease);
    connect(developerButton, &QPushButton::clicked, this, &OperatorWindow::openDeveloperWindow);
}
```

- [ ] **Step 2: Implement toggle state display**

Replace `toggleSeed()` and `togglePreRelease()` with:

```cpp
void OperatorWindow::toggleSeed()
{
    seedEnabled = !seedEnabled;
    updateToggleButton(seedButton, seedEnabled, QStringLiteral("L1 开/关（种子）"));
}

void OperatorWindow::togglePreRelease()
{
    preReleaseEnabled = !preReleaseEnabled;
    updateToggleButton(preReleaseButton, preReleaseEnabled, QStringLiteral("预放开/关"));
}
```

- [ ] **Step 3: Add required include for `QAbstractSpinBox`**

At the top of `operatorwindow.cpp`, add:

```cpp
#include <QAbstractSpinBox>
```

- [ ] **Step 4: Build in Qt Creator**

Run:

```text
Qt Creator -> Build -> Build Project "serial"
```

Expected:

```text
Build succeeds.
No errors about QAbstractSpinBox, QPushButton, QSpinBox, or missing operatorwindow symbols.
```

- [ ] **Step 5: Commit Task 2**

```powershell
git add laser-control/serialhelper/operatorwindow.cpp
git commit -m "Build operator main window UI"
```

---

### Task 3: Add Password-Protected Developer Window Flow

**Files:**
- Modify: `laser-control/serialhelper/operatorwindow.cpp`

- [ ] **Step 1: Implement developer password flow**

Replace `openDeveloperWindow()` with:

```cpp
void OperatorWindow::openDeveloperWindow()
{
    if (developerWindow) {
        hide();
        developerWindow->show();
        developerWindow->raise();
        developerWindow->activateWindow();
        return;
    }

    bool ok = false;
    QString password = QInputDialog::getText(
        this,
        QStringLiteral("开发者验证"),
        QStringLiteral("请输入开发者密码："),
        QLineEdit::Password,
        QString(),
        &ok
    );

    if (!ok) {
        return;
    }

    if (password != QString::fromLatin1(DEVELOPER_PASSWORD)) {
        QMessageBox::warning(this, QStringLiteral("密码错误"), QStringLiteral("开发者密码不正确。"));
        return;
    }

    developerWindow = new Widget();
    developerWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(developerWindow, &QObject::destroyed, this, &OperatorWindow::handleDeveloperWindowClosed);

    hide();
    developerWindow->show();
}
```

- [ ] **Step 2: Implement return-to-operator behavior**

Replace `handleDeveloperWindowClosed()` with:

```cpp
void OperatorWindow::handleDeveloperWindowClosed()
{
    developerWindow = nullptr;
    show();
    raise();
    activateWindow();
}
```

- [ ] **Step 3: Add required include for `QLineEdit`**

At the top of `operatorwindow.cpp`, add:

```cpp
#include <QLineEdit>
```

- [ ] **Step 4: Build in Qt Creator**

Run:

```text
Qt Creator -> Build -> Build Project "serial"
```

Expected:

```text
Build succeeds.
No errors about QLineEdit, QInputDialog, Widget, or QObject::destroyed.
```

- [ ] **Step 5: Commit Task 3**

```powershell
git add laser-control/serialhelper/operatorwindow.cpp
git commit -m "Add developer password flow"
```

---

### Task 4: Start Application On Operator Window

**Files:**
- Modify: `laser-control/serialhelper/main.cpp`

- [ ] **Step 1: Change include**

In `main.cpp`, replace:

```cpp
#include "widget.h"
```

with:

```cpp
#include "operatorwindow.h"
```

- [ ] **Step 2: Change startup window**

In `main.cpp`, replace:

```cpp
    Widget w;
    w.show();
```

with:

```cpp
    OperatorWindow w;
    w.show();
```

- [ ] **Step 3: Build in Qt Creator**

Run:

```text
Qt Creator -> Build -> Build Project "serial"
```

Expected:

```text
Build succeeds.
Application starts with the new operator window instead of the developer Widget.
```

- [ ] **Step 4: Commit Task 4**

```powershell
git add laser-control/serialhelper/main.cpp
git commit -m "Start app on operator window"
```

---

### Task 5: Manual Verification

**Files:**
- No file edits expected.

- [ ] **Step 1: Launch the application from Qt Creator**

Run:

```text
Qt Creator -> Run
```

Expected:

```text
The first window is titled "激光控制".
The old developer Widget is not visible.
```

- [ ] **Step 2: Verify operator layout**

Check:

```text
The page has a light background.
The central panel is white.
The main controls are large blue controls.
The visible controls are:
- L1 开/关（种子）
- 预放开/关
- 功率调整, with values 2% to 100%
- A small 开发者 button
```

- [ ] **Step 3: Verify power input range**

Check:

```text
Set the power input below 2.
Expected: it clamps to 2%.

Set the power input above 100.
Expected: it clamps to 100%.
```

- [ ] **Step 4: Verify toggle display only**

Check:

```text
Click L1 开/关（种子）.
Expected: button text changes between 已关闭 and 已开启.

Click 预放开/关.
Expected: button text changes between 已关闭 and 已开启.

Expected: no serial command is sent from these controls in this first pass.
```

- [ ] **Step 5: Verify password cancel**

Check:

```text
Click 开发者.
Cancel the password dialog.
Expected: operator window remains visible.
Expected: developer window does not open.
```

- [ ] **Step 6: Verify wrong password**

Check:

```text
Click 开发者.
Enter wrong password: 123456
Expected: warning dialog appears.
Expected: operator window remains visible.
Expected: developer window does not open.
```

- [ ] **Step 7: Verify correct password**

Check:

```text
Click 开发者.
Enter correct password: laser2026
Expected: operator window hides.
Expected: existing developer Widget opens.
```

- [ ] **Step 8: Verify return from developer window**

Check:

```text
Close the developer Widget.
Expected: operator window appears again.
```

- [ ] **Step 9: Commit verification note if needed**

If manual verification reveals no code changes, do not create an empty commit. If a small fix is needed, make the fix and commit it:

```powershell
git add laser-control/serialhelper/operatorwindow.cpp laser-control/serialhelper/main.cpp laser-control/serialhelper/serialhelper.pro
git commit -m "Polish operator window behavior"
```
