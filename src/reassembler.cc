#include "reassembler.hh"
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if ( data.empty() ) {
    if ( is_last_substring ) {
      output.close();
    }
    return;
  }

  if ( output.available_capacity() == 0 ) {
    return;
  }

  auto const end_index = first_index + data.size();                                      // data 的末尾
  auto const first_unacceptable = first_unassembled_index + output.available_capacity(); // output 的末尾

  // data is not in [first_unassembled_index, first_unacceptable)
  if ( end_index <= first_unassembled_index || first_index >= first_unacceptable ) // 数据太旧或太新都丢弃
  {
    return;
  }

  // if part of data is out of capacity, then truncate it
  if ( end_index > first_unacceptable ) // 数据超过可接受范围, 截断
  {
    data = data.substr( 0, first_unacceptable - first_index );
    // if truncated, it won't be last_substring
    is_last_substring = false;
  }

  // unordered bytes, save it in buffer and return
  if ( first_index > first_unassembled_index ) // 红色部分，虽然不能直接输出，但是可以保存在 buffer 中
  {
    insert_into_buffer( first_index, std::move( data ), is_last_substring );
    return;
  }

  // remove useless prefix of data (i.e. bytes which are already assembled)
  if ( first_index < first_unassembled_index ) // 数据左边重复出现截断
  {
    data = data.substr( first_unassembled_index - first_index );
  }

  // here we have first_index == first_unassembled_index
  first_unassembled_index += data.size();
  output.push( std::move( data ) );

  if ( is_last_substring ) {
    output.close();
  }

  if ( !buffer.empty() && buffer.begin()->first <= first_unassembled_index ) { // buffer 中有数据可以输出
    pop_from_buffer( output );
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return buffer_size;
}

void Reassembler::insert_into_buffer( const uint64_t first_index, std::string&& data, const bool is_last_substring )
{
  auto begin_index = first_index;
  const auto end_index = first_index + data.size();

  for ( auto it = buffer.begin(); it != buffer.end() && begin_index < end_index; ) {
    if ( begin_index >= it->first ) {
      begin_index = max(
        begin_index, it->first + it->second.size() ); // it->first + it->second.size() 是当前缓冲区一个数据片的结尾
      ++it; // 跳到下一个数据片，因为从begin_index开始到it->first + it->second.size()的数据已经全部在缓冲区了
      continue;
    }

    if ( begin_index == first_index && end_index <= it->first ) { // data 在 it 之前，直接插入就结束
      buffer_size += data.size();
      buffer.emplace(
        it, first_index, std::move( data ) ); // 在 it 之前插入一个数据片, data用完之后可以废弃，所以用move
      return;
    }

    // 下面的情况data可能横跨多个数据片
    const auto right_index = min( it->first, end_index );
    const auto len = right_index - begin_index;
    buffer.emplace(
      it,
      begin_index,
      data.substr( begin_index - first_index,
                   len ) ); // 插入一个数据片，（直接于 pos 前插入元素到容器中）操作之后，it 仍然指向原来的元素
    buffer_size += len;
    begin_index = right_index;
  }

  // 缓冲区访问完了，但是data还有剩余
  if ( begin_index < end_index ) {
    buffer_size += end_index - begin_index;
    buffer.emplace_back( begin_index, data.substr( begin_index - first_index ) ); // 在缓冲区末尾插入一个数据片
  }

  if ( is_last_substring ) {
    has_last = true;
  }
}

void Reassembler::pop_from_buffer( Writer& output )
{
  for ( auto it = buffer.begin(); it != buffer.end(); ) {
    if ( it->first > first_unassembled_index ) { // 这个数据片还不能输出
      break;
    }
    // it->first <= first_unassembled_index
    const auto end = it->first + it->second.size();
    if ( end <= first_unassembled_index ) { // 这个数据片已经输出过了
      buffer_size -= it->second.size();
    } else { // 这个数据片部分输出过了
      auto data = std::move( it->second );
      buffer_size -= data.size();
      if ( it->first < first_unassembled_index ) {
        data = data.substr( first_unassembled_index - it->first );
      }
      first_unassembled_index += data.size();
      output.push( std::move( data ) );
    }
    it = buffer.erase( it ); // 删除已经输出的数据片, 返回下一个数据片的迭代器
  }

  if ( buffer.empty() && has_last ) {
    output.close();
  }
}