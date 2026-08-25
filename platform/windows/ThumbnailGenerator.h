#pragma once

#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QString>

class QWidget;
class QTimer;
struct mpv_handle;

namespace colorfy {

// Generates a real preview thumbnail for a video file by briefly loading it
// into a hidden, off-screen libmpv instance and grabbing a frame via GDI.
// Files are processed one at a time so we never have more than one hidden
// mpv instance alive.
class ThumbnailGenerator : public QObject {
    Q_OBJECT
public:
    explicit ThumbnailGenerator(QObject* parent = nullptr);
    ~ThumbnailGenerator() override;

    void enqueue(const QString& filePath);

signals:
    void thumbnailReady(const QString& filePath, const QPixmap& pixmap);

private slots:
    void captureCurrent();

private:
    void startNext();
    void finishCurrent();

    QQueue<QString> m_queue;
    QString m_activePath;
    QWidget* m_captureWidget = nullptr;
    mpv_handle* m_mpv = nullptr;
    QTimer* m_captureTimer = nullptr;
    int m_captureAttempt = 0;
};

} // namespace colorfy
