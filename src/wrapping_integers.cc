#include "wrapping_integers.hh"
#include <math.h>
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32( zero_point + static_cast<uint32_t>( n ) );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  constexpr uint64_t pow2_31 = ( 1UL << 31 );
  constexpr uint64_t pow2_32 = ( 1UL << 32 );
  uint64_t dis
    = raw_value_
      - static_cast<uint32_t>(
        checkpoint + zero_point.raw_value_ ); // 现在的32位序列号 与 绝对序列checkpoint转化到32位的序列号 的差值
  if ( dis < pow2_31 || checkpoint + dis < pow2_32 )
    return checkpoint + dis;
  return checkpoint + dis - pow2_32;
}
