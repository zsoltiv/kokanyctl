ffmpeg -f dshow -fflags nobuffer -framerate 30 -rtbufsize 1024M -i video="GENERAL WEBCAM" -c:v mjpeg -vf hflip -f mjpeg "udp://127.0.0.1:1341"
