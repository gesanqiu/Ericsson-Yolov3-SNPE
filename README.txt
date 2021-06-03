
step 1
解压文件到根系统
tar -xvf ....tar.gz
cd 目录
cp -aprf * /

step 2
配置IPC网络环境
export GST_RTSPSRC_PATH=rtsp://admin:ZKCD1234@10.0.20.168:554

step 3
配置运行环境
export QTVER=qt/qt5
export QTDIR=/opt/$QTVER
export PATH=$QTDIR/bin/:$PATH
export LD_LIBRARY_PATH=$QTDIR/lib/:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$QTDIR/lib/pkgconfig/:$PKG_CONFIG_PATH
export QT_PLUGIN_PATH=$QTDIR/lib/plugins/

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/opencv/lib

export AIMuViewApp=$(dirname $(pwd))
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/faical
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/flamedetect
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/objectdetect
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/param_conf
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/yuv
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$AIMuViewApp/lib/yaml
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib:/usr/lib/aarch64-linux-gnu/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/rfsa/adsp

export XDG_RUNTIME_DIR=/usr/bin/weston_socket

step 4
执行应用
./AiObject0315