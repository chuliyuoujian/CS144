## Lab3
### Lab3概述
Lab3是让我们实现TCP的Sender部分,是要我们完成TCP中的将数据发送给对方，并且受到对方的ack，改变窗口大小来进行拥塞控制的过程，在完成这一部分的实现前，我们一定要清楚，TCP协议连接的是对等的实体，**任何一方都既可以是接收方，也可以是发送方。**
我们要在这一部分主要实现三个函数：push,receive和tick。
#### push要求：
    void push(const TransmitFunction& transmit)：
    TCP发送器被要求从传出字节流填充窗口：它从流中读取数据，并尽可能多地发送TCP发送器消息，只要还有新的字节可读且窗口中有可用空间。它通过调用提供的 transmit() 函数来发送它们。
    你需要确保你发送的每个TCP发送器消息完全适合接收方的窗口。使每个单独的消息尽可能大，但不要超过 TCPConfig::MAX_PAYLOAD_SIZE（1452字节）的值。
    你可以使用 TCPSenderMessage::sequence length() 方法来计算一个段所占用的序列号总数。记住，SYN和FIN标志也各占用一个序列号，这意味着它们在窗口中占用空间。
    如果窗口大小为零，我应该怎么做？如果接收方宣布窗口大小为零，push 方法应该假装窗口大小为一。发送方可能会发送一个最终被接收方拒绝（并且未确认）的单个字节，但这也可以促使接收方发送一个新的确认段，在其中它透露其窗口中有更多的空间已经打开。没有这个，发送方将永远不会知道它被允许开始发送。
    这是你的实现应该对零大小窗口情况有的唯一特殊行为。TCP发送器实际上不应该记住一个假的窗口大小为1。特殊情况只在 push 方法中。
特别注意一下，push函数的要求是填充窗口，是uint64_t类型，但是手册中还有要求：使每个单独的消息尽可能大，但不要超过 TCPConfig::MAX_PAYLOAD_SIZE（1452字节）的值，这意味着**每次调用push是发送填满窗口的包，而不是只发送一个包!!!**这里因为没搞清楚一直卡在一个点上。
并且一定要记住：如果窗口大小为零，我应该怎么做？如果接收方宣布窗口大小为零，push 方法应该假装窗口大小为一。
#### receive要求
    void receive(const TCPReceiverMessage& msg);
    从接收方收到一条消息，这条消息传达了窗口的新左边界（即确认号 ackno）和右边界（即确认号 ackno 加上窗口大小）。TCP发送器应该查看其未确认段的集合，并移除任何现在已经完全确认的段（即确认号大于该段中的所有序列号）。
#### tick要求
    void tick( uint64 t ms since last tick, const TransmitFunction& transmit );时间已经过去了——自上次调用这个方法以来已经过去了一定数量的毫秒。     发送方可能需要重传一个未确认的段；它可以通过调用 transmit() 函数来做到这一点。（提醒：请不要尝试在你的代码中使用现实世界的“时钟”函数；时间流逝的唯一参考来自自上次滴答以来的毫秒数参数。）

读完文档的要求时可以边读边整理一下我们要做的事情：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/a6721d41107a4d84bf5df7bb94b55aa6.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/0513c7a03ab849e1bb265e724bb1787b.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/903bf3823ac9403f8b81b646438ffd1e.png)
这些是我们读完文档就能够整理出来的，知道了这些实现也会更加清晰。
### 数据结构的选择和成员变量的添加
首先整理完文档的内容很容易知道我们需要一个数据结构来维护所有outstanding的数据，容易想到因为收到ack后我们要根据序列号更新数据结构，因此我用了优先队列，写了一个伪函数来按照seqno排序：
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/bede40a55e1748d5aba98cc90f8c4227.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/fc2be683a900468b887e23e17cbc1af1.png)
这样就可以在收到ack后快速弹出已经不再outstanding的Message。
**注意这里我的成员变量abs_ack_不是发送一个包的ack而是整个push发送完的ack。**
### 代码

```cpp
#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }
  struct my_cmp
  {
    bool operator()( TCPSenderMessage a, TCPSenderMessage b ) { return a.seqno > b.seqno; }
  };

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_{initial_RTO_ms_};                  // 现在的RTO,在初始化列表初始化了
  uint64_t abs_ack_ { 0 };                    // ack的绝对序列号（如果把push发的当成一个大包的话，这个是大包的ack而不是小包的）
  uint64_t abs_old_ack_ { 0 };                // 上一个ack用来当checkpoint
  uint64_t sequence_numbers_in_flight_ { 0 }; // 发送了但还没收到ack的字符数(outstanding的字符数量)
  uint64_t consecutive_retransmissions_ { 0 };
  uint64_t time_ms_ { 0 }; // 计时器时间
  uint16_t window_size_ { 1 };
  std::priority_queue<TCPSenderMessage, std::vector<TCPSenderMessage>, my_cmp> msg_queue_ {};
  bool FIN_ { false };
};
```

