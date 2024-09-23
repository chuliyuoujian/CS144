# Lab0概述
Lab0主要是通过对一些封装好的命令的使用体验一下计算机网络通信的魅力，并实现TCP协议栈中的字节流部分。
## fetch界面
这个我用手册中的命令没能实现，用curl命令替代了。
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/e467c2ed7df540c2ad52f0de49c59b93.png)
### 互联
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/94daf4765fa443bebb497a61c4191385.png)
### 利用Linux下自己的TCPsocket写一个fetch界面的程序
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/8e143f7304c5468088c70c246bdd9d98.png)主要就是通过GET命令来获取网页，在 HTTP 协议中，Host 头部用于指定请求的服务器的域名，而 Connection: close 则用于指示服务器在发送完响应后关闭 TCP连接。

**这就是使用TCPsocket写的一个获取网页的程序，在接下来的实验中会一步一步实现一个自己的TCPsocket！**

接下来是Lab0中最重要的一部分：
## 实现字节流
首先要了解字节流是干什么的，我们要实现一个字节流用来把数据从下层传递给上层，也就是说这个字节流**要有读写功能**，下层来写数据，上层来读取数据。
## 代码：
```cpp
  #include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity)
    : capacity_(capacity), end_write(false), end_read(false), write_size(0), read_size(0), buffer() {}

bool Writer::is_closed() const {
    return end_write;
}

void Writer::push(string data) {
    if (available_capacity() == 0)
        return;
    size_t num = min(data.size(), available_capacity());
    buffer.insert(buffer.end(), make_move_iterator(data.begin()), make_move_iterator(data.begin() + num));
    write_size += num;
}

void Writer::close() {
    end_write = true;
}

uint64_t Writer::available_capacity() const {
    uint64_t num = capacity_ - buffer.size();
    return num;
}

uint64_t Writer::bytes_pushed() const {
    return write_size;
}

bool Reader::is_finished() const {
    return (end_write && buffer.empty()) ? true : false;
}

uint64_t Reader::bytes_popped() const {
    return read_size;
}

string Reader::peek() const {
    return std::string(buffer.begin(), buffer.end());
}

void Reader::pop(uint64_t len) {
    if (len > buffer.size()) {
        set_error();
        return;
    }
    uint64_t tmp = len;
    while (tmp--) {
        buffer.pop_front();
    }
    read_size += len;
}

uint64_t Reader::bytes_buffered() const {
    return buffer.size();
}
```
在数据结构的选择上，由于需要支持读写操作，所以是需要在尾部插入，在头部读取，因此我用了deque，但是因为我这个版本的实验中Reader的peek函数给的是用string_view返回，在我的一次跑样例的时候出现了返回野指针的错误，通过看大佬的博客看到好像是因为 C++ 里的 deque 并不完全内存连续即在多次写入后会在新地区开新内存，又因为string_view只存地址和偏移导致 string_view 的内容有问题。当多次调用 deque的push 和 pop 后，deque 在新开内存会导致内存不连续，进而导致 string_view 内容会存在 \0x00 的溢出。
但我又没有找到好的解决方法，~~所以这里我偷偷把他的string_view改成string了。~~ 我觉得正常他应该是想让你手写一个类似循环队列的东西，这里就偷个懒吧。
特别注意一下，C++构造函数初始化的顺序只和定义变量的顺序有关，所以要先初始化capacity_，再用capacity_来初始化buffer的大小。
然后为了优化性能减少复制，我又使用了move_iterator来优化，但我的性能和测试样例还是只能在网速够快的情况下通过，但我没能找到更好的办法，不知道是不是deque也会导致性能变慢,如果有大佬能解决可以教教我。
