#!/bin/sh

# Requires imagemagick
# This script generates annotations for all the hazmat labels
# in YOLO format

WORKDIR="yolo/placard"
for img in $WORKDIR/*.png ; do
    # the images only contain the placards
    width="1"
    height="1"
    x_center="0.5"
    y_center="0.5"
    class="$(basename $img | cut -d'.' -f1)"
    echo "$class $x_center $y_center $width $height" > "${WORKDIR}/${class}.txt"
done
