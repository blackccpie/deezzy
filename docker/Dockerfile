FROM resin/rpi-raspbian:stretch

RUN apt-get update && \
    apt-get -y install unzip wget && \
    apt-get -y install build-essential gcc-6 g++-6 cmake git libpulse-dev && \
    apt-get -y install qt5-qmake qt5-default qtdeclarative5-dev qml-module-qtquick-controls qml-module-qtquick-layouts && \
    wget https://build-repo.deezer.com/native_sdk/deezer-native-sdk-v1.2.10.zip && \
    unzip deezer-native-sdk-v1.2.10.zip && rm deezer-native-sdk-v1.2.10.zip && \
    git clone https://github.com/blackccpie/deezzy.git

COPY configure-deezzymage.sh /opt

RUN chmod 755 /opt/configure-deezzymage.sh

CMD ["/opt/configure-deezzymage.sh"]
