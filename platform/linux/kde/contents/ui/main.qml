import QtQuick
import QtMultimedia

// Plasma 6 / Qt6 wallpaper plugin entry point. Loaded by plasmashell with a
// `wallpaper` context property giving access to the KConfigXT-backed
// settings declared in ../config/main.xml as wallpaper.configuration.<Key>.
//
// GIF and video go through two different QML item types (AnimatedImage vs.
// MediaPlayer+VideoOutput) since QtMultimedia's MediaPlayer does not treat
// GIF as a playable video source - unlike the Windows build, which uses
// libmpv for both through one code path.
Item {
    id: root

    readonly property string filePath: wallpaper.configuration.SelectedFile ?? ""
    readonly property bool hasFile: root.filePath.length > 0
    readonly property bool isGif: root.filePath.toLowerCase().endsWith(".gif")
    readonly property url fileUrl: root.hasFile ? ("file://" + root.filePath) : ""
    readonly property int fillMode: wallpaper.configuration.FillMode

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    AnimatedImage {
        id: gifPlayer
        anchors.fill: parent
        visible: root.hasFile && root.isGif
        source: visible ? root.fileUrl : ""
        fillMode: root.fillMode
        playing: visible
        cache: false
        asynchronous: true
    }

    AudioOutput {
        id: audioOutput
        muted: wallpaper.configuration.Muted
        volume: wallpaper.configuration.Volume / 100.0
    }

    MediaPlayer {
        id: mediaPlayer
        source: (root.hasFile && !root.isGif) ? root.fileUrl : ""
        videoOutput: videoOutput
        audioOutput: audioOutput
        loops: MediaPlayer.Infinite
        playbackRate: wallpaper.configuration.PlaybackRate

        onSourceChanged: {
            if (source.toString().length > 0)
                play();
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: root.hasFile && !root.isGif
        fillMode: root.fillMode
    }

    Connections {
        target: wallpaper.configuration
        function onSelectedFileChanged() {
            if (!root.isGif && root.hasFile)
                mediaPlayer.play();
        }
    }
}
