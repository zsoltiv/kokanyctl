#!/bin/sh

dev="eth0"
port="1338"

ffmpeg -f video4linux2 -fflags nobuffer -input_format mjpeg -video_size 1280x720 -i /dev/front-camera -fflags nobuffer -c:v copy -f mjpeg "udp://127.0.0.1:${port}"
