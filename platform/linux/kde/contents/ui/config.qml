pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel

// Shown in System Settings > Appearance > Wallpaper when "ChromaEngine" is
// picked as the wallpaper type. The cfg_<Name> properties are the standard
// Plasma wallpaper-config convention: the loader initializes each one from
// ../config/main.xml's current value and writes it back on Apply/OK - no
// explicit save code needed here.
ColumnLayout {
    id: configRoot

    property string cfg_FolderPath: ""
    property string cfg_SelectedFile: ""
    property int cfg_FillMode: 2
    property bool cfg_Muted: true
    property int cfg_Volume: 100
    property double cfg_PlaybackRate: 1.0

    FolderDialog {
        id: folderDialog
        title: "Choose wallpaper folder"
        onAccepted: {
            const path = decodeURIComponent(selectedFolder.toString().replace("file://", ""));
            configRoot.cfg_FolderPath = path;
        }
    }

    RowLayout {
        Layout.fillWidth: true

        Label {
            text: configRoot.cfg_FolderPath.length > 0 ? configRoot.cfg_FolderPath : "No folder selected"
            Layout.fillWidth: true
            elide: Text.ElideMiddle
        }
        Button {
            text: "Browse..."
            onClicked: folderDialog.open()
        }
    }

    FolderListModel {
        id: folderModel
        folder: configRoot.cfg_FolderPath.length > 0 ? ("file://" + configRoot.cfg_FolderPath) : ""
        nameFilters: ["*.mp4", "*.mkv", "*.webm", "*.avi", "*.mov", "*.gif",
                      "*.MP4", "*.MKV", "*.WEBM", "*.AVI", "*.MOV", "*.GIF"]
        showDirs: false
        sortField: FolderListModel.Name
    }

    Label {
        text: "Videos and GIFs in this folder (click to select):"
        visible: folderModel.count > 0
    }

    GridView {
        id: grid
        Layout.fillWidth: true
        Layout.preferredHeight: 320
        Layout.fillHeight: true
        cellWidth: 160
        cellHeight: 110
        model: folderModel
        clip: true

        delegate: Rectangle {
            id: tile

            required property string fileName
            required property string filePath

            width: grid.cellWidth - 8
            height: grid.cellHeight - 8
            radius: 6
            color: tile.filePath === configRoot.cfg_SelectedFile ? "#3a6ea5" : "#2a2a2e"
            border.color: tile.filePath === configRoot.cfg_SelectedFile ? "#5ba8e6" : "transparent"
            border.width: 2

            Column {
                anchors.centerIn: parent
                spacing: 4
                width: parent.width - 12

                Rectangle {
                    width: 40
                    height: 40
                    radius: 6
                    color: "#1c1c20"
                    anchors.horizontalCenter: parent.horizontalCenter

                    Label {
                        anchors.centerIn: parent
                        text: tile.fileName.toLowerCase().endsWith(".gif") ? "GIF" : "▶"
                        color: "#c8c8ce"
                    }
                }
                Label {
                    text: tile.fileName
                    width: parent.width
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 11
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: configRoot.cfg_SelectedFile = tile.filePath
            }
        }
    }

    RowLayout {
        Label { text: "Fill mode:" }
        ComboBox {
            id: fillModeCombo
            model: ["Stretch", "Fit", "Fill (Cover)"]
            currentIndex: configRoot.cfg_FillMode
            onActivated: configRoot.cfg_FillMode = currentIndex
        }
    }

    RowLayout {
        Layout.fillWidth: true

        CheckBox {
            id: muteCheck
            text: "Mute"
            checked: configRoot.cfg_Muted
            onToggled: configRoot.cfg_Muted = checked
        }
        Slider {
            id: volumeSlider
            from: 0
            to: 100
            value: configRoot.cfg_Volume
            enabled: !muteCheck.checked
            onMoved: configRoot.cfg_Volume = value
            Layout.fillWidth: true
        }
    }

    RowLayout {
        Layout.fillWidth: true

        Label { text: "Speed:" }
        Slider {
            id: speedSlider
            from: 0.25
            to: 3.0
            value: configRoot.cfg_PlaybackRate
            onMoved: configRoot.cfg_PlaybackRate = value
            Layout.fillWidth: true
        }
        Label { text: speedSlider.value.toFixed(2) + "x" }
    }
}
