import QtQuick 2.8
import QtQuick.Window 2.2

Window
{
    id: mainWindow
    visible: true
    width: 1280 + menu.width
    height: 720
    title: qsTr("touch screen overlay")
    color: "transparent"
    property string btnTxt: "Connect"


    // signal to move the mouse
    signal moveMouse(int x, int y);
    signal releaseMouse();
    signal buttonClick(int btnID);

    // side menu
    Rectangle
    {
        id: menu
        anchors.top: parent.top
        anchors.right: parent.right
        width: 250
        height: parent.height
        color: "grey"

        Rectangle
        {
            width: parent.width * 0.8
            height: 50
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 50

            Text
            {
                anchors.centerIn: parent
                text: mainWindow.btnTxt
            }

            MouseArea
            {
                anchors.fill: parent
                onClicked:
                {
                    mainWindow.buttonClick(111);
                }
            }
        }
    }


    // clickable area
    Rectangle
    {
        color: "#05000010"
        width: 1280
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
        border.color: "grey"
        border.width: 2

        // entire area is clickable
        MouseArea
        {
            id: mA
            anchors.fill: parent

            onMouseXChanged:
            {
                if (mA.pressed)
                {
                    movePtr(mouseX, mouseY)
                }
            }

            onMouseYChanged:
            {
                if (mA.pressed)
                {
                    movePtr(mouseX, mouseY)
                }
            }

            function movePtr(x, y)
            {
                ptr.x = x
                ptr.y = y
                ptr.color = "blue"
                mainWindow.moveMouse(x, y);
                //console.log('position is ' + x + 'x' + y)
            }

            onReleased:
            {
                ptr.color = "green"
                mainWindow.releaseMouse();
            }
        }

        Rectangle
        {
            id: ptr
            color: "green"
            width: 10
            height: width
            radius: width
            x: 0
            y: 0
        }

        Timer
        {
            id: tmrA
            repeat: true
            running: false
            interval: 5000
            property bool isX: true

            onTriggered:
            {
                if (tmrA.isX)
                {
                    console.log('move to 300x200 ')
                    mA.movePtr(300,200)
                }
                else
                {
                    console.log('move to 100x100 ')
                    mA.movePtr(100,100)
                }

                tmrA.isX = !tmrA.isX
            }
        }


    }



}
