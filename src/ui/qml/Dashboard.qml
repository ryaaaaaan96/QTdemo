import QtQuick

Rectangle {
    width: 340
    height: 480
    color: "#15283a"
    Text {
        id: title
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 22 }
        text: "实时点位"
        color: "#f3f7fb"
        font.pixelSize: 24
        font.bold: true
    }
    Text {
        id: caption
        anchors { left: title.left; right: title.right; top: title.bottom; topMargin: 8 }
        text: "模拟数据 · 页面重载不停止采集"
        color: "#a5bbce"
        font.pixelSize: 13
        wrapMode: Text.Wrap
    }
    ListView {
        anchors { left: parent.left; right: parent.right; top: caption.bottom; bottom: parent.bottom; margins: 18 }
        spacing: 10
        clip: true
        model: pointModel
        delegate: Rectangle {
            required property string sourceName
            required property string pointName
            required property string valueText
            required property string unitText
            required property string qualityText
            width: ListView.view.width
            height: 92
            radius: 8
            color: "#223c52"
            Text {
                x: 14; y: 12
                text: sourceName + " / " + pointName
                color: "#c8d8e6"
                font.pixelSize: 12
            }
            Text {
                x: 14; y: 37
                text: valueText + " " + unitText
                color: "#ffffff"
                font.pixelSize: 25
            }
            Text {
                anchors { right: parent.right; bottom: parent.bottom; margins: 12 }
                text: qualityText
                color: qualityText === "有效" ? "#76dfbd" : "#ffd08a"
                font.pixelSize: 12
            }
        }
    }
}
