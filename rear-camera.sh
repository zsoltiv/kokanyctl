#!/bin/sh

port="1341"
address="$(ip -4 -br a show eth0 | grep -Eo '192\.168\.69\.[0-9]+')"
ffplay -fflags nobuffer "udp://${address}:${port}"
