Library `fs` is the filesystem.

# 改造成ffs

## 实现思路

在磁盘第0号块保存MBR

在磁盘第1号->第logNum保存日志块

在磁盘第logNum + 1号块->第logNum + (柱面组大小) * (柱面数量)号块保存柱面组
    
    柱面组包含的内容：
    
    [ 超级块 | Inode块 | bitmap块 | 数据块 ]

## 保持不变的地方

读取块的方式不变，依旧是向底层发送读取块的block编号，即block地址

## 需要修改的地方

1. 文件的放置
