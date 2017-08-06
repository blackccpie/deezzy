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

import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

import Native.DeezzyApp 1.0

ApplicationWindow {
    id: appwindow
    width: 800
    height: 186
    visible: true

    DeezzyApp
    {
        id: deezzy

        Component.onCompleted: deezzy.connect();
        Component.onDestruction: deezzy.disconnect();
    }

    Item {
        id: playLogic

        property int index: -1
        //property DeezzyApp mediaPlayer: deezzy
        // property FolderListModel items: FolderListModel {
        //     folder: "music"
        //     nameFilters: ["*.mp3"]
        // }
        property var items: ["dzmedia:///album/43144861"]

        function init(){
            if(deezzy.playbackState==DeezzyApp.Paused){
            	deezzy.pause();
            }else if(deezzy.playbackState==DeezzyApp.Playing){
            	deezzy.play();
            }else{
                setIndex(0);
            }
        }

        function setIndex(i)
        {
            index = i;

            if (index < 0 || index >= items.count)
            {
                index = -1;
                deezzy.content = "";
            }
            else{
                //mediaPlayer.source = items.get(index,"filePath");
                deezzy.content = items[i]; // TODO-TMP
                deezzy.play();
            }
        }

        function next(){
            setIndex(index + 1);
        }

        function previous(){
            setIndex(index - 1);
        }

        function msToTime(duration) {
            var seconds = parseInt((duration/1000)%60);
            var minutes = parseInt((duration/(1000*60))%60);

            minutes = (minutes < 10) ? "0" + minutes : minutes;
            seconds = (seconds < 10) ? "0" + seconds : seconds;

            return minutes + ":" + seconds;
        }

        Connections {
            target: deezzy

            onPaused: {
                playPause.source = "icons/play.png";
            }

            onPlaying: {
                 playPause.source = "icons/pause.png";
            }

            onStopped: {
                playPause.source = "icons/play.png";
        		//if (playLogic.mediaPlayer.status == MediaPlayer.EndOfMedia)
        		//    playLogic.next();
            }

            onError: {
                console.log(error+" error string is "+errorString);
            }

        //     onMediaObjectChanged: {
        //         if (playLogic.mediaPlayer.mediaObject)
        //             playLogic.mediaPlayer.mediaObject.notifyInterval = 50;
        //     }
        }
    }

    FontLoader {
        id: appFont
        name: "OpenSans-Regular"
        source: "fonts/OpenSans-Regular.ttf"
    }

    Image {
        id: foreground
        source: "images/bar.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        ColumnLayout{
            id: container
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: foreground.implicitWidth - 80
            height: foreground.implicitHeight - 60

            RowLayout {
                id: wrapper
                anchors.fill: parent

                Rectangle {
                    id: leftWapper
                    height: 126
                    width: 126
                    radius: 7

                    BorderImage {
                        id: coverBorder
                        source: "images/cover_overlay.png"
                        anchors.fill: parent
                        anchors.margins: 4
                        border { left: 10; top: 10; right: 10; bottom: 10 }
                        horizontalTileMode: BorderImage.Stretch
                        verticalTileMode: BorderImage.Stretch

                        Image {
                            id: coverPic
                            //source: player.metaData.coverArtUrlLarge ? player.metaData.coverArtUrlLarge : "images/cover.png"
                            anchors.fill: coverBorder
                            anchors.margins: 2
                        }
                    }

                }

                ColumnLayout {
                    id: rightWapper
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    RowLayout {
                        id: upperWrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        Layout.leftMargin: 20
                        spacing: 25

                        Image {
                            id: prevTrack
                            source: "icons/rewind.png"
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 20
                            state: "none"
                            MouseArea {
                                anchors.fill: parent
                                onClicked: playLogic.previous()
                                onPressed: prevTrack.state = "pressed"
                                onReleased: prevTrack.state = "none"
                            }
                            states: State {
                                name: "pressed"
                                when: mouseArea.pressed
                                PropertyChanges { target: prevTrack; scale: 0.8 }
                            }
                            transitions: Transition {
                                NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                            }
                        }

                        Rectangle{
                            width: 30
                            anchors.verticalCenter: parent.verticalCenter
                            Image {
                                id: playPause
                                source: "icons/play.png"
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                                state: "none"
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: playLogic.init();
                                    onPressed: playPause.state = "pressed"
                                    onReleased: playPause.state = "none"
                                }
                                states: State {
                                    name: "pressed"
                                    when: mouseArea.pressed
                                    PropertyChanges { target: playPause; scale: 0.8 }
                                }
                                transitions: Transition {
                                    NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                                }
                            }
                        }

                        Image {
                            id: nextTrack
                            source: "icons/forward.png"
                            anchors.verticalCenter: parent.verticalCenter
                            state: "none"

                            MouseArea {
                                anchors.fill: parent
                                onClicked: playLogic.next()
                                onPressed: nextTrack.state = "pressed"
                                onReleased: nextTrack.state = "none"
                            }
                            states: State {
                                name: "pressed"
                                when: mouseArea.pressed
                                PropertyChanges { target: nextTrack; scale: 0.8 }
                            }
                            transitions: Transition {
                                NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                            }
                        }

                        Item {
                            Layout.fillWidth: true

                            ColumnLayout {
                                anchors.verticalCenter: parent.verticalCenter
                                Layout.fillWidth: true

                                Text {
                                    id: trackTitle
                                    //text: player.metaData.title ? player.metaData.title : "Song title unavailable"
                                    color: "#eeeeee"
                                    font.family: appFont.name
                                    font.pointSize: 17
                                    font.bold: true
                                    style: Text.Raised
                                    styleColor: "#111111"
                                    wrapMode: Text.Wrap
                                }
                                Text {
                                    id: trackAlbum
                                    //text: player.metaData.albumTitle ? player.metaData.albumTitle : "Song title unavailable"
                                    color: "steelblue"
                                    font.family: appFont.name
                                    font.pointSize: 17
                                    font.bold: true
                                    style: Text.Raised
                                    styleColor: "#111111"
                                    wrapMode: Text.Wrap
                                }
                            }
                        }

                        Image {
                            id: shareTrack
                            source: "icons/share.png"
                            anchors.verticalCenter: parent.verticalCenter
                            state: "none"

                            MouseArea {
                                anchors.fill: parent
                                onPressed: shareTrack.state = "pressed"
                                onReleased: shareTrack.state = "none"
                            }
                            states: State {
                                name: "pressed"
                                when: mouseArea.pressed
                                PropertyChanges { target: shareTrack; scale: 0.8 }
                            }
                            transitions: Transition {
                                NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                            }
                        }
                    }

                    RowLayout {
                        id: lowerWrap
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        Layout.leftMargin: 20
                        spacing: 15

                        Text {
                            id: currentTime
                            //text: playLogic.msToTime(player.position)
                            font.family: appFont.name
                            color: "#dedede"
                            font.pointSize: 18
                        }

                        SliderBar{
                            Layout.fillWidth: true
                            //audioPlayer: player
                            bgImg: "images/slider_background.png"
                            bufferImg: "images/slider_value_right.png"
                            progressImg: "images/slider_value_left.png"
                            knobImg: "images/slider_knob.png"
                        }

                        Text {
                            id: totalTime
                            //text: playLogic.msToTime(player.duration)
                            font.family: appFont.name
                            color: "#dedede"
                            font.pointSize: 18
                        }
                    }

                }

            }

        }

    }
}
