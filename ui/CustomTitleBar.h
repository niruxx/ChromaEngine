#pragma once

#include <QWidget>

class QLabel;
class QToolButton;
class QMouseEvent;

namespace colorfy {

// A title bar matching the app's dark theme, replacing the native Windows
// one. Resize-from-edges is implemented at the owning QMainWindow level via
// WM_NCHITTEST (see SettingsWindow::nativeEvent). Drag-to-move and
// double-click-to-maximize are handled right here via plain mouse-event
// tracking (press starts a drag, move repositions the window, release ends
// it) - not WM_NCHITTEST's HTCAPTION (returning HTCAPTION worked in
// isolated hit-test probes but proved unreliable for actual mouse-driven
// dragging - Qt's own mouse handling on the widget appears to compete with
// the native non-client drag for the same click) and not
// QWindow::startSystemMove() either (hands off to a native OS modal move
// loop; live testing showed it could leave the mouse in a stuck
// captured-for-move state that made the app - and even the tray icon's
// context menu - stop responding to clicks afterward). Manual tracking
// avoids handing control to any native modal loop at all.
class CustomTitleBar : public QWidget {
    Q_OBJECT
public:
    // Logical-pixel layout constants, kept here as the single source of
    // truth for SettingsWindow::nativeEvent's hit-testing, which computes
    // the draggable region directly in native/physical coordinates rather
    // than going through Qt's widget-local coordinate mapping (see that
    // function's comment for why).
    static constexpr int kHeight = 32;
    static constexpr int kButtonWidth = 44;
    static constexpr int kButtonCount = 3; // minimize, maximize/restore, close

    explicit CustomTitleBar(const QString& title, QWidget* parent = nullptr);

    void setMaximized(bool maximized);

    // Buttons, exposed so the owning window can hit-test around them.
    QWidget* minimizeButton() const;
    QWidget* maximizeButton() const;
    QWidget* closeButton() const;

signals:
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();
    // Lets the owning window silence expensive repaint work (the animated
    // background) while a drag is in progress.
    void dragStarted();
    void dragFinished();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    bool isOnButton(const QPoint& pos) const;

    QToolButton* m_minimizeButton = nullptr;
    QToolButton* m_maximizeButton = nullptr;
    QToolButton* m_closeButton = nullptr;
    bool m_maximized = false;

    bool m_dragging = false;
    QPoint m_dragStartGlobalPos;
    QPoint m_dragStartWindowPos;
};

} // namespace colorfy
