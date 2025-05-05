import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2

MessageDialog {
    id: conflictDialog
    title: qsTr("文件已存在")
    icon: StandardIcon.Question
    standardButtons: Dialog.Yes | Dialog.No

//    property var conflicts: []
    property string relPath: ""

    text: qsTr("文件已存在：\n%1\n是否覆盖？").arg(relPath)
    onYes: {
        accepted()
    }
    onNo: {
        rejected()
    }

    // 对外信号
    signal accepted()
    signal rejected()

}
