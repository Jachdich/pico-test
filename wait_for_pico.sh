#!/bin/bash

until [[ $(find /dev -name "ttyACM*" 2>/dev/null) ]]; do
    sleep 0.1
done
sleep 0.5
FNAME=$(find /dev -name "ttyACM*" 2>/dev/null)
screen $FNAME 115200
reset
