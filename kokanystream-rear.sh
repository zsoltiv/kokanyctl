#!/bin/sh

dev="eth0"
port="1341"

ffmpeg -fflags nobuffer -f v4l2 -framerate 30 -input_format mjpeg -video_size 1280x720 -i /dev/rear-camera -c:v mjpeg -f mjpeg -vf vflip "udp://127.0.0.1:${port}?buffer_size=65536&pkt_size=512"
