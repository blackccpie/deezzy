#!/bin/sh

set -x

USER_UID=$(id -u)

pulseaudio --start

xhost +local:docker
docker run	--rm -ti \
			-e DISPLAY=$DISPLAY \
			-v /tmp/.X11-unix:/tmp/.X11-unix \
			--volume=/run/user/${USER_UID}/pulse:/run/user/1000/pulse \
			deezzymage
xhost -local:docker
