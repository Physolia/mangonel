import QtQuick 2.2
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import org.kde 1.0
import QtGraphicalEffects 1.0

Window {
    id: window
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.X11BypassWindowManagerHint

    color: "transparent"

    width: 1000
    height: 650
    visible: true
    y: -height

    onVisibleChanged:{
        inputText.text = ""

        if (visible) {
            requestActivate()

            var desktopWidth = window.screen.width
            window.width = Math.max(desktopWidth / 1.5, 800)
        }

        inputText.preHistoryText = ""
        inputText.historyIndex = -1
    }

    onActiveChanged: if (!active) visible = false

    Connections {
        target: Mangonel
        onTriggered: window.visible = true
    }

    Rectangle {
        id: background
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: shadow.radius
        }
        height: 400 + historyList.height

        Behavior on height { NumberAnimation { duration: 50 } }

        color: Qt.rgba(0, 0, 0, 0.5)
        radius: 10
        visible: false

        Rectangle {
            id: bottomBackground

            anchors {
                bottom: background.bottom
                left: background.left
                right: background.right
            }
            color: "black"
            height: inputText.height + 20
        }
    }

    DropShadow {
        id: shadow
        anchors.fill: background
        radius: 10
        samples: 17
        color: Qt.rgba(0, 0, 0, 0.75)
        source: background
        cached: true
    }

    MouseArea {
        id: mouseArea
        anchors.fill: background
        acceptedButtons: Qt.RightButton
        onClicked: {
            popupMenu.popup()
        }
    }

    ListView {
        id: resultList
        visible: false

        anchors {
            top: background.top
//            bottom: historyList.top
            left: background.left
            right: background.right
        }
        height: 350
        clip: true
        model: Mangonel.apps
        orientation: Qt.Horizontal
        highlightMoveDuration: 50
        preferredHighlightBegin: width/2 - itemWidth/2
        preferredHighlightEnd: width/2 +  itemWidth/2
        highlightRangeMode: ListView.StrictlyEnforceRange

        property real itemWidth: height

        delegate: Item {
            width: resultList.itemWidth
            height: resultList.height

            function launch() {
                modelData.launch(modelData.program)
            }

            property string type: modelData.type
            property string completion: modelData.completion

            opacity: ListView.view.currentIndex === index ? 1 : 0.2
            Behavior on opacity { NumberAnimation { duration: 200 } }

            Image {
                id: icon
                anchors {
                    top: parent.top
                    topMargin: 10
                    horizontalCenter: parent.horizontalCenter
                }

                source: "image://icon/" + modelData.icon
                sourceSize.width: parent.width
                sourceSize.height: parent.height - nameText.height - 20
                visible: false
            }

            Glow {
                id: iconGlow
                anchors.fill: icon
                source: icon
                radius: 16
                samples: 32
            }

            Text {
                id: nameText
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 20
                    left: parent.left
                    right: parent.right
                }
                property string name: modelData.name
                onNameChanged: {
                    var index = name.toLowerCase().indexOf(inputText.text.toLowerCase())
                    if (index === -1) {
                        text = name
                        return
                    }

                    text = name.substring(0, index)
                    text += "<b>"
                    text += name.substring(index, index + inputText.text.length)
                    text += "</b>"
                    text += name.substring(index + inputText.text.length)
                }
                color: "white"

                font.pointSize: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData.name
                wrapMode: Text.WordWrap
            }
        }
    }

    LinearGradient {
        id: mask
        anchors.fill: resultList
        property real centerLeft: (width / 2 - resultList.itemWidth / 2) / width
        property real centerRight: (width / 2 + resultList.itemWidth / 2) / width

        gradient: Gradient {
            GradientStop { position: 0.1; color: Qt.rgba(1, 1, 1, 1) }
            GradientStop { position: mask.centerLeft; color: Qt.rgba(0, 0, 0, 0) }
            GradientStop { position: mask.centerRight; color: Qt.rgba(0, 0, 0, 0) }
            GradientStop { position: 0.9; color: Qt.rgba(1, 1, 1, 1) }
        }

        start: Qt.point(0, 0)
        end: Qt.point(resultList.width, 0)
        opacity: 0.5
        visible: false
    }

    MaskedBlur {
        anchors.fill: resultList
        source: resultList
        maskSource: mask
        radius: 8
        samples: 24
    }

    Text {
        anchors {
            left: background.left
            leftMargin: 15
            verticalCenter: inputText.verticalCenter
        }
        text: (resultList.count > 0 && resultList.currentItem !== null) ? resultList.currentItem.type : ""
        color: "white"
    }

    ListView {
        id: historyList
        anchors {
            bottom: inputText.top
            bottomMargin: 15
        }
        x: inputText.x + inputText.cursorRectangle.x - inputText.width
        height: inputText.historyIndex >= 0 ? 190 : 0
        interactive: false
        highlightFollowsCurrentItem: true
        currentIndex: inputText.historyIndex

        model: height ? Mangonel.history : 0
        verticalLayoutDirection: ListView.BottomToTop

        delegate: Text { text: modelData; color: "white"; font.bold: index == inputText.historyIndex; opacity: font.bold ? 1 : 0.4 }
    }

    TextInput {
        id: inputText
        anchors {
            horizontalCenter: background.horizontalCenter
            bottom: background.bottom
            bottomMargin: 10
        }

        property var history: Mangonel.history
        property int historyIndex: -1
        property string preHistoryText: ""

        onHistoryIndexChanged: {
            if (historyIndex < 0) {
                text = preHistoryText
                return
            }
            if (historyIndex >= history.length ) return

            text = history[historyIndex]
        }

        Keys.onDownPressed: {
            if (historyIndex >= 0) {
                historyIndex--
            }
        }

        Keys.onUpPressed: {
            if (historyIndex < 0) { // preserve what is written now
                preHistoryText = text
            }

            if (historyIndex <= history.length - 1) {
                historyIndex++
            }
        }

        color: "white"
        focus: true
        font.pointSize: 15
        font.bold: true
        onTextChanged: Mangonel.getApp(text)

        Keys.onEscapePressed: window.visible = false
        Keys.onLeftPressed: {
            if (resultList.currentIndex > 0) {
                resultList.currentIndex--
            }
            event.accepted = true
        }
        Keys.onRightPressed: {
            if (resultList.currentIndex < resultList.count - 1) {
                resultList.currentIndex++
            }
            event.accepted = true
        }
        Keys.onTabPressed: {
            event.accepted = true
            if (resultList.currentItem === null) {
                return
            }
            text = resultList.currentItem.completion
        }

        onAccepted: {
            if (resultList.currentItem === null) {
                return
            }
            Mangonel.addToHistory(text)
            resultList.currentItem.launch()
            window.visible = false
        }
    }

    MouseArea {
        anchors {
            left: background.left
            right: background.right
            bottom: background.bottom
        }

        acceptedButtons: "MiddleButton"
        onClicked: inputText.text += Mangonel.selectionClipboardContent()
        height: bottomBackground.height
    }

    Menu {
        id: popupMenu
        title: "Mangonel"
        MenuItem {
            text: qsTr("Configure &shortcuts")
            iconName: "configure-shortcuts"
            onTriggered: Mangonel.showConfig()
            shortcut: StandardKey.Preferences
        }
        MenuItem {
            text: qsTr("Configure &notifications")
            iconName: "preferences-desktop-notifications"
            onTriggered: Mangonel.configureNotifications()
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("&Quit")
            iconName: "application-exit"
            onTriggered: Qt.quit()
            shortcut: StandardKey.Quit
        }
    }

    // Ugly hack to work around being shown with garbage on initial showing with nvidia driver
    Component.onCompleted: uglyHackTimer.start()

    Timer {
        id: uglyHackTimer
        repeat: false
        interval: 100
        onTriggered: {
            window.width = Math.max(window.screen.width / 1.5, 800)
            window.y = window.screen.height / 2 - height / 2
            window.x = window.screen.width / 2 - width / 2
            window.flags = Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
            window.visible = false
        }
    }
}
