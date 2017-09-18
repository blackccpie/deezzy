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

import QtQuick 2.3
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.2

import Native.DeezzyApp 1.0

ApplicationWindow {

    id: appWindow
    flags: Qt.FramelessWindowHint

    width: 480
    height: 320
    visible: true

    property int currentTrackDuration: 0 // TODO-TMP : Manage with a C++ binding?

    signal bufferProgress( int progress )
    signal renderProgress( int progress )

    function seek(progress) {
        deezzy.seek(progress);
    }

    DeezzyApp
    {
        id: deezzy
        objectName: "deezzy"

        Component.onCompleted: {
            sliderBar.setBufferProgress(0);
            sliderBar.setRenderProgress(0);
            appWindow.bufferProgress.connect(sliderBar.setBufferProgress);
            appWindow.renderProgress.connect(sliderBar.setRenderProgress);
            sliderBar.seek.connect(appWindow.seek);
            deezzy.connect();
        }
        Component.onDestruction: deezzy.disconnect();
    }

    Item {
        id: playLogic

        property int index: -1
        property var playlist: deezzy.defaultPlaylist()

        function init(){
            if(deezzy.playbackState==DeezzyApp.Paused){
                console.log("DEEZZY STATE IS PAUSED");
            	deezzy.resume();
            }else if(deezzy.playbackState==DeezzyApp.Playing){
                console.log("DEEZZY STATE IS PLAYING");
            	deezzy.pause();
            }else if(deezzy.playbackState==DeezzyApp.Stopped){
                console.log("DEEZZY STATE IS STOPPED");
                deezzy.content = playlist;
                deezzy.play();
            }else {
                console.log("DEEZZY STATE IS UNKNOWN");
            }
        }

        function stop(){
            deezzy.stop();
        }

        function next(){
            deezzy.next();
        }

        function previous(){
            deezzy.previous();
        }

        function love(){
            //deezzy.love();
            console.log("LOVE TRACK NOT IMPLEMENTED YET :-(");
        }

        function ban(){
            //deezzy.dislike();
            console.log("BAN TRACK NOT IMPLEMENTED YET :-(");
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
                playPause.source = "icons/play.svg";
            }

            onPlaying: {
                console.log("DEEZZY ONPLAYING");
                trackTitle.text = deezzy.trackInfos.title;
                trackArtist.text = deezzy.trackInfos.artist;
                trackAlbum.text = deezzy.trackInfos.albumTitle;
                coverPic.source = deezzy.trackInfos.coverArtUrl;
                playPause.source = "icons/pause.svg";
            }

            onStopped: {
                console.log("DEEZZY ONSTOPPED");
                playPause.source = "icons/play.svg";
                sliderBar.setRenderProgress(0);
                currentTime.text = '00:00';
            }

            onLoggedIn: {
                console.log("DEEZZY ONLOGGEDIN");
                controlWrapper.enabled = true;
            }

            onBufferProgress: {
                console.log("DEEZZY BUFFER PROGRESS " + progress.toFixed(2));
                bufferProgress(progress);
            }

            onRenderProgress: {
                console.log("DEEZZY RENDER PROGRESS " + progress.toFixed(2));
                currentTime.text = playLogic.msToTime(progress*appWindow.currentTrackDuration/100)
                renderProgress(progress);
            }

            onTrackDuration: {
                console.log("DEEZZY TRACK DURATION " + (duration_ms/1000));
                appWindow.currentTrackDuration = duration_ms;
                totalTime.text = playLogic.msToTime(duration_ms);
            }

            onError: {
                console.log("DEEZZY ONERROR");
            }

            onPlaylistChanged: {
                playLogic.playlist = playlist;
            }
        }
    }

    FontLoader {
        id: appFont
        name: "OpenSans"
        source: "fonts/OpenSans-Regular.ttf"
    }

    Rectangle {
        id: foreground

        color: "black"
        anchors.verticalCenterOffset: 0
        anchors.horizontalCenterOffset: 0
        width: parent.width
        height: parent.height

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.top: parent.top

        Row {
            id: topRow
            x: 5
            y: 10

            Rectangle{
                id: rectangleFirstRow
                width: 250

                Image {
                    id: deezerLogo
                    source: "images/deezer-logo.png"
                    width: 180
                    height: 40
                    mipmap: true
                    anchors.left: parent.left
                    anchors.top: parent.top
                }

                Rectangle {
                    id: spacer
                    width: 255
                    anchors.left: deezerLogo.right
                    anchors.top: parent.top
                }

                Image {
                    id: powerOff
                    source: "icons/power.svg"
                    width: 30
                    height: 30
                    mipmap: true
                    anchors.left: spacer.right
                    anchors.top: parent.top
                    state: "none"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: Qt.quit();
                        onPressed: powerOff.state = "pressed"
                        onReleased: powerOff.state = "none"
                    }
                    states: State {
                        name: "pressed"
                        when: mouseArea.pressed
                        PropertyChanges { target: powerOff; scale: 0.8 }
                    }
                    transitions: Transition {
                        NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }

        Item {

            id: controlWrapper

            width: 480
            height: 150

            enabled: false
            opacity: enabled ? 1. : 0.3

            Row {
                x: 165
                y: 60

                Rectangle {

                    Item {
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            Layout.fillWidth: true

                            Text {
                                id: trackArtist
                                text: deezzy.trackInfos.artist ? deezzy.trackInfos.artist : "Artist unavailable"
                                color: "#eeeeee"
                                font.family: appFont.name
                                font.pointSize: 16
                                font.bold: true
                                style: Text.Raised
                                styleColor: "#111111"
                                wrapMode: Text.Wrap
                            }
                            Text {
                                id: trackTitle
                                text: deezzy.trackInfos.title ? deezzy.trackInfos.title : "Song title unavailable"
                                color: "#eeeeee"
                                font.family: appFont.name
                                font.pointSize: 14
                                font.bold: true
                                style: Text.Raised
                                styleColor: "#111111"
                                wrapMode: Text.Wrap
                            }
                            Text {
                                id: trackAlbum
                                text: deezzy.trackInfos.albumTitle ? deezzy.trackInfos.albumTitle : "Album title unavailable"
                                color: "steelblue"
                                font.family: appFont.name
                                font.pointSize: 15
                                font.bold: true
                                style: Text.Raised
                                styleColor: "#111111"
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            RowLayout {
                id: midRow
                x: 5
                y: 120
                width: 480
                height: 150

                ColumnLayout {
                    id: container

                    RowLayout {
                        id: wrapper
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            id: leftWrapper
                            height: 120
                            width: 120
                            radius: 7

                            Image {
                                id: coverPic
                                source: deezzy.trackInfos.coverArtUrl ? deezzy.trackInfos.coverArtUrl : "images/cover.png"
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                width: 108
                                height: 108
                                onStatusChanged: {
                                    if (coverPic.status == Image.Ready)
                                        loadingPic.opacity = 0.
                                    else if (coverPic.status == Image.Loading)
                                        loadingPic.opacity = 1.
                                }
                            }

                            Image {
                                id: loadingPic
                                source: "icons/time-lapse.svg"
                                width: 50
                                height: 50
                                mipmap: true
                                opacity: 0.
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        ColumnLayout {
                            id: rightWrapper
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            RowLayout {
                                id: upperWrap
                                Layout.fillWidth: true
                                Layout.preferredHeight: 100
                                Layout.leftMargin: 10
                                spacing: 22

                                Rectangle{
                                    width: 40
                                	Image {
                                    	id: prevTrack
                                    	source: "icons/play-prev.svg"
                                        width: 40
                                        height: 40
                                        mipmap: true
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
                                }

                                Rectangle{
                                    width: 40
                                    anchors.verticalCenter: parent.verticalCenter
                                    Image {
                                        id: playPause
                                        source: "icons/play.svg"
                                        width: 40
                                        height: 40
                                        mipmap: true
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

                                Rectangle{
                                    width: 40
                                	Image {
                                    	id: stopTrack
                                    	source: "icons/stop.svg"
                                        width: 40
                                        height: 40
                                        mipmap: true
                                    	anchors.verticalCenter: parent.verticalCenter
                                    	state: "none"

                                    	MouseArea {
                                        	anchors.fill: parent
                                        	onClicked: playLogic.stop()
                                        	onPressed: stopTrack.state = "pressed"
                                        	onReleased: stopTrack.state = "none"
                                    	}
                                    	states: State {
                                        	name: "pressed"
                                        	when: mouseArea.pressed
                                        	PropertyChanges { target: stopTrack; scale: 0.8 }
                                    	}
                                    	transitions: Transition {
                                        	NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                                        }
                                    }
                                }

                                Rectangle{
                                    width: 40
                                	Image {
                                    	id: nextTrack
                                    	source: "icons/play-next.svg"
                                        width: 40
                                        height: 40
                                        mipmap: true
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
                                }

                                Rectangle{
                                    width: 30
                                    Image {
                                        id: loveTrack
                                        source: "icons/heart.svg"
                                        width: 25
                                        height: 25
                                        mipmap: true
                                        anchors.verticalCenter: parent.verticalCenter
                                        state: "none"

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: playLogic.love()
                                            onPressed: loveTrack.state = "pressed"
                                            onReleased: loveTrack.state = "none"
                                        }
                                        states: State {
                                            name: "pressed"
                                            when: mouseArea.pressed
                                            PropertyChanges { target: loveTrack; scale: 0.8 }
                                        }
                                        transitions: Transition {
                                            NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                                        }
                                    }
                                }

                                Rectangle{
                                    width: 30
                                    Image {
                                        id: banTrack
                                        source: "icons/heart.svg"
                                        width: 25
                                        height: 25
                                        mipmap: true
                                        anchors.verticalCenter: parent.verticalCenter
                                        state: "none"

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: playLogic.ban()
                                            onPressed: banTrack.state = "pressed"
                                            onReleased: banTrack.state = "none"
                                        }
                                        states: State {
                                            name: "pressed"
                                            when: mouseArea.pressed
                                            PropertyChanges { target: banTrack; scale: 0.8 }
                                        }
                                        transitions: Transition {
                                            NumberAnimation { properties: "scale"; duration: 100; easing.type: Easing.InOutQuad }
                                        }
                                    }

                                    Image {
                                        id: cross
                                        source: "icons/cross.svg"
                                        width: 25
                                        height: 25
                                        mipmap: true
                                        anchors.horizontalCenter: banTrack.horizontalCenter
                                        anchors.verticalCenter: banTrack.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                id: bottomRow
                x: 5

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: midRow.bottom
                RowLayout {
                    id: lowerWrap
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 15

                    Text {
                        id: currentTime
                        text: '00:00'
                        font.family: appFont.name
                        color: "#dedede"
                        font.pointSize: 16
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
                        text: '00:00'
                        font.family: appFont.name
                        color: "#dedede"
                        font.pointSize: 16
                    }
                }
            }
        }
    }
}
