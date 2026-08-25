#include "ThumbnailGenerator.h"

#include <QColor>
#include <QDebug>
#include <QTimer>
#include <QWidget>

#include <windows.h>

#include <mpv/client.h>

namespace colorfy {

namespace {

constexpr int kCaptureWidth = 320;
constexpr int kCaptureHeight = 180;
constexpr int kInitialCaptureDelayMs = 900;
constexpr int kRetryCaptureDelayMs = 700;
constexpr int kMaxCaptureAttempts = 2;

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

QImage grabWindowImage(HWND hwnd, int width, int height)
{
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow)
        return {};

    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    QImage image(width, height, QImage::Format_RGB32);
    GetDIBits(hdcMem, hBitmap, 0, height, image.bits(), &bmi, DIB_RGB_COLORS);

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);

    return image;
}

} // namespace

ThumbnailGenerator::ThumbnailGenerator(QObject* parent)
    : QObject(parent)
{
    m_captureTimer = new QTimer(this);
    m_captureTimer->setSingleShot(true);
    connect(m_captureTimer, &QTimer::timeout, this, &ThumbnailGenerator::captureCurrent);
}

ThumbnailGenerator::~ThumbnailGenerator()
{
    if (m_mpv)
        mpv_terminate_destroy(m_mpv);
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

    m_captureWidget = new QWidget();
    m_captureWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    // On-screen (not off in negative coordinates) so DWM reliably keeps
    // compositing it - a fully transparent layered+click-through window
    // hides it instead. Off-screen positioning was found to make frame
    // capture flaky (DWM doesn't always keep presenting to windows placed
    // entirely outside the virtual desktop).
    m_captureWidget->setGeometry(0, 0, kCaptureWidth, kCaptureHeight);
    m_captureWidget->setAttribute(Qt::WA_NativeWindow);
    m_captureWidget->show();
    HWND hwnd = reinterpret_cast<HWND>(m_captureWidget->winId());

    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    m_mpv = mpv_create();
    if (!m_mpv) {
        qWarning() << "ThumbnailGenerator: mpv_create failed";
        finishCurrent();
        return;
    }

    int64_t wid = reinterpret_cast<int64_t>(hwnd);
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(m_mpv, "mute", "yes");
    mpv_set_option_string(m_mpv, "audio", "no");
    // Software decode: deterministic and avoids any GPU/driver session
    // flakiness from rapidly creating and tearing down mpv instances.
    mpv_set_option_string(m_mpv, "hwdec", "no");
    mpv_set_option_string(m_mpv, "keepaspect", "yes");
    mpv_set_option_string(m_mpv, "panscan", "1.0");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "osd-level", "0");
    mpv_set_option_string(m_mpv, "start", "20%");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_initialize(m_mpv);

    const QByteArray utf8Path = m_activePath.toUtf8();
    const char* args[] = {"loadfile", utf8Path.constData(), nullptr};
    mpv_command(m_mpv, args);

    m_captureTimer->start(kInitialCaptureDelayMs);
}

void ThumbnailGenerator::captureCurrent()
{
    if (!m_captureWidget) {
        finishCurrent();
        return;
    }

    ++m_captureAttempt;
    HWND hwnd = reinterpret_cast<HWND>(m_captureWidget->winId());
    const QImage image = grabWindowImage(hwnd, kCaptureWidth, kCaptureHeight);

    if (looksUninitialized(image) && m_captureAttempt < kMaxCaptureAttempts) {
        m_captureTimer->start(kRetryCaptureDelayMs);
        return;
    }

    // If it still looks uninitialized after every retry, leave the
    // placeholder icon in place rather than replacing it with a black tile.
    if (!image.isNull() && !looksUninitialized(image))
        emit thumbnailReady(m_activePath, QPixmap::fromImage(image));

    finishCurrent();
}

void ThumbnailGenerator::finishCurrent()
{
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
    if (m_captureWidget) {
        m_captureWidget->deleteLater();
        m_captureWidget = nullptr;
    }
    m_activePath.clear();

    startNext();
}

} // namespace colorfy
