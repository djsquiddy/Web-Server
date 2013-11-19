#pragma once
#ifndef _HANDLE_THREAD_H_
#define _HANDLE_THREAD_H_

#include "ThreadSafeQueue.h"
#include "Server.hpp"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

class HandleThread
{
public:
	HandleThread(ThreadSafeQueue<std::string>* log,
		ThreadSafeQueue<boost::shared_ptr<CS3100::Connection> >* con)
		: m_loggerQueue(log), m_connectionQueue(con) {}
		~HandleThread() { delete m_handleThread; }
	
		void start();

private:
	void run();
	void determineFileType(std::string&);
	ThreadSafeQueue<std::string>* m_loggerQueue;	
	ThreadSafeQueue<boost::shared_ptr<CS3100::Connection> >* m_connectionQueue;
	boost::thread * m_handleThread;
};

#endif