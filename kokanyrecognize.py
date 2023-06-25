#!/usr/bin/python3

import cv2 as cv
from sys import argv, exit

if len(argv) < 2:
    exit('No IP address provided')


def draw_motion(still, current):
    if still is None:
        print('still is none')
        return
    first = cv.cvtColor(still, cv.COLOR_RGB2GRAY)
    second = cv.cvtColor(current, cv.COLOR_RGB2GRAY)
    first = cv.GaussianBlur(first, (5, 5), 0)
    second = cv.GaussianBlur(second, (5, 5), 0)
    motion = cv.absdiff(first, second)
    motion = cv.threshold(motion, 20, 255, cv.THRESH_BINARY)[1]
    cv.imshow('LOL!', motion)
    cv.waitKey(0)
    cv.destroyAllWindows()


url = 'tcp://' + argv[-1] + ':1338'

cap = cv.VideoCapture(url)
if not cap.isOpened():
    exit(f'Failed to open stream at {url}')

still = None
while True:
    ret, frame = cap.read()
    if ret is False:
        continue
    draw_motion(still, frame)
    still = frame
cap.release()
