ffmpeg -f dshow -fflags nobuffer -framerate 30 -rtbufsize 1024M -i video="USB CAMERA" -c:v copy -f mjpeg "udp://127.0.0.1:1338"
