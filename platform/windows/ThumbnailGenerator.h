#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QString>

class QWidget;
class QTimer;
class QThread;
struct mpv_handle;

namespace colorfy {

// Generates a short sequence of real preview frames for a video file by
// briefly loading it into a hidden, off-screen libmpv instance and seeking
// through it, grabbing a frame at each stop via mpv's own screenshot
// command. Files are processed one at a time so we never have more than one
// hidden mpv instance alive; all seeks for one file share a single mpv
// session rather than paying the startup cost per frame.
//
// The actual "screenshot-to-file" command blocks until a fresh frame is
// decoded, downscaled, and written to disk - for a 4K source this can take
// long enough (especially with the real wallpaper's own mpv instance also
// competing for the CPU/GPU) to visibly stall the UI if run on the main
// thread, since it shares this process's single Qt event loop. That work
// (plus the immediate re-read of the file) runs on a single persistent
// background thread instead - reused for every capture via queued
// invocation, rather than spun up and torn down per capture (which, with up
// to 6 stops x 3 retries per video, meant hundreds of short-lived OS threads
// over a folder scan; the resulting churn was almost certainly what made the
// whole app progressively sluggish and eventually unresponsive). Only the
// cheap mpv session setup/tear-down and Qt signal handling stay on the UI
// thread.
class ThumbnailGenerator : public QObject {
    Q_OBJECT
public:
    explicit ThumbnailGenerator(QObject* parent = nullptr);
    ~ThumbnailGenerator() override;

    void enqueue(const QString& filePath);

    // When false, only one frame is captured per video (much faster - a
    // single mpv session + one screenshot instead of up to 18) and the
    // library grid shows it as a static thumbnail instead of a cycling
    // preview. Takes effect for files enqueued after the call.
    void setAutoPlayEnabled(bool enabled);

signals:
    void framesReady(const QString& filePath, const QList<QPixmap>& frames);

private slots:
    void captureCurrent();

private:
    void startNext();
    void onCaptureResult(QImage image);
    void seekToNextStop();
    void finishCurrent();

    QQueue<QString> m_queue;
    QString m_activePath;
    QString m_screenshotPath;
    QWidget* m_captureWidget = nullptr;
    mpv_handle* m_mpv = nullptr;
    QTimer* m_captureTimer = nullptr;
    QThread* m_workerThread = nullptr;
    QObject* m_workerContext = nullptr;
    bool m_autoPlayEnabled = false;
    int m_captureAttempt = 0;
    int m_stopIndex = 0;
    QList<QPixmap> m_collectedFrames;
};

} // namespace colorfy
