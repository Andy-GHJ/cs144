#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  string message1 = "GET " + path + " HTTP/1.1" + "\r\n"; // 注意空格
  string message2 = "Host: " + host + "\r\n";
  string message3 = "Connection: close\r\n";
  string message4 = "\r\n";
  // 注：这些消息格式是http协议的规范，这里不展开
  string buf;
  // 建立tcp连接
  TCPSocket tcpSocket;                          // 创建客户端，注意不要用new
  tcpSocket.connect( Address( host, "http" ) ); // 连接服务端
  // 向字节流写入
  tcpSocket.write( message1 + message2 + message3 + message4 );
  tcpSocket.shutdown( SHUT_WR ); // 断开写入流（关闭写入接口），并不会关闭套接字
  // 从字节流循环读取
  while ( !tcpSocket.eof() ) // 没到结尾
  {
    tcpSocket.read( buf ); // 将字节流读进buf
    cout << buf;
  }
  // shutdown() 会等待输入缓冲区中的数据传输完成后再关闭连接
  // close() 无论输入缓冲区中是否有数据都将立即关闭套接字
  tcpSocket.close();
  // return;
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
