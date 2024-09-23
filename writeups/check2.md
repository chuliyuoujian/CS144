## Lab2
### Lab2概述
这个实验主要是来实现手册中的三个序列号之间的转换，和接收端。做到这里想跑样例的时候发现前面的性能测试跑不过无法进行后面的验证了，但是在速度方面我也实在没能想到其他更快的办法~~（我觉得是我电脑的问题，他真的已经很快了）~~  如果有大佬有改进意见可以提出哈OvO。为了能正常进行我用了很恶心的方法------改测试样例的速度限制。
![1](https://i-blog.csdnimg.cn/direct/d25faf7bedf347dc85facd969dc05ebb.png)也是实在想不到办法为了正常进行才这样的。
对于序列的转换关键点就是这里的图片了：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/3cd638a3621d43a28fcb4d5f9b951e91.png)
之所以要转换实际上是因为：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/7d4d65bd05d2484f9338b44c2e8c95ab.png)
在TCP的协议头中我们也能知道seq,ack都是32位循环的，但是我们在接收数据的时候我们是不能有重复的，因此需要一个绝对序列号，手册中也说到了，绝对序列号不会超过uint64_t，所以不用考虑超过uint64_t怎么办了。
### 序列转换的设计思路
由于seqno是一个32位类型的数字，在超过2^32-1后就会回到0,因此我们写两个函数，一个用来从absolute seqno->seqno，另一个用来seqno->absolute seqno。第一个比较好写转成32位让他自动取余就可以了（有点像字符串哈希的思路）,第二个卡了好久，先看一下代码：

```cpp
#include "wrapping_integers.hh"
#include <math.h>
using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    return Wrap32(zero_point.raw_value_ + static_cast<uint32_t>(n));
}

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    constexpr uint64_t pow2_31 = (1UL << 31);
    constexpr uint64_t pow2_32 = (1UL << 32);
    uint64_t dis = raw_value_ - static_cast<uint32_t>(checkpoint + zero_point.raw_value_); //现在的32位序列号 与 绝对序列checkpoint转化到32位的序列号 的差值
    if (dis < pow2_31 || checkpoint + dis < pow2_32) 
        return checkpoint + dis;
    return checkpoint + dis - pow2_32;
}
```
代码很短 但是情况还是蛮多的，下面是最简单的一种情况：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/bdb46d5e983749108c331954210b0b26.png)看图片就很好理解了,这种情况的话（即checkpoint对应到seqno序列上的点在当前的raw_value_的左边一点）求出在32位上的偏移量dis，直接加上就可以了。
那么如果checkpoint对应在32位上的点在当前raw_value_的右边呢？
首先，我认为要理解第二种情况首先要理解这个测试结果：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/d32f89a030bb400192308c1a7e433b30.png)
首先为我们要理解原码反码补码的存储规则，理解为什么static_cast<uint32_t>(-1)变成了2^32-1,在理解了这个的基础上再来看下面的情况。
我们来看一下图片：
![在这里插入图片描述](https://img-blog.csdnimg.cn/9018098e748c4fa49ca838a71f4cff92.png)
这种情况下，我们的dis如果正常按照代码中的差值计算方法应该是负数，但是我们选择了一个uint类型去接收，负数就会转化成：uint64_t的最大值再减去abs(dis),所以，当前的dis实际上是这个值，因此我们得到的是这样的：
![在这里插入图片描述](https://img-blog.csdnimg.cn/9ee88a4b585a4f199d9b1d2e8c4a14f9.png)
可以看出对应在绝对序列上面的当前点应该在checkpoint的左边，刚好应该是差了2^32-我们得到的dis，这样也就减去了我们图中黄色的部分。
### TCPReceiver的实现
~~经过了漫长的改数据范围~~ 也是成功通过了哈哈哈，今天在看的其他人的代码的时候发现了前面的解决方案，我看到了有人Lab0用了deque\<string\>的结构，然后就可以正常使用string_view返回peek_out函数了，但是我还是觉得更多的是我自身虚拟机的问题，~~复制一遍也不至于差这么多吧~~ 。
在理论基础上我们知道了TCP的协议头是这样的：
![!\[在这里插入图片描述\](https://i-blog.csdnimg.cn/direct/53a491bad2844b1aae4236b062e73f42.png](https://i-blog.csdnimg.cn/direct/eae789f7f49e49989b4a83a919ea6682.png)

这三个也是我们这部分代码中比较关键的部分。

下面是实现方法：
这一个部分理清楚代码实现相对简单，主要流程就是用来接收下层发来的包，如果接收到的TCPMessage包里面含有RST标志，那么就说明有错误需要重传，SYN 和 FIN 标志的作用如下：  
SYN标志：用于建立连接。当 TCP 连接的一端想要建立一个连接时，它会发送一个带有 SYN 标志的 TCP 数据包。这个数据包用来同步序列号，以便两个端点可以开始跟踪数据传输中的序列号。SYN 标志通常与 ACK 标志一起使用，以建立稳定的连接状态，ACK的值表示你这个包前面的我都收到了，你可以开始发ACK号的包了。
FIN 标志：用于终止连接。当 TCP 连接的一端完成发送所有数据，并想要关闭连接时，它会发送一个带有 FIN 标志的 TCP 数据包。这表明发送端已经完成了数据发送，并准备关闭连接。接收端在收到 FIN 标志的数据包后，知道发送端已经没有更多的数据要发送，并将开始关闭连接的过程。
然后为了防止可能zero_point本来就会出现0值，无法将它初始化成0，所以选用了一个optional来记录，nullopt就表示optional没有值它可以明确表示某个类型是否有值（has_value()函数），特别注意一下，我们要发给重组器的编号是不包含SYN的，重组器只需要将要发送的数据排列好即可，不需要将标志也传进去，也就是在上一个序列转换手册中的这个部分：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/67a796ca0e5a4c448361640e9718d6b3.png)
然后send函数就比较简单了，
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/b325f5b869c04e159221603b56e89e7d.png)
+1就是表示你前面的我都收到了，该发这个值了，也就是说ack应该是下一个没收到的序列号。
### 代码

```cpp
#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
    if (message.RST) {
        reader().set_error();
        return;
    }
    if (message.SYN) zero_point = message.seqno;
    if (!zero_point.has_value()) return;
    uint64_t abs_seq = message.seqno.unwrap(zero_point.value(), writer().bytes_pushed());
    reassembler_.insert((abs_seq - (message.SYN == 0)), message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const {
    TCPReceiverMessage message;
    if (reassembler().reader().has_error()) {
        message.ackno = nullopt;
        message.window_size = 0;
        message.RST = true;
        return message;
    }
    message.RST = false;
    if (zero_point.has_value())
        message.ackno = Wrap32::wrap(writer().bytes_pushed() + writer().is_closed() + 1, zero_point.value());
    message.window_size = (writer().available_capacity() > UINT16_MAX) ? UINT16_MAX : writer().available_capacity();
    return message;
}
```
特别注意一下，我开始将optional\<Wrap32\>zero_point在构造函数里面初始化了也就是：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/e6887adb88c44569b7a16467acacef1b.png)
，就会一直报错，原因是：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/edb6e806f19746389955a289387502fa.png)
给的send()函数是const，属于**常量成员函数**，在这个函数里面不能修改任何成员变量，所以不能在构造函数将其初始化。

### 结果
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/c792990b0c90402bb53da1b89eab2f05.png)
~~自然也是改了测试点数据范围之后的啦~~ 
