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
import QtQuick.Layouts 1.2

import Native.DeezzyApp 1.0

ApplicationWindow {

    id: appWindow
    //flags: Qt.FramelessWindowHint

    width: 800
    height: 186
    visible: true

    signal bufferProgress( int progress )
    signal renderProgress( int progress )

    function seek(progress) {
        deezzy.seek(progress);
    }

    DeezzyApp
    {
        id: deezzy

        Component.onCompleted: {
            appWindow.bufferProgress.connect(sliderBar.setBufferProgress)
            appWindow.renderProgress.connect(sliderBar.setRenderProgress)
            sliderBar.seek.connect(appWindow.seek)
            deezzy.connect();
        }
        Component.onDestruction: deezzy.disconnect();
    }

    Item {
        id: playLogic

        property int index: -1
        property var playlist: "dzmedia:///album/42861141"

        function init(){
            if(deezzy.playbackState==DeezzyApp.Paused){
                console.log("DEEZZY STATE IS PAUSED");
            	deezzy.resume();
            }else if(deezzy.playbackState==DeezzyApp.Playing){
                console.log("DEEZZY STATE IS PLAYING");
            	deezzy.pause();
            }else if(deezzy.playbackState==DeezzyApp.Stopped){
                console.log("DEEZZY STATE IS STOPPED");
                deezzy.content = playlist; // TODO-TMP
                deezzy.play();
            }else {
                console.log("DEEZZY STATE IS UNKNOWN");
            }
        }

        function next(){
            deezzy.next();
        }

        function previous(){
            deezzy.previous();
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
                console.log("DEEZZY ONPAUSED");
                playPause.source = "icons/play.png";
            }

            onPlaying: {
                console.log("DEEZZY ONPLAYING");
                trackTitle.text = deezzy.trackInfos.title;
                trackAlbum.text = deezzy.trackInfos.albumTitle;
                coverPic.source = deezzy.trackInfos.coverArtUrl;
                playPause.source = "icons/pause.png";
                //totalTime.text = playLogic.msToTime(1000 * deezzy.trackInfos.duration)
            }

            onStopped: {
                console.log("DEEZZY ONSTOPPED");
                playPause.source = "icons/play.png";
        		//if (playLogic.mediaPlayer.status == MediaPlayer.EndOfMedia)
        		//    playLogic.next();
            }

            onBufferProgress: {
                console.log("DEEZZY BUFFER PROGRESS " + progress.toFixed(2));
                bufferProgress(progress);
            }

            onRenderProgress: {
                console.log("DEEZZY RENDER PROGRESS " + progress.toFixed(2));
                //lowerWrap.currentTime.text = playLogic.msToTime(player.position)
                renderProgress(progress);
            }

            onError: {
                console.log("DEEZZY ONERROR");
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
                            source: deezzy.trackInfos.coverArtUrl ? deezzy.trackInfos.coverArtUrl : "images/cover.png"
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
                                    text: deezzy.trackInfos.title ? deezzy.trackInfos.title : "Song title unavailable"
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
                                    text: deezzy.trackInfos.albumTitle ? deezzy.trackInfos.albumTitle : "Song title unavailable"
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
                            //text: playLogic.msToTime(position)
                            font.family: appFont.name
                            color: "#dedede"
                            font.pointSize: 18
                        }

                        SliderBar{
                            id: sliderBar
                            Layout.fillWidth: true
                            bgImg: "images/slider_background.png"
                            bufferImg: "images/slider_value_right.png"
                            progressImg: "images/slider_value_left.png"
                            knobImg: "images/slider_knob.png"
                        }

                        Text {
                            id: totalTime
                            //text: playLogic.msToTime(duration)
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
