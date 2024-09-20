#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reader().set_error();
    return;
  }
  if ( message.SYN )
    zero_point = message.seqno;
  if ( !zero_point.has_value() )
    return;
  uint64_t abs_seq = message.seqno.unwrap( zero_point.value(), writer().bytes_pushed() );
  reassembler_.insert( ( abs_seq - ( message.SYN == 0 ) ), message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message;
  if ( reassembler().reader().has_error() ) {
    message.ackno = nullopt;
    message.window_size = 0;
    message.RST = true;
    return message;
  }
  message.RST = false;
  if ( zero_point.has_value() )
    message.ackno = Wrap32::wrap( writer().bytes_pushed() + writer().is_closed() + 1, zero_point.value() );
  message.window_size = ( writer().available_capacity() > UINT16_MAX ) ? UINT16_MAX : writer().available_capacity();
  return message;
}
