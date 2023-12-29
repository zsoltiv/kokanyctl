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
from subprocess import Popen
from shutil import which

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
"""
CLASSES = ['BLASTING AGENTS',
           'COMBUSTIBLE',
           'CORROSIVE',
           'DANGEROUS',
           'EXPLOSIVES',
           'FLAMMABLE GAS',
           'FLAMMABLE SOLID',
           'FUEL OIL',
           'INHALATION HAZARD',
           'NON-FLAMMABLE GAS',
           'ORGANIC PEROXIDE',
           'OXIiDIZER',
           'OXYGEN',
           'POISON',
           'RADIOACTIVE',
           'SUD LHOR']
"""
CLASSES = [('Blas', 'Blasting Agents'),
           ('COR', 'Corrosive'),
           ('DWW', ''),
           ('Expl', 'Explosives'),
           ('FOil', 'Fuel Oil'),
           ('FS', 'Flammable solid'),
           ('FlamG', 'Flammable Gas'),
           ('IH', 'Inhalation Hazard'),
           ('NF', 'Non-Flammable Gas'),
           ('O2', 'Oxygen'),
           ('OP', 'Organic Peroxide'),
           ('Oxi', 'Oxilidizer'),
           ('PO', ''),
           ('RA', 'Radioactive'),
           ('SC', '')]
url = 'tcp://' + argv[-1] + ':1338'
#audio_url = 'tcp://' + argv[-1] + ':1340'
model = cv.dnn.readNet('yolo/model.onnx')
model.setPreferableBackend(cv.dnn.DNN_BACKEND_OPENCV)
model.setPreferableTarget(cv.dnn.DNN_TARGET_CPU)


#def start_audio_process():
#    return Popen([which('ffplay'), '-vn', audio_url, '-nodisp'])


def draw_bounding_box(frame, x, y, w, h, obj, likely):
    WIDTH_SCALE = frame.shape[1] / IMGSZ
    HEIGHT_SCALE = frame.shape[0] / IMGSZ
    print(likely.shape)
    x = int((x - w / 2) * WIDTH_SCALE)
    w = int(w * WIDTH_SCALE)
    y = int((y - h / 2) * HEIGHT_SCALE)
    h = int(h * HEIGHT_SCALE)

    cv.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 32)
    print(f'i={obj}')
    cv.putText(frame,
               CLASSES[obj][1] if CLASSES[obj][1] != '' else CLASSES[obj][0],
               (x, y - 10),
               cv.FONT_HERSHEY_SIMPLEX,
               3,
               (0, 255, 0),
               8)


def feed_model(frame):
    CONFIDENCE_THRESHOLD = 0.5
    foo = frame.copy()
    blob = cv.dnn.blobFromImage(foo, 1 / 255.0, (IMGSZ, IMGSZ), swapRB=False)
    model.setInput(blob)
    predictions = model.forward()[0]
    confident = []
    print(predictions.shape)
    for i in range(4, predictions.shape[0]):
        most_likely = np.argmax(predictions[i])
        print(f'most_likely={most_likely}')
        if predictions[i][most_likely] > CONFIDENCE_THRESHOLD:
            confident.append((i - 4, most_likely))
    for i, likely in confident:
        x = predictions[0][likely]
        y = predictions[1][likely]
        w = predictions[2][likely]
        h = predictions[3][likely]
        print(f'x={x}\ty={y}\tw={w}\th={h}')
        draw_bounding_box(frame, x, y, w, h, i, likely)


cap = cv.VideoCapture(url, cv.CAP_FFMPEG)
if not cap.isOpened():
    exit(f'Failed to open stream at {url}')
if not cap.set(cv.CAP_PROP_CONVERT_RGB, 1.0):
    exit('Failed to set CAP_PROP_CONVERT_RGB')
if cap.get(cv.CAP_PROP_CONVERT_RGB) == 0:
    exit('CAP_PROP_CONVERT_RGB not supported by FFmpeg backend')


#process = start_audio_process()
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
#    if process.poll() is None:
#        process = start_audio_process()
    framenum += 1
cv.destroyAllWindows()
cap.release()
