#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  if ( error_ || is_closed() || data.empty() ) {
    return;
  }
  const size_t len = min( data.length(), available_capacity() );
  queue_ += data.substr( 0, len ); // 往 queue 里写入
  pushed_len_ += len;              // 已经写入多少个字节
}

void Writer::close()
{
  closed_ = true;
  // Your code here.
}

void Writer::set_error()
{
  error_ = true;
  // Your code here.
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_ == true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - pushed_len_ + popped_len_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushed_len_;
}

string_view Reader::peek() const
{
  // Your code here.
  return queue_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && queue_.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( queue_.empty() ) {
    return;
  }
  len = std::min( len, queue_.size() );
  queue_.erase( queue_.begin(), queue_.begin() + len ); // 模拟读出，其实是擦除
  popped_len_ += len;                                   // 已经读出多少字节
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return pushed_len_ - popped_len_; // 还有多少未读出
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return popped_len_;
}
