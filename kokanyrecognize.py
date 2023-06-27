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


IMGSZ = 640
url = 'tcp://' + argv[-1] + ':1338'
model = cv.dnn.readNet('yolo/model.onnx')
model.setPreferableBackend(cv.dnn.DNN_BACKEND_OPENCV)
model.setPreferableTarget(cv.dnn.DNN_TARGET_CPU)


def draw_bounding_box(frame, detection):
    x, y, w, h = detection[0], detection[1], detection[2], detection[3]
    WIDTH_SCALE = frame.shape[0] / IMGSZ
    HEIGHT_SCALE = frame.shape[1] / IMGSZ
    x = int(x * WIDTH_SCALE)
    w = int(abs(x - w) / 2 * WIDTH_SCALE)
    y = int(y * HEIGHT_SCALE)
    h = int(abs(y - h) / 2 * HEIGHT_SCALE)
    cv.rectangle(frame, (x, y), (x + w, y + h), (255, 0, 0), 2)


def feed_model(frame):
    CONFIDENCE_THRESHOLD = 0.5
    foo = frame.copy()
    blob = cv.dnn.blobFromImage(foo, 1 / 255.0, (IMGSZ, IMGSZ), swapRB=False, crop=False)
    model.setInput(blob)
    predictions = model.forward()[0]
    confident = np.extract(predictions[:,4] > CONFIDENCE_THRESHOLD, predictions)
    draw_bounding_box(frame, predictions[0])


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
    feed_model(frame)
    if still is not None:
        cv.imshow('motion', frame)
        cv.waitKey(1)
    still = orig
cv.destroyAllWindows()
cap.release()
