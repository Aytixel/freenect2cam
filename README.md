# Freenect2cam

Linux Kinect V2 driver, using **Freenect2, V4l2loopback** and **OpenCV.**

## Build

You have to have **download, build** and **install, libfreenect2, opencv, libv4l-dev** and **v4l2loopback** before!
We also recommend installing v4l-utils.

```
mkdir build && cd build
cmake -Dfreenect2_DIR=$HOME/freenect2/lib/cmake/freenect2 .. && make
```

## Run it

```
sudo modprobe v4l2loopback video_nr=0 card_label="Kinect V2 Color" exclusive_caps=1

./build/freenect2cam /dev/video0

sudo modprobe -r v4l2loopback
```

## Utility command

```
v4l2-ctl --device=/dev/video0 --all
v4l2-ctl --list-devices

ffmpeg -f x11grab -framerate 60 -video_size 1920x1080 -i :1.0 -pix_fmt yuv420p -f v4l2 /dev/video0
ffplay /dev/video0
```