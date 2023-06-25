#!/usr/bin/python3

import cv2 as cv
import numpy as np
from sys import argv, exit

if len(argv) < 2:
    exit('No IP address provided')


def draw_motion(still, current):
    MOTION_COLOR = [255, 20, 147]  # deeppink
    if still is None:
        print('still is none')
        return
    first = cv.cvtColor(still, cv.COLOR_RGB2GRAY)
    second = cv.cvtColor(current, cv.COLOR_RGB2GRAY)
    first = cv.GaussianBlur(first, (5, 5), 0)
    second = cv.GaussianBlur(second, (5, 5), 0)
    motion = cv.absdiff(first, second)
    motion = cv.threshold(motion, 20, 255, cv.THRESH_BINARY)[1]
    rows, cols = np.where(motion == 255)
    for i in range(len(rows)):
        current[rows[i], cols[i]] = MOTION_COLOR


url = 'tcp://' + argv[-1] + ':1338'

cap = cv.VideoCapture(url)
if not cap.isOpened():
    exit(f'Failed to open stream at {url}')
if not cap.set(cv.CAP_PROP_CONVERT_RGB, 1.0):
    exit('Failed to set CAP_PROP_CONVERT_RGB')
if cap.get(cv.CAP_PROP_CONVERT_RGB) == 0:
    exit('CAP_PROP_CONVERT_RGB not supported by FFmpeg backend')

still = None
while True:
    ret, frame = cap.read()
    orig = frame.copy()
    if ret is False:
        continue
    draw_motion(still, frame)
    if still is not None:
        cv.imshow('motion', frame)
        cv.waitKey(1)
    still = orig
cv.destroyAllWindows()
cap.release()