```cpp
#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
	// Your code here.
	return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
	// Your code here.
	return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
	// Your code here.
	// 如果窗口已经比当前缓存的数据还小了，，就先不发送了(滑动窗口)
	if (sequence_numbers_in_flight_ >= (window_size_!=0?window_size_:1) ) {
		return;
	}
	Wrap32 seq_=Wrap32::wrap(abs_ack_,isn_);
	//计算当前还能接受的大小
	uint16_t
		accept_num_=(window_size_==0?1:window_size_-sequence_numbers_in_flight_-static_cast<uint16_t>(seq_==isn_));
	//得到能够传输的字符串
	string s="";
	if(reader().bytes_buffered()) {
		s=reader().peek();
		s=s.substr(0,accept_num_);
		input_.reader().pop(s.size());
	}
	size_t len=0;
	string_view view(s);
	while(view.size()||seq_==isn_||(!FIN_&&writer().is_closed())) {
		len=min(view.size(),TCPConfig::MAX_PAYLOAD_SIZE);
		string payload( view.substr( 0, len ) );
		//这里手册中提到不能有FIN直接就发，要看情况 可能留到下次发FIN
		TCPSenderMessage message {seq_,seq_==isn_,payload,false,writer().has_error()};
		//看看FIN够不够发
		if(!FIN_&&writer().is_closed()&&len==view.size()&&(sequence_numbers_in_flight_ + message.sequence_length()<(window_size_==0?1:window_size_) )) { FIN_=message.FIN=true;
		}
		transmit(message);
		abs_ack_+=message.sequence_length();
		sequence_numbers_in_flight_+=message.sequence_length();
		msg_queue_.push(move(message));
		if(!FIN_&&writer().is_closed()&&len==view.size()) break;
		seq_=Wrap32::wrap(abs_ack_,isn_);
		view.remove_prefix(len);
	}
}

TCPSenderMessage TCPSender::make_empty_message() const
{
	// Your code here.
	return { Wrap32::wrap( abs_ack_, isn_ ), false, "", false, writer().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	// Your code here.
	if ( msg.RST ) {
		writer().set_error();
		return;
	}
	window_size_ = msg.window_size;

	uint64_t abs_recv_ack_ = msg.ackno ? msg.ackno.value().unwrap( isn_, abs_old_ack_ ) : 0;
	if ( abs_recv_ack_ > abs_old_ack_ && abs_recv_ack_ <= abs_ack_ ) {
		abs_old_ack_ = abs_recv_ack_;
		// 计时器清0
		time_ms_ = 0;
		//RTO设置为初始值
		RTO_ms_ = initial_RTO_ms_;
		//连续重传次数清零
		consecutive_retransmissions_ = 0;

		uint64_t abs_seq = msg_queue_.top().seqno.unwrap( isn_, abs_old_ack_ ) +     msg_queue_.top().sequence_length();
		while ( !msg_queue_.empty() && abs_seq <= abs_recv_ack_ ) {
			if ( abs_seq <= abs_recv_ack_ ) {
				sequence_numbers_in_flight_ -= msg_queue_.top().sequence_length();
				msg_queue_.pop();
				abs_seq = msg_queue_.top().seqno.unwrap( isn_, abs_old_ack_ ) +     msg_queue_.top().sequence_length();
			}
		}
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	// Your code here.
	if ( !msg_queue_.empty() ) {
		time_ms_ += ms_since_last_tick;
	}
	// 如果在重传之前收到ack并且队列为空，则不需要重传了，此时timer相关信息已经清0
	if ( time_ms_ >= RTO_ms_ ) {//等待时间大于RTO，重传
		transmit( msg_queue_.top() );
		if ( window_size_ > 0 ) {//注意window_size_=0不触发RTO避退
			++consecutive_retransmissions_;
			RTO_ms_ <<= 1;
		}
		time_ms_ = 0;
	}
}
```

### 结果
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/db526a9aa116496fb601d48446da9a11.png)
### 总结
这一部分的实现一定要读清楚文档中的内容，就比如前面提到的push函数发送多少数据的问题，一定要搞清楚，以及我们所设计的成员变量应当在哪一部分正确的更新，要提前思考清楚。
同时在写这个Lab的时候也可以思考一些其他的知识，比如我在这个Lab的实现中也更加清楚了SYN_Flood泛洪攻击时为什么半连接队列默认是1024时，每秒发送200个伪造的SYN包就足够撑爆半连接队列，因为重传时RTO会不断地×2，以至于默认重传5次的话，一个伪造的SYN报文就会占用1+2+4+8+16+32(最后一次还要等32s才知道最后也超时了)，因此一个伪造的SYN包就会占用63s之久。
