首先需安装交叉编译器。

打包命令

```
make all
```

若报错，尝试安装以下依赖项

```
sudo apt install u-boot-tools
sudo apt install lzop
sudo apt install mtd-utils
```

若使用 `ubuntu 22` 以上版本需手动拷贝 mkfs.jffs2到 `build/build/bin` 目录
```
cp -r /usr/sbin/mkfs.jffs2 ./build/build/bin/
```