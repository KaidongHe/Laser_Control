# User Main Window Design

## Context

`serialhelper-1.2` currently starts directly into the full Qt control window. That window is useful for development and debugging, but it exposes serial configuration, charts, logs, TRY scanning, and low-level laser controls that ordinary users should not need.

The new startup experience should be a simple operator-facing main window. The existing full window becomes a password-protected developer interface.

## Goals

- Show a clean operator main window at application startup.
- Keep the interface focused on three controls:
  - `L1 开/关（种子）`
  - `预放开/关`
  - `功率调整`, range `2-100%`
- Use the approved "clean industrial" visual direction: light background, centered panel, large blue controls, restrained styling.
- Preserve the existing full `Widget` developer UI and its current behavior.
- Provide a low-visibility developer entry point that opens the old full UI only after password verification.

## Non-Goals

- Do not redesign the developer UI in this task.
- Do not upgrade the serial protocol in this task.
- Do not change STM32 firmware behavior in this task.
- Do not implement broader safety hardening here.

## Proposed Architecture

Add a new Qt startup window class, tentatively `MainWindow` or `OperatorWindow`, in the `serialhelper` project. `main.cpp` should instantiate this new operator window instead of directly showing the existing `Widget`.

The existing `Widget` remains the developer window. The operator window owns the developer-entry action:

1. User clicks a small `开发者` button.
2. The app shows a password dialog.
3. If the password is correct, hide the operator window and instantiate/show the existing `Widget`.
4. If the password is wrong or canceled, stay on the operator window.
5. When the developer window closes, show the operator window again.

The operator window should be visually and logically independent enough that future operator controls can be wired without disturbing the developer UI.

## UI Layout

The operator window uses a light application background and a centered white panel.

Inside the panel:

- Header: concise title such as `激光控制`.
- Three large vertical controls with consistent width and height.
- The first two controls behave as toggle buttons and visibly represent on/off state.
- The third control is a numeric percentage input for `2-100%`.
- A small `开发者` button sits in a low-emphasis location, such as the bottom-right of the panel.

The visual style should follow the approved mockup option A:

- Light gray or blue-gray page background.
- White main panel with subtle border/shadow.
- Primary blue controls.
- Rounded corners around 8 px for the final implementation unless existing Qt styling makes a slightly larger radius look more natural.
- Clear Chinese labels using a font that renders well on Windows, such as Microsoft YaHei with fallback.

## Behavior

At startup:

- Show the operator window.
- Do not show the developer window automatically.

For developer access:

- Click `开发者`.
- Prompt for a password.
- Correct password hides the operator window and opens the existing `Widget` developer interface.
- Incorrect password shows a short warning and keeps the operator window open.
- Closing the developer interface returns to the operator window.

For the three operator controls:

- `L1 开/关（种子）` and `预放开/关` are intended operator controls for STM32 behavior.
- The first implementation pass will build the UI, state display, and developer-window gating without inventing unsupported firmware commands.
- `功率调整` accepts only integer values from `2` to `100`.
- Serial command mapping must be explicitly confirmed before wiring these controls to hardware, because the current firmware protocol only defines `'0'~'9'` laser step commands and does not yet define explicit seed/pre-release/power-percentage commands.

## Error Handling

- Password dialog cancel: no state change.
- Wrong password: show a modal warning.
- Developer window already open: keep the operator window hidden and bring the existing developer window to front instead of opening duplicate windows.
- Invalid power input: Qt spin box constraints prevent out-of-range values.

## Testing

Manual verification should cover:

- Application starts on the new operator window.
- The operator layout matches the approved clean industrial direction.
- Power control cannot go below `2%` or above `100%`.
- Canceling the password dialog leaves the app on the operator window.
- Wrong password does not open developer UI.
- Correct password hides the operator window and opens the existing developer UI.
- Closing the developer UI shows the operator window again.
- Re-clicking developer entry when the developer UI is already open does not create confusing duplicate windows.

## Password Policy

The first implementation will use a single hardcoded developer password constant: `laser2026`.

This is acceptable for the first local desktop version because the goal is to hide the developer UI from ordinary operators, not to provide strong account security. The password value should live in one obvious constant so it can be changed later without touching UI logic.
