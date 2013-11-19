#pragma once
#ifndef _THREAD_SAFE_QUEUE_H_
#define _THREAD_SAFE_QUEUE_H_

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/optional.hpp>
#include <list>

template <typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue(void) : m_queue(), m_mutex() {}
	
	
	// this addes an item to the queue
	inline void enqueue(T item)
	{
		boost::unique_lock<boost::mutex> lock(m_mutex);
		m_queue.push_back(item);
	}

	// this removes an item from the queue if the
	// queue is empty it returns null
	inline boost::optional<T> dequeue()
	{
		boost::optional<T> temp = NULL;
		boost::unique_lock<boost::mutex> lock(m_mutex);
		if(!m_queue.empty())
		{
			temp = m_queue.front();
			m_queue.pop_front();
		}
		return temp;
	}

private:
	std::list<T> m_queue;
	mutable	boost::mutex m_mutex;
};

#endif