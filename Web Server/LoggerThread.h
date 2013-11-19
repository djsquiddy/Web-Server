#pragma once
#ifndef _LOGGER_THREAD_H_
#define _LOGGER_THREAD_H_

#include "ThreadSafeQueue.h"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class LoggerThread
{
public:
	LoggerThread(ThreadSafeQueue<std::string>* log)
		: m_loggerQueue(log){}
	~LoggerThread() { delete m_loggerThread; }
	void start();
private:
	void run();
	boost::thread * m_loggerThread;
	ThreadSafeQueue<std::string>* m_loggerQueue;
};

#endif