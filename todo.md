# todo list

- [ ] debug辅助工具

功能：读入磁盘内容，按照指定格式输出，以便于检查inode块、bitmap块、datablock块是否正确

实现思路：在user目录下，仿照mkfs，使用mkfs中的函数，读取块，然后格式化输出