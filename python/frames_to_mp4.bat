@echo off
echo "Exporting Frames..."
ffmpeg.exe -r 15 -i detect_frame%%04d.jpg -c:v libx264 -vf fps=30 -pix_fmt yuv420p out.mp4
