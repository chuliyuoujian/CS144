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
 //if(window_size_<sequence_numbers_in_flight_) return;
 if ( ( window_size_ && sequence_numbers_in_flight_ >= window_size_ )
       || ( window_size_ == 0 && sequence_numbers_in_flight_ >= 1 ) ) {
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

  while(!view.empty()||seq_==isn_||(!FIN_&&writer().is_closed())) {
          len=min(view.size(),TCPConfig::MAX_PAYLOAD_SIZE);
          //string payload=view.substr(0,len);
		  string payload( view.substr( 0, len ) );
          //这里手册中提到不能有FIN直接就发，要看情况 可能留到下次发FIN
          TCPSenderMessage message {seq_,seq_==isn_,payload,false,writer().has_error()};
          //看看FIN够不够发
          if(!FIN_&&writer().is_closed()&&len==view.size()&&(sequence_numbers_in_flight_ + message.sequence_length()<window_size_||( window_size_ == 0 && message.sequence_length() == 0) )) { FIN_=message.FIN=true;
        }
          transmit(message);
          abs_ack_+=message.sequence_length();
          sequence_numbers_in_flight_+=message.sequence_length();
          msg_queue_.push(move(message));
          if(!FIN_&&writer().is_closed()&&len==view.size()) break;
          seq_=Wrap32::wrap(abs_ack_,isn_);
		  view.remove_prefix(len);
  }
/*  string_view view( s );

  while ( !view.empty() || seq_ == isn_ || ( !FIN_ && writer().is_closed() ) ) {
    len = min( view.size(), TCPConfig::MAX_PAYLOAD_SIZE );

    string payload( view.substr( 0, len ) );

    TCPSenderMessage message { seq_, seq_ == isn_, move( payload ), false, writer().has_error() };
    if ( !FIN_ && writer().is_closed() && len == view.size()
         && ( sequence_numbers_in_flight_ + message.sequence_length() < window_size_
              || ( window_size_ == 0 && message.sequence_length() == 0 ) ) ) {
      FIN_ = message.FIN = true;
    }

    transmit( message );

    abs_ack_ += message.sequence_length();
    sequence_numbers_in_flight_ += message.sequence_length();
    msg_queue_.emplace( move( message ) );

    // 当前窗口大小限制携带不了FIN，留着以后发，没有新的消息了直接退出
    if ( !FIN_ && writer().is_closed() && len == view.size() ) {
      break;
    }

    seq_ = Wrap32::wrap( abs_ack_, isn_ );
    view.remove_prefix( len );
}*/
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
  //*************
  uint64_t abs_seq_k = msg.ackno ? msg.ackno.value().unwrap( isn_, abs_old_ack_ ) : 0;
  if ( abs_seq_k > abs_old_ack_ && abs_seq_k <= abs_ack_ ) {
    abs_old_ack_ = abs_seq_k;

    // 计时器清0
    time_ms_ = 0;
    RTO_ms_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;

    uint64_t abs_seq = 0;
    while ( !msg_queue_.empty() && abs_seq <= abs_seq_k ) {
      abs_seq = msg_queue_.top().seqno.unwrap( isn_, abs_old_ack_ ) + msg_queue_.top().sequence_length();
      if ( abs_seq <= abs_seq_k ) {
        sequence_numbers_in_flight_ -= msg_queue_.top().sequence_length();
        msg_queue_.pop();
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
  if ( time_ms_ >= RTO_ms_ ) {
    transmit( msg_queue_.top() );
    if ( window_size_ > 0 ) {
      ++consecutive_retransmissions_;
      RTO_ms_ <<= 1;
    }
    time_ms_ = 0;
  }
}
