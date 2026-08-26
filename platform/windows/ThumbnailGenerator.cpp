#include "ThumbnailGenerator.h"

#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QWidget>

#include <windows.h>

#include <mpv/client.h>

namespace colorfy {

namespace {

constexpr int kCaptureWidth = 320;
constexpr int kCaptureHeight = 180;
constexpr int kInitialSettleMs = 500;
constexpr int kSubsequentSettleMs = 300;
constexpr int kRetrySettleMs = 350;
constexpr int kMaxCaptureAttempts = 3;

// Evenly spread across the file, avoiding the very start/end where content
// is often a fade or a title card.
const int kStopPercents[] = {8, 22, 36, 50, 64, 78};
constexpr int kStopCount = 6;

// Cheap check for "decoder hadn't produced a frame yet" (solid black), so we
// can retry once rather than showing a blank tile for otherwise-fine videos.
// Not a proxy for "is this video's content dark" - sampling a sparse grid of
// pixels keeps false positives on legitimately dark content acceptably rare.
bool looksUninitialized(const QImage& image)
{
    if (image.isNull())
        return true;

    for (int y = 0; y < image.height(); y += 17) {
        for (int x = 0; x < image.width(); x += 23) {
            if (qGray(image.pixel(x, y)) > 8)
                return false;
        }
    }
    return true;
}

QString thumbnailTempDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/colorfy-thumbs");
    QDir().mkpath(dir);
    return dir;
}

} // namespace

ThumbnailGenerator::ThumbnailGenerator(QObject* parent)
    : QObject(parent)
{
    m_captureTimer = new QTimer(this);
    m_captureTimer->setSingleShot(true);
    connect(m_captureTimer, &QTimer::timeout, this, &ThumbnailGenerator::captureCurrent);

    // One persistent background thread for the lifetime of this object,
    // reused for every capture via queued invocation - not a new OS thread
    // per capture (see the class comment for why that mattered).
    m_workerThread = new QThread(this);
    m_workerThread->setObjectName(QStringLiteral("ThumbnailCaptureWorker"));
    m_workerContext = new QObject();
    m_workerContext->moveToThread(m_workerThread);
    m_workerThread->start();
}

ThumbnailGenerator::~ThumbnailGenerator()
{
    // Blocks until any in-flight capture on the worker thread finishes -
    // only ever delays shutdown, never interactive use - so the mpv handle
    // it's using is never torn down out from under it.
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_workerContext;

    if (m_mpv)
        mpv_terminate_destroy(m_mpv);
}

void ThumbnailGenerator::setAutoPlayEnabled(bool enabled)
{
    m_autoPlayEnabled = enabled;
}

void ThumbnailGenerator::enqueue(const QString& filePath)
{
    m_queue.enqueue(filePath);
    if (!m_mpv && m_activePath.isEmpty())
        startNext();
}

