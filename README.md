First, the cross-compiler needs to be installed.

packaging command:

```
make all
```

If an error is reported, try installing the following dependencies:

```
sudo apt install u-boot-tools
sudo apt install lzop
sudo apt install mtd-utils
```

If using a version of `ubuntu 22` or later, you need to manually copy `mkfs.jffs2` to the `build/build/bin` directory:

```
cp -r /usr/sbin/mkfs.jffs2 ./build/build/bin/
```