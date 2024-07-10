ffmoeg -f dshow -fflags nobuffer -framerate 30 -i video="GENERAL WEBCAM" -c:v mjpeg -vf vflip -f mjpeg "udp://127.0.0.1:1341"
