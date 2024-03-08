#!/bin/sh

address="$1"
port="1340"

ffplay -nodisp "tcp://${address}:${port}"
