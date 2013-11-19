#include "LoggerThread.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include <string>

void LoggerThread::start()
{
	m_loggerThread = new boost::thread(boost::bind(&LoggerThread::run, this));
}

/*
 * This function logs all the requests that have come in
*/
void LoggerThread::run()
{
	while(true)
	{
		if(!m_loggerQueue->isEmpty())
		{
			std::ofstream fileOut("log.txt", std::ios::app|std::ios::out|std::ios::binary);
			std::string request = m_loggerQueue->dequeue();
			fileOut.write(request.c_str(), request.size());
			fileOut.close();
		}
		else
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		}
	}
}