#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class Timer
{
private:
  uint64_t initial_RTO_ms_; // 初始重传超时时间
  uint64_t current_RTO_ms;  // 当前重传超时时间
  uint64_t time_ms { 0 };   // 计时器时间
  bool running { false };   // 是否运行

public:
  explicit Timer( uint64_t init_ROT ) : initial_RTO_ms_( init_ROT ), current_RTO_ms( init_ROT ) {}

  void stop() { running = false; } // 停止计时器

  void start() // 开始计时器
  {
    time_ms = 0;
    running = true;
  }

  void tick( uint64_t ms_since_last_tick ) // 计时器滴答
  {
    if ( running ) {
      time_ms += ms_since_last_tick; // 更新计时器时间
    }
  }

  bool is_running() { return running; }

  bool is_expired() { return running && time_ms >= current_RTO_ms; } // 计时器是否过期

  void doubleRTO() { current_RTO_ms *= 2; } // 重传超时时间翻倍

  void resetRTO() { current_RTO_ms = initial_RTO_ms_; } // 重置重传超时时间
};

class TCPSender
{
  Wrap32 isn_;                    // 初始序列号
  uint64_t initial_RTO_ms_;       // 初始重传超时时间
  bool syn_ { false };            // 是否发送了SYN
  bool fin_ { false };            // 是否发送了FIN
  uint64_t retransmit_cnt_ { 0 }; // 超时重复发送的数量

  uint64_t acked_seqno { 0 };     // 已确认的序列号
  uint64_t next_seqno { 0 };      // 下一个要发送的序列号
  uint64_t window_size { 1 };     // 窗口大小
  uint64_t outstanding_cnt { 0 }; // 发送中的数量

  std::queue<TCPSenderMessage> outstanding_segments {}; // 发送中的队列 发送中未被确认的队列，还在“飞行”中的数据
  // Keep track of which segments have been sent but not yet acknowledged by the receiver
  std::queue<TCPSenderMessage> queued_segments {}; // 缓存队列 准备发送的队列，发送数据从这里获取

  Timer timer { initial_RTO_ms_ }; // 计时器

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible isn_ */
  TCPSender( uint64_t initial_RTO_ms_, std::optional<Wrap32> fixed_isn_ );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
