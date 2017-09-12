@echo off
del detect_*.jpg
del frame*.jpg
ffmpeg.exe -i %1 -r 30 frame%%04d.jpg
python candidate_1.py
rm %1_converted.mp4
ffmpeg.exe -r 30 -i detect_frame%%04d.jpg -c:v libx264 -vf fps=30 -pix_fmt yuv420p %1_converted.mp4
del detect_*.jpg
del frame*.jpg