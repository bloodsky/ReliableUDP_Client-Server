#!/bin/bash

make clean

make client

make server

if [ -f ./sample_files/test.txt ]; then
    cp ./sample_files/test.txt ./ServerFiles/test.txt
fi

if [ -f ./sample_files/silvio.jpg ]; then
    cp ./sample_files/silvio.jpg ./ServerFiles/silvio.jpg
fi

if [ -f ./sample_files/foto.jpg ]; then
    cp ./sample_files/foto.jpg ./ServerFiles/foto.jpg
fi

if [ -f ./sample_files/kolibri.iso ]; then
    cp ./sample_files/kolibri.iso ./ServerFiles/kolibri.iso
fi