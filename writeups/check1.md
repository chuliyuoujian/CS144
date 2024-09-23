# Lab1
## Lab1概述
Lab1是让我们实现重组器，通过理论课我们知道了**网络层的数据是不可靠的**，那么TCP协议是怎么实现的"可靠"呢？通过这个实验我们可以很好的了解TCP怎么保证数据的“可靠”。
## 设计Lab1的思路
首先看一下手册上面给的信息：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/4f6cc4b8bfe44e7b8a02a8e8de50347c.png)
手册中提到**每一个byte都有一个标号index，并且index从0开始**，这个从0开始也为我们函数里面计算图片中的first unpopped index,first unassembled index,first unacceptable index省去了许多+1的麻烦。
然后来分析怎么去写：
首先我们需要存储图片中的整个数据，也就是所有想要写进字节流的字符，因为每一个字符都有一个index标号，所以我们可以想到用map这种key-value的数据结构来一对一存储。
由于网络层可能涉及到乱序，重传等多种情况，所以我们应该考虑怎么将这些数据放进字节流，手册中也说明了，**只有当某一个编号的字符前面所有的字符都知道了，这一串字符才会被写入字节流**，也就是说加入我们得到了编号是3的字符，那么只有我知道了0123,才会将其写入，否则我们就先不写入字节流口，而是等待，等到我们知道了0123再传入。
因此，我们再看手册中的图片可知：
**蓝色部分指的是：已经写进字节流的字符
绿色部分指的是写进了map结构（buf_）已经有序了，但还没向字节流里写的数据。
红色部分指的是已经在map里了但是他前面的字符还不知道因此不能传进字节流的数据。**
**主要的过程就是我们从上一层收到字符串data，传给我们写的这个map结构，这时map得到的需要是没有重复的数据，然后当这个标号的前面所有字符都知道了就将这一串字符写进字节流供给上层读取。**
可以分情况讨论，比如：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/9c625fc8a68a4061b2efbc0b1754b74d.png)这种情况下在绿色部分的数据就不要，只把红色部分存进map即可。
而以下这种因为没有收到过，不涉及重复，全存即可：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/b121f70c2625445db7a5fe5e00869a35.png)注意我们在Lab0实现的字节流是有容量限制的，我们不能超出这个容量限制。
## 代码：

```cpp
#include "reassembler.hh"
#include <sstream>
using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    if (is_last_substring) {
        flag_eof = true;
        index_eof = first_index + data.size();
    }
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
        if (first_index + i >= get_unsort() && first_index + i < get_unaccept()) {
            buf_[first_index + i] = data[i];
        }
    }
    string str = "";
    ostringstream str_stream;
    size_t now = get_unsort(); /* 已经插入的字符 */
    for (auto& to : buf_) {
        size_t id = to.first;
        char ch = to.second;
        if (id > now || now >= get_unaccept())
            break;
        if (id < now) {
            continue;
        }
        if (id == now) {
            str_stream << ch;
            ++now;
        }
    }
    str = str_stream.str();
    output_.writer().push(str);
    if (flag_eof && get_unsort() == index_eof)
        output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const {
    return buf_.size() - output_.writer().bytes_pushed();
}

```

性能测试目前还是无法通过的，不知道是不是Lab0的问题。
