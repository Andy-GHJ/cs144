#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , ip_to_mac_()
  , arp_timer_()
  , waited_dagrams_()
  , out_frames_()
  , IP_MAP_TTL( 30000 )
  , ARP_TTL( 5000 )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // 如果destination Ethertnet address已知，直接发送
  // 如果destination Ethertnet address未知，发送ARP request，将dgram入队列，等待ARP reply
  uint32_t target_ip = next_hop.ipv4_numeric();
  if ( ip_to_mac_.count( target_ip ) ) { // 在ARP映射表内，直接发送，推入待发送队列out_frames_
    // 构建以太网帧
    EthernetFrame ethe_frame;
    ethe_frame.header.type = EthernetHeader::TYPE_IPv4;
    // 1.设置包装后的以太网帧的源地址和目的地址
    ethe_frame.header.src = ethernet_address_;
    ethe_frame.header.dst = ip_to_mac_[next_hop.ipv4_numeric()].first;
    // 2.ip数据报序列化后放在以太网帧的payload
    ethe_frame.payload = serialize( dgram );
    // 3.推进待发送队列
    out_frames_.push( ethe_frame );
  } else {                                  // 不在ARP映射表内，不知道对方MAC地址，发送ARP request
    if ( !arp_timer_.count( target_ip ) ) { // 这个ip地址没有发送过ARP request（也是一个以太网帧）
      // 发生ARP request
      ARPMessage arpmsg;
      arpmsg.opcode = ARPMessage::OPCODE_REQUEST;
      arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
      arpmsg.sender_ethernet_address = ethernet_address_;
      arpmsg.target_ip_address = next_hop.ipv4_numeric();

      EthernetFrame eframe { { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP },
                             serialize( arpmsg ) };
      // 发送ARP请求，目的MAC地址是BROADCAST
      out_frames_.push( eframe );
      arp_timer_.emplace( next_hop.ipv4_numeric(), 0 );
      waited_dagrams_.insert( { target_ip, { dgram } } );
    } else {
      waited_dagrams_[target_ip].push_back( dgram );
    }
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // 处理接受的帧
  // 如果目的MAC不是自己又不是广播地址，直接返回
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST )
    return {};

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    // 收到的帧是IPV4类型
    InternetDatagram data;
    if ( parse( data, frame.payload ) ) // 解析
      return data;
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arpmsg;
    if ( parse( arpmsg, frame.payload ) ) { // 如果解析没有错误
      // 更新自己的ARP表
      ip_to_mac_.insert( { arpmsg.sender_ip_address, { arpmsg.sender_ethernet_address, 0 } } );
      if ( arpmsg.opcode == ARPMessage::OPCODE_REPLY ) {
        // 这个是应答，是自己发送ARP请求，别人回答，这时候可以把自己的waited_dagrams_内的相应的数据报发送出去
        vector<InternetDatagram> datas = waited_dagrams_[arpmsg.sender_ip_address];
        for ( InternetDatagram data : datas ) {
          send_datagram( data, Address::from_ipv4_numeric( arpmsg.sender_ip_address ) );
        }
        waited_dagrams_.erase( arpmsg.sender_ip_address ); // 发送完了，删除
      } else if ( arpmsg.opcode == ARPMessage::OPCODE_REQUEST ) {
        // ARP请求
        if ( arpmsg.target_ip_address == ip_address_.ipv4_numeric() ) {
          // 目标地址和自己的地址一样，则构建ARPMessage应答
          ARPMessage reply_msg;
          reply_msg.opcode = ARPMessage::OPCODE_REPLY;
          reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
          reply_msg.sender_ethernet_address = ethernet_address_;
          reply_msg.target_ip_address = arpmsg.sender_ip_address;
          reply_msg.target_ethernet_address = arpmsg.sender_ethernet_address;

          EthernetFrame reply_frame {
            { arpmsg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP },
            serialize( reply_msg ) };
          out_frames_.push( reply_frame );
        }
      }
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // 保证ip_to_mac_里面的总是新的，所以在发送的时候就不需要单独考虑有没有过时
  for ( auto it = ip_to_mac_.begin(); it != ip_to_mac_.end(); ) {
    it->second.second += ms_since_last_tick;
    if ( it->second.second >= IP_MAP_TTL ) {
      it = ip_to_mac_.erase( it );
    } else {
      ++it;
    }
  }
  // 保证arp_timer_里面的总是最近发送的，所以在发送arp请求的时候如果没有，则发送，如果有，则#64
  for ( auto it = arp_timer_.begin(); it != arp_timer_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second >= ARP_TTL ) {
      it = arp_timer_.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( out_frames_.empty() ) {
    return {};
  }
  auto frame = out_frames_.front();
  out_frames_.pop();
  return frame;
}
