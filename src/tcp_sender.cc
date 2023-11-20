#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  // 有多少序列号在飞行（发送中）
  return outstanding_cnt;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  // 发生了多少次连续*重新*传输
  return retransmit_cnt_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  // 如果需要发送TCPSenderMessage，则发送（或者为空）
  if ( queued_segments.empty() )
    return {};               // 如果没有需要发送的TCPSenderMessage，则返回空
  if ( !timer.is_running() ) // 如果计时器没有运行
    timer.start();           // 启动计时器
  TCPSenderMessage msg = queued_segments.front();
  queued_segments.pop();
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // 从出站流中推送字节，构建发送队列
  uint64_t currwindow_size = max( window_size, (uint64_t)1 );
  while ( outstanding_cnt < currwindow_size ) {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap( next_seqno, isn_ );

    if ( !syn_ ) { // 如果没有发送SYN
      msg.SYN = syn_ = true;
      outstanding_cnt += 1;
    }

    uint64_t payload_size = min( TCPConfig::MAX_PAYLOAD_SIZE, (size_t)( currwindow_size - outstanding_cnt ) );

    read( outbound_stream, payload_size, msg.payload ); // 从outbounm_stream读取payload_size字节 到msg.payload

    outstanding_cnt += msg.payload.length();

    // 判断是否需要发送FIN
    if ( !fin_ && outbound_stream.is_finished() && outstanding_cnt < currwindow_size ) {
      fin_ = true;
      msg.FIN = true;
      outstanding_cnt += 1;
    }

    if ( msg.sequence_length() == 0 )
      break;

    queued_segments.push( msg );
    outstanding_segments.push( msg );
    next_seqno += msg.sequence_length();

    if ( msg.FIN || outbound_stream.bytes_buffered() == 0 ) // 如果发送了FIN或者outbound_stream没有数据了
      break;
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  // 生成一个空的TCPSenderMessage
  Wrap32 seqno = Wrap32::wrap( next_seqno, isn_ );
  return TCPSenderMessage { seqno, false, {}, false };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // 接收并处理来自对等方接收器的TCPReceiverMessage
  // 接受到的TCPReceiverMessage的确认号，处理已经发送的TCPSenderMessage
  window_size = msg.window_size; // 更新窗口大小

  if ( msg.ackno.has_value() ) { // 如果有确认号
    auto ackno = msg.ackno.value().unwrap(
      isn_, next_seqno ); // 获取确认号，表明之前的数据已经被确认，期望接收到第ackno个字节

    if ( ackno > next_seqno ) // 如果确认号大于下一个要发送的序列号，说明有错误
      return;

    acked_seqno = ackno; // 更新已确认的序列号

    while ( !outstanding_segments.empty() ) {
      auto front_msg = outstanding_segments.front();
      if ( front_msg.sequence_length() + front_msg.seqno.unwrap( isn_, next_seqno ) <= acked_seqno ) {
        // 说明当前的TCPSenderMessage已经被确认，可以从outstanding_segments中移除
        // acked_seqno 可以覆盖数据段的所有字节
        outstanding_segments.pop();
        outstanding_cnt -= front_msg.sequence_length(); // 更新在飞行中的数量

        timer.resetRTO();
        if ( !outstanding_segments.empty() ) {
          timer.start();
        }
        retransmit_cnt_ = 0;
      } else {
        // 说明当前的TCPSenderMessage还没有被确认，不能从outstanding_segments中移除
        break;
      }
    }

    if ( outstanding_segments.empty() )
      timer.stop();
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  // 自上次调用tick（）方法以来经过了多少毫秒
  timer.tick( ms_since_last_tick );

  if ( timer.is_expired() ) {                             // 超时
    queued_segments.push( outstanding_segments.front() ); // 重新发送
    if ( window_size != 0 ) {
      ++retransmit_cnt_;
      timer.doubleRTO();
    }

    timer.start(); // 重启计时器
  }
}
