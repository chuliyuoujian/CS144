#include "reassembler.hh"

using namespace std;
Reassembler::Reassembler( const size_t capacity )
  : capacity_( capacity ), output_( capacity_ ), buf( {} ), flag_eof( false ) /*,index_eof(0)*/
{}
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if ( is_last_substring ) {
    flag_eof = true;
    // index_eof=first_index+data.size();
  }
  // 把不是重复的数据放进buf_里
  for ( size_t i = 0; i < (int)data.size(); ++i ) {
    if ( index + i >= get_unsort() ) {
      buf_[index + i] = data[i];
    }
  }
  string str = "";
  for ( auto& to : buf_ ) {
    size_t id = to.first;
    char ch = to.second;
    if ( id > get_unsort() || id >= get_unaccept() )
      break;
    if ( id < get_unsort() ) {
      continue;
    }
    if ( id == get_unsort() ) {
      str += ch;
    }
  }
  output_.push( str );
  if ( flag_eof /*&&get_unsort()==index_eof*/ )
    output_.close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return buf_.size() - output_.bytes_pushed();
}
