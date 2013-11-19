#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <boost/shared_ptr.hpp>
#include <array>
 
namespace CS3100
{
  class ConnectionImpl;
 
  class Connection
  {
  public:
    static const size_t ARRAY_SIZE = 100;
    typedef std::array<char,ARRAY_SIZE> BufferType;
    Connection();
    Connection(boost::shared_ptr<ConnectionImpl>);
    void send(const BufferType&,size_t length);
    size_t receive(BufferType&);
  private:
    boost::shared_ptr<ConnectionImpl> pImpl;
  };
 
  class ServerImpl;
 
  class Server
  {
  public:
    Server(unsigned short port = 8080);
    boost::shared_ptr<Connection> getConnection();
    void start();
    void stop();
  private:
    boost::shared_ptr<ServerImpl> pImpl;
  };
}
 
#endif