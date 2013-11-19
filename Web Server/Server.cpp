#include "Server.hpp"
#include <string>
#include <cstring>
#include <fstream>


#include <boost/thread.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

/** Connection Implementation
 */
namespace CS3100
{
class ConnectionImpl: public boost::enable_shared_from_this<ConnectionImpl>
{
private:
  boost::asio::io_service& io_service;
  boost::asio::ip::tcp::socket socket;
public:
  ConnectionImpl(boost::asio::io_service& io_service_)
  :io_service(io_service_)
  ,socket(io_service)
  {    
  }
  
  ~ConnectionImpl()
  {
  }
  
  void send(const Connection::BufferType & data,size_t length)
  {
	boost::asio::write
	(
	  socket
	  ,boost::asio::buffer
	  (
	data.data()
	,length
	  )
	);
  }
  size_t receive(Connection::BufferType & data)
  {
	boost::system::error_code error;
	size_t length = socket.read_some
	(
	  boost::asio::buffer
	  (
	data.data()
	,data.size()
	  )
	  ,error
	);
	if(!error) return length;
	if(error == boost::asio::error::eof)
	  return length;
	else if(error)
	  throw boost::system::system_error(error);
  }
  boost::asio::ip::tcp::socket& getSocket()
  {return socket;}
};

}

CS3100::Connection::Connection():
pImpl()
{}

CS3100::Connection::Connection(boost::shared_ptr<ConnectionImpl> impl):
pImpl(impl)
{}
	
void CS3100::Connection::send(const std::array<char,ARRAY_SIZE>& data,size_t length)
{
  return pImpl->send(data,length);
}

size_t CS3100::Connection::receive(std::array<char,ARRAY_SIZE>& data)
{
  return pImpl->receive(data);
}


/** Server Implementation
 *  
 * Currently empty, but will contain networking code
 */
class CS3100::ServerImpl
{
public:
  ServerImpl(uint16_t port);
  void start();
  void stop();
  boost::optional<boost::shared_ptr<CS3100::Connection> > getConnection();
private:
  void run();
  void startAccept();
  void onAccept(boost::shared_ptr<CS3100::ConnectionImpl> connection, const boost::system::error_code& error);
  static boost::asio::io_service ms_io_service;
  boost::thread m_thread;
  boost::asio::ip::tcp::acceptor acceptor;
  boost::mutex connectionListMutex;
  std::list<boost::shared_ptr<CS3100::Connection> > connectionList;
};

boost::asio::io_service CS3100::ServerImpl::ms_io_service;

CS3100::ServerImpl::ServerImpl(uint16_t port)
: m_thread()
, acceptor(ms_io_service,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port))
, connectionListMutex()
, connectionList()
{}


void CS3100::ServerImpl::startAccept()
{
  boost::shared_ptr<CS3100::ConnectionImpl> connection(new CS3100::ConnectionImpl(ms_io_service));
  acceptor.async_accept
  (
	connection->getSocket()
	,boost::bind
	(
	  &CS3100::ServerImpl::onAccept
	  ,this
	  ,connection
	  ,boost::asio::placeholders::error
	)
  );
}

void CS3100::ServerImpl::onAccept(boost::shared_ptr<CS3100::ConnectionImpl> connection, const boost::system::error_code& error)
{
  if(!error)
  {
	boost::interprocess::scoped_lock<boost::mutex> lock(connectionListMutex);
	connectionList.push_back(boost::shared_ptr<CS3100::Connection>(new CS3100::Connection(connection)));
  }
  startAccept();
}

void CS3100::ServerImpl::start()
{
  startAccept();
  //start thread to run io service loop
  m_thread = boost::thread(boost::bind(&CS3100::ServerImpl::run,this));
}

void CS3100::ServerImpl::run()
{
  ms_io_service.run();
}

void CS3100::ServerImpl::stop()
{
  ms_io_service.stop();
}

boost::optional< boost::shared_ptr<CS3100::Connection> > CS3100::ServerImpl::getConnection()
{
  boost::interprocess::scoped_lock <boost::mutex> lock(connectionListMutex);
  if(connectionList.empty()) return boost::optional< boost::shared_ptr<CS3100::Connection> >();
  auto connection = connectionList.front();
  connectionList.pop_front();
  return boost::optional< boost::shared_ptr<CS3100::Connection> >(connection);
}

CS3100::Server::Server(uint16_t port):
pImpl(new CS3100::ServerImpl(port))
{
}

boost::shared_ptr<CS3100::Connection> CS3100::Server::getConnection()
{
  while(true)
  {
	auto connection = pImpl->getConnection();
	if(connection) return *connection;
	boost::this_thread::sleep(boost::posix_time::milliseconds(500));
  }
}

void CS3100::Server::start()
{
  pImpl->start();
}
void CS3100::Server::stop()
{
  pImpl->stop();
}


