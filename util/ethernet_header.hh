#pragma once

#include "parser.hh"

#include <array>
#include <cstdint>
#include <string>

// Helper type for an Ethernet address (an array of six bytes)
using EthernetAddress = std::array<uint8_t, 6>;

// Ethernet broadcast address (ff:ff:ff:ff:ff:ff)
constexpr EthernetAddress ETHERNET_BROADCAST = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// Printable representation of an EthernetAddress
std::string to_string( EthernetAddress address );

// Ethernet frame header
struct EthernetHeader
{
  static constexpr size_t LENGTH = 14;         //!< Ethernet header length in bytes
  static constexpr uint16_t TYPE_IPv4 = 0x800; //!< Type number for [IPv4](\ref rfc::rfc791)
  static constexpr uint16_t TYPE_ARP = 0x806;  //!< Type number for [ARP](\ref rfc::rfc826)

  EthernetAddress dst; // 目的MAC地址
  EthernetAddress src; // 源MAC地址
  uint16_t type;       // 2字节，表示上层协议类型，如0x0800表示IPv4，0x0806表示ARP

  // Return a string containing a header in human-readable format
  std::string to_string() const; // 以字符串形式返回header内容

  void parse( Parser& parser );                   // 解析header内容
  void serialize( Serializer& serializer ) const; // 序列化header内容
};
