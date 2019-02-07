!!! **IMPORTANT** : PLEASE NOTE DEEZER HAS STOPPED SUPPORTING THE DEEZER NATIVE SDK !!!

# deezzy
Lightweight Qml/C++ Deezer Player using _Deezer Native SDK_ (amongst other platorms, it supports ARM Linux).

**deezzy** has no search/playlist management, it's default goal is  to _**play your user's flow radio**_, unless you launch it with a specific deezer album/playlist url.

![Deezzy](http://blackccpie.free.fr/deezzy/deezzy.png)

- UI Layout inspired by : [qml-audio-player](https://github.com/rohitsinghsengar/qml-audio-player)
- Deezer Native API : [Native SDK](http://developers.deezer.com/sdk/native)
- Third party JSON library: [JSON for Modern C++](https://github.com/nlohmann/json)

## Platforms support

Deezzy has been developped and initially tested on an Ubuntu 16.04LTS box, and then successfully built and tested on a Raspberry Pi 2 with a Tontec 3.5 inches touchscreen.
As far as the _Deezer Native SDK_ and _Qt SDK_ are crossplatforms, deezzy should be easily portable to Windows/MacOS.

![Deezzy-RPi](http://blackccpie.free.fr/deezzy/deezzy-rpi.jpg)

## Building and Running on the Raspberry Pi:

**!! First of all you need a Deezer user account in order to get a valid user id!!!**

1. Download the _Deezer Native SDK_ and clone the _deezzy_ repo (_both should be in the same directory_):
```shell
$ wget https://build-repo.deezer.com/native_sdk/deezer-native-sdk-v1.2.10.zip
$ unzip deezer-native-sdk-v1.2.10.zip
$ git clone https://github.com/blackccpie/deezzy.git
```

2. Register your application [here](http://developers.deezer.com) to get your own app id, and authorize it by getting your access token following [this](http://developers.deezer.com/api/oauth) procedure (use following permissions : `basic_access,email,manage_library`). With these infos, update the `private_user.h` header with your `USER_ID`, `USER_ACCESS_TOKEN` and the `USER_CACHE_PATH` of your choice.
```shell
$ nano deezzy/src/deezer_wrapper/private/private_user.h
```

3. make sure you have Qt/Qml and pulseaudio prerequisites installed:
```shell
$ sudo apt-get install qt5-qmake qt5-default qtdeclarative5-dev qml-module-qtquick-controls qml-module-qtquick-layouts
$ sudo apt-get install libpulse-dev
```

4. run rpi build script (_requires CMake and GCC6_):
```shell
$ cd deezzy
$ sh build_rpi_gcc6.sh
```

5. [Optional] List your audio devices and select the default one:
```shell
$ pacmd list-sinks
$ pacmd set-default-sink <YOUR_DEFAULT_SINK_NAME>
```

5. Start **pulseaudio** service
```shell
$ pulseaudio -D
```
6. Run **deezzy** binary in flow radio mode or given a deezer url
```shell
$ ./deezzy
```
You can found the binary in /home/pi/deezzy/bin/, and run `./deezzy` in the correct folder to start streaming.

```shell
$ ./deezzy dzmedia:///album/659384
```

## Experimental Raspbian Docker support:

I made some initial tests to run *deezzy* in a docker container, to simplify deployment and dependencies management.
In the `docker` directory you will find some usefull scripts to build and run an image containing prebuilt *deezzy*.
For now the gui/x11 part is handled, the pulseaudio part is handled too thanks to [this project](https://github.com/TheBiggerGuy/docker-pulseaudio-example), but there is still a strange bug that seems to "freeze" track play when a new track is starting.
