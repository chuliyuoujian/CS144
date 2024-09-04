#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), end_write( false ), end_read( false ), write_size( 0 ), read_size( 0 ), buffer()
{}

bool Writer::is_closed() const
{
  // Your code here.
  return end_write;
}

void Writer::push( string data )
{
  // Your code here.
  /*if(available_capacity()==0) return;
  int64_t num=min(data.size(),available_capacity());
  for(int i=0;i<num;++i) {
          buffer.push_back(data[i]);
  }
  write_size+=num;
  return;*/
  if ( available_capacity() == 0 )
    return;
  size_t num = min( data.size(), available_capacity() );
  buffer.insert( buffer.end(), make_move_iterator( data.begin() ), make_move_iterator( data.begin() + num ) );
  write_size += num;
}

void Writer::close()
{
  // Your code here.
  end_write = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  uint64_t num = capacity_ - buffer.size();
  return num;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return write_size;
}

bool Reader::is_finished() const
{
  // Your code here.
  return ( end_write && buffer.empty() ) ? true : false;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return read_size;
}

string Reader::peek() const
{
  // Your code here.
  return std::string( buffer.begin(), buffer.end() );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len > buffer.size() ) {
    set_error();
    return;
  }
  uint64_t tmp = len;
  while ( tmp-- ) {
    buffer.pop_front();
  }
  read_size += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.size();
}
