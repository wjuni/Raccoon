@echo off
del frame*.jpg
ffmpeg.exe -i source.mp4 -r 15 frame%04d.jpg