// Main.qml
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
//import QtQml 2.2

ApplicationWindow {
    id: window
    visible: true
    width: 680
    height: 300
    minimumWidth: 680
    minimumHeight: 300
    title: qsTr("Copy Tool V01.001")
    property int margin: 16
    property bool scanned: false
    // 绑定冲突列表和对话框实例
    property var conflictList: []
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
    }

    ColumnLayout {
        anchors {
            top: parent.top; topMargin: margin
            left: parent.left; leftMargin: margin
            right: parent.right; rightMargin: margin
        }
        spacing: 12

        // 可移动设备列表，仅在扫描后且设备数 >= 2 时显示
        RowLayout {
            spacing: 8; Layout.fillWidth: true
            visible: scanned && diskManager.drives.length >= 2

            Label { text: qsTr("U盘列表:"); Layout.preferredWidth: 60 }
            ComboBox {
                id: driveCombo
                Layout.fillWidth: true
                model: diskManager.drives
                onCurrentIndexChanged: {
                    if (currentIndex >= 0) {
                        diskManager.setCurrentDriveIndex(currentIndex)
                        scanned = false // 用户选择后隐藏下拉框
                    }
                }
            }
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            Label {
                text: qsTr("U盘")
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 50
            }
            TextField {
                id: targetField
                placeholderText: qsTr("请选择U盘设备路径")
                text: diskManager.currentDrive
                Layout.fillWidth: true
                readOnly: true
            }
            Button {
                text: qsTr("自动检测")
                onClicked: {
                    scanned = true
                    diskManager.scanDrives()
                }
            }
            Button {
                text: qsTr("路径查找")
                onClicked: {
                    onClicked: targetDialog.open()
                }
            }
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            Label {
                text: qsTr("本地文件")
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 50
            }

            TextField {
                id: localField
                placeholderText: qsTr("请选择本地文件路径")
                text: diskManager.currentLocalFile
                Layout.fillWidth: true
                readOnly: true
            }

            // 自动检测按钮先不做，保留样式禁用状态
            Button {
                text: qsTr("自动检测")
                enabled: false
            }

            // 路径查找功能
            Button {
                text: qsTr("路径查找")
                onClicked: {
                    onClicked: localDialog.open()
                }
            }
        }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            Label {
                text: qsTr("信息")
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 50
            }
            TextField {
                id: infoField
                readOnly: true
                text: diskManager.message
                placeholderText: qsTr("状态信息……")
                Layout.fillWidth: true
            }
        }

        // 分割线
        Rectangle {
            height: 1
            color: "#cccccc"
            Layout.fillWidth: true
        }

        // 底部按钮 行
        RowLayout {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: 20

            Button {
                text: qsTr("拷贝")
                onClicked: {
//                    progressBar.visible = true
                    diskManager.copyFiles(localField.text, targetField.text)
                }
            }
//            ProgressBar {
//                id: progressBar
//                from: 0; to: 1
//                visible: false
//                Layout.fillWidth: true
//            }

            ConflictDialog{
                id: conflictDialog
                onAccepted: {
                    // 用户确认覆盖后再执行复制
                    diskManager.setUserConflictChoice(true)
                }
                onRejected: {
                    diskManager.setUserConflictChoice(false)
                }
            }

            Connections {
                target: diskManager
                function onFileConflictDetected(fileSrcPath, relativePath)
                {
                    conflictDialog.relPath = relativePath
                    conflictDialog.open()
                }

//                onFileConflictDetected: {
//                    // 在弹窗前隐藏进度，以免遮挡
////                    progressBar.visible = false
//                    conflictDialog.relPath = relativePath
//                    conflictDialog.open()
//                }
//                onCopyProgress: {
//                    progressBar.visible = true
//                    progressBar.value = current / total
//                }
//                onCopyFinished: {
//                    progressBar.visible = false
//                }
            }

            Button {
                text: qsTr("检查U盘拷贝完全")
                onClicked: {
                    diskManager.verifyCopy(localField.text, targetField.text)
                }
            }
            // 弹出U盘
            Button {
                text: qsTr("检查加密狗")
                onClicked: {
                    diskManager.checkDog()
                }
            }
            // 弹出U盘
            Button {
                text: qsTr("弹出U盘")
                onClicked: {
                    diskManager.ejectDrive(targetField.text)
                }
            }
        }
    }

    // 1. U盘的 FileDialog
    FileDialog {
        id: targetDialog
        title: qsTr("请选择U盘路径")
        selectFolder: true        // 只选目录
        onAccepted: {
            diskManager.setCurrentDriveUrl(folder)
        }
    }

//    // 2. 本地文件的 FileDialog
    FileDialog {
        id: localDialog
        title: qsTr("请选择本地文件路径")
        selectFolder: true
        onAccepted: {
            diskManager.setCurrentLocalFileUrl(folder)
        }
    }
}
