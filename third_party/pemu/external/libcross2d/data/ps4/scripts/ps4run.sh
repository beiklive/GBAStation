#!/bin/bash
curl -T $1 ftp://$2:2121/data/eboot.bin
tio /dev/ttyUSB0
