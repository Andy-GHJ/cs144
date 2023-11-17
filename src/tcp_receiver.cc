#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) { // 表示这是一个SYN包，说明在建立连接
    isn = message.seqno;
    rcv_syn = true;
  }
  if ( !rcv_syn ) // 没有收到SYN包但是，SYN之后的包到达，说明还没有建立连接，不接收数据，直接返回
    return;
  uint64_t abs_seqno = message.seqno.unwrap(
    isn, inbound_stream.bytes_pushed() ); // 转换成绝对序列号, 离已经传输的字节数最近的绝对序列号
  if (
    abs_seqno == 0
    && !message
          .SYN ) // 如果绝对序列号为0，说明是第一个包，但是不是SYN包，说明不是建立连接的第一个包，不接收数据，直接返回
    return;
  uint64_t first_index = abs_seqno > 0
                           ? abs_seqno - 1
                           : 0; // 第一个字节的序列号，如果是SYN包，那么第一个字节的序列号是0，否则是abs_seqno-1
  reassembler.insert( first_index, message.payload, message.FIN, inbound_stream ); // 插入数据
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  uint16_t window_size = static_cast<uint16_t>( std::min(
    inbound_stream.available_capacity(), uint64_t( UINT16_MAX ) ) ); // 因为window_size是16位的，所以要转换成16位
  std::optional<Wrap32> ackno = Wrap32::wrap( inbound_stream.bytes_pushed() + rcv_syn + inbound_stream.is_closed(),
                                              isn ); // ackno是相对序列号，表明下一个期望接收的序列号，之前都收到了
  return TCPReceiverMessage( { rcv_syn ? ackno : std::nullopt, window_size } );
}
