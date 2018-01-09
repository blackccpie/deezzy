FROM resin/rpi-raspbian:stretch

ENV UNAME deezzy

RUN apt-get update && \
    apt-get -y install unzip wget && \
    apt-get -y install build-essential gcc-6 g++-6 cmake git libpulse-dev && \
    apt-get -y install qt5-qmake qt5-default libqt5svg5 qtdeclarative5-dev qml-module-qtquick-controls qml-module-qtquick-layouts && \
    wget https://build-repo.deezer.com/native_sdk/deezer-native-sdk-v1.2.10.zip && \
    unzip deezer-native-sdk-v1.2.10.zip && rm deezer-native-sdk-v1.2.10.zip && \
    git clone https://github.com/blackccpie/deezzy.git

COPY private_user.h deezzy/src/deezer_wrapper/private

RUN cd deezzy && sh build_rpi_gcc6.sh

COPY pulse-client.conf /etc/pulse/client.conf

COPY configure-deezzymage.sh /opt

RUN chmod 755 /opt/configure-deezzymage.sh

# Set up the user
RUN export UNAME=$UNAME UID=1000 GID=1000 && \
    mkdir -p "/home/${UNAME}" && \
    echo "${UNAME}:x:${UID}:${GID}:${UNAME} User,,,:/home/${UNAME}:/bin/bash" >> /etc/passwd && \
    echo "${UNAME}:x:${UID}:" >> /etc/group && \
    mkdir -p /etc/sudoers.d && \
    echo "${UNAME} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${UNAME} && \
    chmod 0440 /etc/sudoers.d/${UNAME} && \
    chown ${UID}:${GID} -R /home/${UNAME} && \
    gpasswd -a ${UNAME} audio

USER $UNAME

CMD ["/opt/configure-deezzymage.sh"]
