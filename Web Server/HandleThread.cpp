#include "HandleThread.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <string>
#include <cstring>
void HandleThread::start()
{
	m_handleThread = new boost::thread(boost::bind(&HandleThread::run, this));
}

/*
 * This function handles the incoming request and sends out the approprate responds
*/
void HandleThread::run()
{
	// creating the local variables
	//const int MAX_SIZE = 25;
	int fileLength = 0;
	std::string fileName;
	std::string fileType;
	std::ifstream fileIn;
	CS3100::Connection::BufferType buffer;
	boost::shared_ptr<CS3100::Connection> connection;
	std::string fileRequest;
	
	while(true)
	{
		// check to see if the connection queue is empty if it is dequeue
		// if not but the thread to sleep and try again
		if(!m_connectionQueue->isEmpty())
			connection = m_connectionQueue->dequeue();
		else
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
			continue;
		}
	
		// getting the request from the server
		buffer.fill(0);
		connection->receive(buffer);
	
		// getting the file name and the file extension from the request
		fileName = buffer.data();
		fileName = fileName.substr(fileName.find(' ') + 1, fileName.length());
		fileName = fileName.substr(0, fileName.find(' '));
		fileType = fileName.substr(fileName.find('.') + 1, fileName.length());

		if(fileName == "/")
			fileName = "index.html";
		else
			fileName = fileName.substr(1, fileName.length());

		{	// get the time and the date that the request came in
			fileRequest = "File Requested: " + fileName + '\n';
			time_t currTime;
			time(&currTime);
			fileRequest += "Date: " + boost::lexical_cast<std::string>(ctime(&currTime));
		}

		// open the file
		fileIn.open(fileName.c_str(), std::ios::app|std::ios::in|std::ios::binary);

		// if the file exist
		if (fileIn.is_open())
		{
			// getting the size of the file
			fileIn.seekg(0, std::ios::end);
			fileLength = (int)fileIn.tellg();
			fileIn.seekg(0, std::ios::beg);

			// determining the file type
			determineFileType(fileType);

			// Sending the HTTP 200 OK response to the server
			memcpy(buffer.data(), "HTTP\\1.0 200 OK\r\nContent-type: ", 31);
			fileRequest += "Request Status: 200 OK\r\n\r\n";
			connection->send(buffer, 31);

			// sending the file extension to the server
			buffer.fill(0);
			memcpy(buffer.data(), fileType.c_str(), fileType.size());
			connection->send(buffer, fileType.size());
		
			// adds the file length to the string
			buffer.fill(0);
			try
			{
				fileType = "Content-length: " + boost::lexical_cast<std::string>(fileLength) + "\r\n\r\n";
			}
			catch(std::exception&){}
			
			// sending the file size to the server
			memcpy(buffer.data(), fileType.c_str(), fileType.size());
			connection->send(buffer, fileType.size());

			// sending the file to the log file
			while(fileLength != 0)
			{
				buffer.fill(0);
				fileIn.read(buffer.data(), 1);
				connection->send(buffer, 1);
				fileLength--;
			}

			//memcpy(buffer.data(), "\r\n\r\n", 4);
			//connection->send(buffer, 4);
		}
		else // if the file does not exist
		{
			// if the file couldn't not be found or opened send the 
			// HTTP 404 NOT FOUND response
			buffer.fill(0);
			memcpy(buffer.data(), "HTTP\\1.0 404 NOT FOUND\r\n\r\n", 24);
			fileRequest += "Request Status: 404 NOT FOUND\r\n\r\n";
			connection->send(buffer, 17);
		}
		fileIn.close();

		m_loggerQueue->enqueue(fileRequest);
	}
}

void HandleThread::determineFileType(std::string& fileType)
{
	if(fileType == "txt")
		fileType = "text/text\r\n";
	else if(fileType == "html")
		fileType = "text/html\r\n";
	else if(fileType == "png")
		fileType = "image/png\r\n";
	else if(fileType == "jpeg")
		fileType = "image/jpeg\r\n";
}