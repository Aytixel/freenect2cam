# Freenect2cam

Linux Kinect V2 driver, using **Freenect2, V4l2loopback** and **OpenCV.**

## Build

You have to have **download, build** and **install, libfreenect2, opencv, libv4l-dev** and **v4l2loopback** before!
We also recommend **installing v4l-utils.**

**⚠️ Do not forget to set up udev rules, for device access on linux, after installing libfreenect2.**

```
mkdir build && cd build
cmake -Dfreenect2_DIR=$HOME/freenect2/lib/cmake/freenect2 .. && make
```

## Run it

```
sudo modprobe v4l2loopback video_nr=0,1,2 card_label="Kinect V2 Color,Kinect V2 IR,Kinect V2 Depth" exclusive_caps=1,1,1

./build/freenect2cam /dev/video0 /dev/video1 /dev/video2

sudo modprobe -r v4l2loopback
```

## Install to load on reboot with systemd

If you use the **OpenGLPacketPipeline** from libfreenect2, you will **need more configuration,** because it **needs X11 display server to work.**

```
sudo cp ./modprobe/load-v4l2loopback.conf /etc/modules-load.d/
sudo cp ./modprobe/v4l2loopback.conf /etc/modprobe.d/
sudo cp ./systemd/freenect2cam.service /etc/systemd/system/
sudo cp ./build/freenect2cam /usr/bin/

sudo systemctl daemon-reload
sudo systemctl enable freenect2cam.service
sudo systemctl start freenect2cam.service
```

## Utility command

```
v4l2-ctl --device=/dev/video0 --all
v4l2-ctl --list-devices

ffplay /dev/video0
```
