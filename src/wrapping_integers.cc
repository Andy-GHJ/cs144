#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) // 绝对序列号n，起始序列号zero_point，转相对序列号
{
  // Your code here.
  n = uint32_t( n );
  return zero_point + n; // zero_point 相当于SYN包的序列号
}

uint64_t Wrap32::unwrap( Wrap32 zero_point,
                         uint64_t checkpoint ) // zero_point是相当于SYN包的序列号
  const // 相对转绝对序列号,32位转64位，是一对多的关系，要求返回最接近checkpoint的绝对序列号
{
  uint32_t diff = raw_value_ - zero_point.raw_value_;
  uint32_t ckp_mod = static_cast<uint32_t>( checkpoint );
  uint32_t add = diff - ckp_mod, subtract = ckp_mod - diff;
  uint64_t added = checkpoint + add, subtracted = checkpoint - subtract;
  if ( subtracted > checkpoint || add <= subtract )
    return added;
  return subtracted;
}
