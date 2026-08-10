一个游戏，做什么我还没想好，反正想到什么做什么

请使用我的常用邮箱与我联系 sdfzngfxh@outlook.com

哦对了，本仓库的所有用AI的提交都会注明

编译：

```
# 这些命令是我临时写的，没有经过运行测试，等我测试了我会把这句话删掉
# 对于debian系用户，换成apt安装，windows用户可以使用winget，windows用户请将lmms.exe添加至环境变量
#必须
sudo pacman -S llvm21-libs llvm21 ninja cmake lmms clang
#第三方库（SDL依赖）
sudo pacman -S alsa-lib cmake hidapi ibus jack libdecor libthai fribidi libgl libpulse libusb libx11 libxcursor libxext libxfixes libxi libxinerama libxkbcommon libxrandr libxrender libxss libxtst mesa ninja pipewire sndio vulkan-driver vulkan-headers wayland wayland-protocols
mkdir build
cd build
# 对于中国大陆的用户，由于Github连接不稳定甚至无法连接，可以多尝试几次运行这个命令
# 或者使用 git config --global http.proxy http://<proxy-server>:<port>
#         git config --global https.proxy https://<proxy-server>:<port> 
cmake -G "Ninja" ..
ninja && ninja BGM && ninja PackRes
```

| 功能 | 实现状态 | 备注 |
| :----- | :----- | :----- |
|跨平台|OK||
| 数据文件的加载/保存 | [完成] | |
| DataManager的快照 | 存疑 |AI |
| GUI支持(绘制图片(SVG)/文本) | 部分||
| 多线程Lua(Worker) | [完成]|AI|
|Logger|[完成]||
|BGM/SFX播放|[完成]||
|命令行参数处理|[完成]||
|长的像OOP的ECS|没头绪||
|OpenCL的渲染器|起步||
|矢量3D渲染|没头绪||
