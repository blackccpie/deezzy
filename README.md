# deezzy
Lightweight Qml/C++ Deezer Player using Native SDK.

**deezzy** has no search/playlist management, it's goal is  to _**play your user's flow radio**_.

![Deezzy](http://blackccpie.free.fr/deezzy/deezzy.png)

- UI Layout inspired by : [qml-audio-player](https://github.com/rohitsinghsengar/qml-audio-player)
- Deezer API : [Native SDK](http://developers.deezer.com/sdk/native)
- Third party JSON library: [JSON for Modern C++](https://github.com/nlohmann/json)

## Running on the Raspberry Pi:

1. Start **pulseaudio** service
```shell
$ pulseaudio -D
```
2. Run **deezzy** binary
```shell
$ ./deezzy
```

![Deezzy-RPi](http://blackccpie.free.fr/deezzy/deezzy-rpi.jpg)

