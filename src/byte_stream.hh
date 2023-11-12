#pragma once

#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
protected:
  uint64_t capacity_;
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  std::string queue_ {};
  uint64_t pushed_len_ { 0 }; // 入
  uint64_t popped_len_ { 0 }; // 出
  bool closed_ { false };     // 关闭
  bool error_ { false };      // 错误

public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.

  void close();     // Signal that the stream has reached its ending. Nothing more will be written.
  void set_error(); // Signal that the stream suffered an error.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now? 当前可用容量
  uint64_t bytes_pushed() const; // Total number of bytes cumulatively pushed to the stream 总共有多少字节被推到流中
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer
  void pop( uint64_t len );      // Remove `len` bytes from the buffer 从缓冲区中删除len字节

  bool is_finished() const; // Is the stream finished (closed and fully popped)?
  bool has_error() const;   // Has the stream had an error?

  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t len, std::string& out );
