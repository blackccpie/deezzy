/*
The MIT License

Copyright (c) 2017-2017 Albert Murienne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

import QtQuick 2.0
//import QtMultimedia 5.0

Image {
    id: barSlider

    signal seek( int progress )

    //property DeezzyApp deezzy
    property url bgImg
    property url bufferImg: "images/slider_value_left.png"
    property url progressImg: "images/slider_value_right.png"
    property url knobImg

    width: parent.width
    source: bgImg
    anchors.verticalCenterOffset: 3

    function setRenderProgress(progress) {
        trackProgress.width = progress * trackProgressParent.width / 100;
    }
    function setBufferProgress(progress) {
        trackBuffer.width = progress * trackBufferParent.width / 100;
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            seek( 100 * mouse.x/width )
        }
    }

    Rectangle {
        id: trackBufferParent
        width: parent.width - 4
        anchors.verticalCenterOffset: -1
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        height: 16
        color: "transparent"
        BorderImage {
            id: trackBuffer
            source: bufferImg
            border { left: 8; top: 8; right: 8; bottom: 8 }
            horizontalTileMode: BorderImage.Stretch
            verticalTileMode: BorderImage.Stretch
        }
    }

    Rectangle {
        id: trackProgressParent
        width: parent.width - 4
        anchors.verticalCenterOffset: -1
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        height: 16
        color: "transparent"
        BorderImage {
            id: trackProgress
            source: progressImg
            border { left: 8; top: 8; right: 8; bottom: 8 }
            horizontalTileMode: BorderImage.Stretch
            verticalTileMode: BorderImage.Stretch
        }
    }

    Rectangle {
        width: parent.width - 10
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        color: "transparent"
        Image {
            id: trackSeeker
            source: knobImg
            anchors.verticalCenter: parent.verticalCenter
            x: trackProgress.width - 10
            state: "none"
            MouseArea {
                id: dragArea
                anchors.fill: parent
                drag.target: parent
                drag.axis: Drag.XAxis
                drag.minimumX: -10
                drag.maximumX: parent.parent.width-20
                onPressed: trackSeeker.state = "pressed"
                onReleased: {
                    trackSeeker.state = "none"
                    seek( 100 * trackSeeker.x/(parent.parent.width-10) )
                }
            }
            states: State {
                name: "pressed"
                when: mouseArea.pressed
                PropertyChanges { target: trackSeeker; scale: 1.2 }
            }
            transitions: Transition {
                NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
            }
        }
    }
}
