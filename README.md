# file_transfer_c

实现一个简单的支持文件上传和下载的客户端-服务器程序，使用到的技术栈为socket

程序包含两个部分：服务端和客户端，服务端用于保存文件，客户端支持从服务端下载文件

服务端和客户端之间实现一个简易的文本协议

上传文件

1. 客户端输入`upload <filename>\n`
2. 客户端通过socket将该命令以及文件大小发给服务端
3. 服务端解析命令，在文件目录下创建一个新文件
4. 客户端通过socket将文件内容发送给服务端
5. 服务端读取socket缓冲区，写入新文件

下载文件

1. 客户端输入`download <filename>\n`
2. 服务端查找文件，发送查找结果，以及文件大小
3. 客户端创建新文件，准备写入
4. 服务端通过socket将文件内容发给客户
5. 客户读socket缓冲区，写入到新文件

## 项目说明

项目构建及使用

```bash
mkdir build
cd build
cmake .. && make

./server                # 启动服务端
./client <server_ip>    # 启动客户端，需要指定server_ip
```

目录结构

```
.
├── build
├── CMakeLists.txt
├── include
├── README.md
└── source
    ├── clients.c
    └── server.c
```

## note

获取文件大小的方式：

```c
fseek(file, 0, SEEK_END);       // 文件光标移动到文件末尾
filesize = ftell(file);         // 获取当前文件光标和文件头的偏移，单位Byte
fseek(file, 0, SEEK_SET);       // 恢复文件光标到初始位置
```