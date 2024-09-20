#pragma once

#include "byte_stream.hh"
#include <map>
class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output )
    : output_( std::move( output ) ), buf_( {} ), flag_eof( false ), index_eof( 0 )
  {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;          // the Reassembler writes to this ByteStream
  std::map<size_t, char> buf_; // 收到的不靠谱字节流
  bool flag_eof;               // 结束标志
  size_t index_eof;            // 结尾的字节编号,用来确保所有字节都能被写入
public:
  size_t get_unread()
  { // 得到第一个未读取的字符下标
    return output_.reader().bytes_popped();
  }
  size_t get_unsort()
  { // 得到第一个没有排序的字符下标（也就是没有放进output_的第一个下标）
    return output_.writer().bytes_pushed();
  }
  size_t get_unaccept()
  { // 得到不能超出output_容量限制u的下标
    return get_unread() + output_.get_capacity_();
  }
};
