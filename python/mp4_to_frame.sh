#!/bin/bash
rm *.jpg
ffmpeg -i source.mp4 -r 15 frame%04d.jpg
