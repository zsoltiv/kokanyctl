#!/usr/bin/python3

"""
copyright (c) 2023 Zsolt Vadasz

This file is part of kokanyctl.

kokanyctl is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

kokanyctl is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with kokanyctl. If not, see <https://www.gnu.org/licenses/>.
"""

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
CLASSES = [('Blas', 'Blasting Agents'),
           ('COR', 'Corrosive'),
           ('DWW', 'Flammable Gas'),
           ('Expl', 'Explosives'),
           ('FOil', 'Fuel Oil'),
           ('FS', 'Flammable solid'),
           ('FlamG', ''),
           ('IH', 'Inhalation Hazard'),
           ('NF', 'Non-Flammable Gas'),
           ('O2', 'Oxygen'),
           ('OP', 'Organic Peroxide'),
           ('Oxi', 'Oxilidizer'),
           ('PO', ''),
           ('RA', 'Radioactive'),
           ('SC', 'Spontaneously Combustible')]
url = 'udp://' + argv[-1] + ':1338?overrun_nonfatal=1'
model = cv.dnn.readNet('yolo/model.onnx')
model.setPreferableBackend(cv.dnn.DNN_BACKEND_OPENCV)
model.setPreferableTarget(cv.dnn.DNN_TARGET_CPU)


def draw_bounding_box(frame, x, y, w, h, obj):
    WIDTH_SCALE = frame.shape[1] / IMGSZ
    HEIGHT_SCALE = frame.shape[0] / IMGSZ
    x = int(x * WIDTH_SCALE)
    w = int(w * WIDTH_SCALE)
    y = int(y * HEIGHT_SCALE)
    h = int(h * HEIGHT_SCALE)

    cv.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 32)
    cv.putText(frame,
               CLASSES[obj][1] if CLASSES[obj][1] != '' else CLASSES[obj][0],
               (x, y - 10),
               cv.FONT_HERSHEY_SIMPLEX,
               3,
               (0, 255, 0),
               8)


def box(row):
    # [x y w h]
    return [
        row[0] - row[2] / 2,
        row[1] - row[3] / 2,
        row[2],
        row[3],
    ]


def feed_model(frame):
    CONFIDENCE_THRESHOLD = 0.25
    blob = cv.dnn.blobFromImage(frame, 1 / 255, (IMGSZ, IMGSZ), swapRB=True)
    model.setInput(blob)
    predictions = np.transpose(model.forward()[0])
    rows = predictions.shape[0]
    boxes = []
    confidences = []
    class_ids = []

    for i in range(rows):
        _, confidence, _, (_, class_id) = cv.minMaxLoc(predictions[i][4:])
        if confidence >= CONFIDENCE_THRESHOLD:
            boxes.append(box(predictions[i]))
            confidences.append(confidence)
            class_ids.append(class_id)

    indices = cv.dnn.NMSBoxes(boxes,
                              confidences,
                              CONFIDENCE_THRESHOLD,
                              0.45,
                              0.5)
    for i in indices:
        x, y, w, h = boxes[i]
        class_id = class_ids[i]
        draw_bounding_box(frame, x, y, w, h, class_id)


cap = cv.VideoCapture(url, cv.CAP_FFMPEG)
if not cap.isOpened():
    exit(f'Failed to open stream at {url}')
if not cap.set(cv.CAP_PROP_CONVERT_RGB, 1.0):
    exit('Failed to set CAP_PROP_CONVERT_RGB')
if cap.get(cv.CAP_PROP_CONVERT_RGB) == 0:
    exit('CAP_PROP_CONVERT_RGB not supported by FFmpeg backend')


still = None
framenum = 0
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
    if framenum % 10 == 0:
        still = orig
    framenum += 1
cv.destroyAllWindows()
cap.release()
