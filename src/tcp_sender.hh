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
  uint64_t abs_ack_ { 0 };                    // ack的绝对序列号
  uint64_t abs_old_ack_ { 0 };                // 上一个ack用来当checkpoint
  uint64_t sequence_numbers_in_flight_ { 0 }; // 发送了但还没收到ack的字符数(outstanding的字符数量)
  uint64_t consecutive_retransmissions_ { 0 };
  uint64_t time_ms_ { 0 }; // 计时器时间
  uint16_t window_size_ { 1 };
  std::priority_queue<TCPSenderMessage, std::vector<TCPSenderMessage>, my_cmp> msg_queue_ {};
  bool FIN_ { false };
};
