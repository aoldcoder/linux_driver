# linux_driver
add a NTFS driver for kernel-3.18 ,apply MTK plateform

android挂载NTFS U盘
一.下载ntfs-3g解压到external
二.编译external/ntfs-3g,out/target/product目录下生成二进制文件
三.修改system/vold目录下文件PublicVolume.cpp增加类型ntfs
