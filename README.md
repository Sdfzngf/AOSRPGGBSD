一个游戏，做什么我还没想好，反正想到什么做什么

哦对了，本仓库的所有用AI的提交都会注明

编译：

```
# 这些命令是我临时写的，没有经过运行测试，等我测试了我会把这句话删掉
# 对于debian系用户，换成apt安装，windows用户可以使用winget，windows用户请将lmms.exe添加至环境变量
sudo pacman -S llvm21-libs llvm21 ninja cmake  lmms
mkdir build
cd build
# 对于中国大陆的用户，由于Github连接不稳定甚至无法连接，可以多尝试几次运行这个命令
# 或者使用 git config --global http.proxy http://<proxy-server>:<port>
#         git config --global https.proxy http://<proxy-server>:<port> 
cmake -G "Ninja" ..
ninja && ninja PackRes
```
