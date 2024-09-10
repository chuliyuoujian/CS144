#include "reassembler.hh"
#include<sstream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if ( is_last_substring ) {
    flag_eof = true;
     index_eof=first_index+data.size();
  }
  size_t first_unsort=get_unsort();
  size_t first_unaccept=get_unaccept();
  for(size_t i=0;i<(size_t)data.size();++i) {
	  if(first_index+i>=first_unsort&&first_index+i<first_unaccept) {
		  buf_[first_index+i]=data[i];
	  }
  }
  string str = "";
  ostringstream str_stream;
  size_t now=get_unsort();//已经插入的字符（由于我们是在最后一起插入一串，也就是相当于bytes_pushed是一次性加入多个字符，所以在插入过程中用now动态记录已经插入output_的字符）
  for ( auto& to : buf_ ) {
    size_t id = to.first;
    char ch = to.second;
   if ( id > now || now >= get_unaccept() )
      break;
    if ( id < now) {
      continue;
    }
    if ( id == now) {
      str_stream<<ch;
	  ++now;
    }
  }
  str=str_stream.str();
  output_.writer().push(str);
  if ( flag_eof&&get_unsort()==index_eof)
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return buf_.size() - output_.writer().bytes_pushed();
}