void ThumbnailGenerator::startNext()
{
    if (m_queue.isEmpty())
        return;

    m_activePath = m_queue.dequeue();
    m_captureAttempt = 0;
    m_stopIndex = 0;
    m_collectedFrames.clear();
    m_screenshotPath = thumbnailTempDir() + QStringLiteral("/") + QUuid::createUuid().toString(QUuid::Id128) + QStringLiteral(".png");

    // mpv still needs a window target to initialize its video output even
    // though we now read frames back via its own screenshot command instead
    // of capturing this window's pixels - it never needs to be visible.
    //
    // This used to sit on-screen at (0,0) behind a WS_EX_LAYERED alpha=0
    // trick to hide it, which was needed for an earlier GDI-capture
    // approach. That trick doesn't reliably hide mpv's own D3D-presented
    // content though (flip-model swap chains often bypass layered-window
    // alpha blending entirely), which showed up as videos visibly flashing
    // in the top-left corner while a folder's thumbnails were generated.
    // Since screenshot-to-file reads mpv's decoded frame directly - not
    // this window's presented pixels - the window no longer has to be
    // composited correctly (or even on-screen) for capture to work, so
    // parking it off-screen hides it unconditionally.
    m_captureWidget = new QWidget();
    m_captureWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    m_captureWidget->setGeometry(-10000, -10000, kCaptureWidth, kCaptureHeight);
    m_captureWidget->setAttribute(Qt::WA_NativeWindow);
    m_captureWidget->show();
    HWND hwnd = reinterpret_cast<HWND>(m_captureWidget->winId());

    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_NOACTIVATE;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

    const int64_t wid = reinterpret_cast<int64_t>(hwnd);
    const QString path = m_activePath;

    // mpv_create/mpv_initialize/loadfile all run off the main thread - only
    // the QWidget above (needed for its HWND) has to be created here.
    // mpv_initialize in particular can take a real, noticeable moment
    // (probing decoders, etc.), and doing this synchronously on the main
    // thread once per file in a folder scan was a separate source of
    // stutter beyond the screenshot capture itself (see applyVideoFilters-
    // adjacent comments elsewhere for the general pattern: anything that
    // blocks on mpv's C API doesn't belong on the UI thread).
    QMetaObject::invokeMethod(
        m_workerContext,
        [this, wid, path]() {
            mpv_handle* mpv = mpv_create();
            if (!mpv) {
                QMetaObject::invokeMethod(
                    this,
                    [this] {
                        qWarning() << "ThumbnailGenerator: mpv_create failed";
                        finishCurrent();
                    },
                    Qt::QueuedConnection);
                return;
            }

            int64_t widLocal = wid;
            mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &widLocal);
            mpv_set_option_string(mpv, "mute", "yes");
            mpv_set_option_string(mpv, "audio", "no");
            // Software decode: deterministic and avoids any GPU/driver
            // session flakiness from rapidly creating and tearing down mpv
            // instances.
            mpv_set_option_string(mpv, "hwdec", "no");
            // Without this, FFmpeg's software decoder defaults to roughly
            // one thread per CPU core for frame/slice-parallel decoding -
            // reasonable for a single real-time player, but this mpv
            // instance is a throwaway used only to grab a handful of still
            // frames, and every video in a folder scan gets one in
            // sequence, on top of the wallpaper's and the preview's own
            // decode threads running at the same time.
            mpv_set_option_string(mpv, "vd-lavc-threads", "2");
            mpv_set_option_string(mpv, "osc", "no");
            mpv_set_option_string(mpv, "osd-level", "0");
            mpv_set_option_string(mpv, "start", QByteArray::number(kStopPercents[0]).append('%').constData());
            mpv_set_option_string(mpv, "keep-open", "yes");
            mpv_set_option_string(mpv, "input-default-bindings", "no");
            mpv_set_option_string(mpv, "input-vo-keyboard", "no");
            mpv_initialize(mpv);

            const QByteArray utf8Path = path.toUtf8();
            const char* args[] = {"loadfile", utf8Path.constData(), nullptr};
            mpv_command(mpv, args);

            QMetaObject::invokeMethod(
                this,
                [this, mpv] {
                    m_mpv = mpv;
                    m_captureTimer->start(kInitialSettleMs);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

void ThumbnailGenerator::captureCurrent()
{
    if (!m_mpv) {
        finishCurrent();
        return;
    }

    ++m_captureAttempt;

    // mpv's client API is safe to call from a non-owning thread as long as
    // calls to the same handle are never made concurrently - guaranteed
    // here since captureCurrent() is never re-entered until this capture's
    // result has come back and been fully processed (see onCaptureResult).
    mpv_handle* const mpv = m_mpv;
    const QString screenshotPath = m_screenshotPath;

    QMetaObject::invokeMethod(
        m_workerContext,
        [this, mpv, screenshotPath]() {
            QFile::remove(screenshotPath);
            const QByteArray pathUtf8 = screenshotPath.toUtf8();
            const char* args[] = {"screenshot-to-file", pathUtf8.constData(), "video", nullptr};
            mpv_command(mpv, args);
            QImage image(screenshotPath);
            QFile::remove(screenshotPath);

            QMetaObject::invokeMethod(this, [this, image]() { onCaptureResult(image); }, Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

void ThumbnailGenerator::onCaptureResult(QImage image)
{
    const int stopCount = m_autoPlayEnabled ? kStopCount : 1;

    if (looksUninitialized(image) && m_captureAttempt < kMaxCaptureAttempts) {
        m_captureTimer->start(kRetrySettleMs);
        return;
    }

    // If it still looks uninitialized after every retry, just skip this stop
    // point rather than adding a black frame to the sequence.
    if (!image.isNull() && !looksUninitialized(image))
        m_collectedFrames.append(QPixmap::fromImage(image.scaled(
            kCaptureWidth, kCaptureHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));

    ++m_stopIndex;
    m_captureAttempt = 0;

    if (m_stopIndex >= stopCount) {
        if (!m_collectedFrames.isEmpty())
            emit framesReady(m_activePath, m_collectedFrames);
        finishCurrent();
        return;
    }

    seekToNextStop();
}

void ThumbnailGenerator::seekToNextStop()
{
    if (!m_mpv) {
        finishCurrent();
        return;
    }

    const QByteArray percent = QByteArray::number(kStopPercents[m_stopIndex]);
    const char* args[] = {"seek", percent.constData(), "absolute-percent", nullptr};
    mpv_command(m_mpv, args);

    m_captureTimer->start(kSubsequentSettleMs);
}

void ThumbnailGenerator::finishCurrent()
{
    if (m_mpv) {
        // Fire-and-forget on the worker thread: tearing down an mpv
        // instance can briefly block (joining its internal threads), and
        // that doesn't need to hold up starting the next file - each
        // file's mpv_handle is entirely independent.
        mpv_handle* mpv = m_mpv;
        m_mpv = nullptr;
        QMetaObject::invokeMethod(m_workerContext, [mpv]() { mpv_terminate_destroy(mpv); }, Qt::QueuedConnection);
    }
    if (m_captureWidget) {
        m_captureWidget->deleteLater();
        m_captureWidget = nullptr;
    }
    QFile::remove(m_screenshotPath);
    m_activePath.clear();
    m_collectedFrames.clear();

    startNext();
}

} // namespace colorfy
