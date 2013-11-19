/*
 * Name: Dylan Jones
 * Program : Toy Web Server
 * Date : 2/11/2012 
*/
// Included headers
#include "ThreadSafeQueue.h"
#include "Server.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <exception>
#include <vector>
#include <fstream>
#include <cstring>
#include <string>
#include <ctime>

// Function prototypes
void handle(ThreadSafeQueue<boost::shared_ptr<CS3100::Connection>>* connectionQueue,
	ThreadSafeQueue<std::string>* loggingQueue);
void logging(ThreadSafeQueue<std::string>* loggingQueue);

// Function definitions
// assuming the command line argument is or somewhere along those lines --thread=20
int main(int argc, char** args)
{
	// Declaring the local variables
	int maxThreads;
	CS3100::Server server;
	std::vector<boost::thread> threadPool;
	ThreadSafeQueue<std::string> loggingQueue;
	ThreadSafeQueue<boost::shared_ptr<CS3100::Connection>> connectionQueue;

	// determine how many threads to use from the command line arguments
	try
	{
		std::string commandLineArgs = args[1];
		// if the command line argument contained an equal sign assuming they type in
		// --thread=(some finite number)
		if(std::string::npos != commandLineArgs.find('='))
			commandLineArgs = commandLineArgs.substr(commandLineArgs.find('=') + 1, commandLineArgs.length());
		maxThreads = boost::lexical_cast<int>(commandLineArgs);
	}
	catch(std::exception&)
	{
		maxThreads = 10;
	}
	// start the server
	server.start();

	threadPool.reserve(maxThreads);
	
	// start the logging thread
	boost::thread loggingThread(logging, &loggingQueue);
	
	// create all the threads for the handle function
	for(int i = 1; i <= maxThreads; i++)
		threadPool.push_back(boost::thread(handle, &connectionQueue, &loggingQueue));

	while(true)
	{
		auto connection = server.getConnection();
		connectionQueue.enqueue(connection);
	}

	server.stop();
	return 0;
}// End main function

/*
 * This function handles the incoming request and sends out the appropriate response
*/
void handle(ThreadSafeQueue<boost::shared_ptr<CS3100::Connection> >* connectionQueue,
	ThreadSafeQueue<std::string>* loggingQueue)
{
	// creating the local variables
	int fileLength = 0;
	std::string fileName;
	std::string fileType;
	std::string fileRequest;
	std::ifstream fileIn;
	CS3100::Connection::BufferType buffer;
		
	while(true)
	{
		// check to see if the connection queue is empty if it is dequeue
		// if not put the thread to sleep and try again
		auto connection = connectionQueue->dequeue();
		if(!connection)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
			continue;
		}

		// resets the values
		fileLength = 0;
		fileName = "";
		fileType = "";
		fileRequest = "";

		// getting the request from the server
		buffer.fill(0);
		connection.get()->receive(buffer);

		// getting the file name and the file extension from the request
		fileName = buffer.data();
		fileName = fileName.substr(fileName.find(' ') + 1, fileName.length());
		fileName = fileName.substr(0, fileName.find(' '));
		fileType = fileName.substr(fileName.find('.') + 1, fileName.length());

		// if a specfic file is not specified then use the /index.html file as the default
		if(fileName == "/")
			fileName = "index.html";
		else
			fileName = fileName.substr(fileName.find('/') + 1, fileName.length());

		// get the time and the date that the request came in
		{
			fileRequest = "File Requested: " + fileName + '\n';
			time_t currTime;
			time(&currTime);
			fileRequest += "Date: " + (std::string)ctime(&currTime);
		}

		
		// if index.html does not exits it creates a file of all the files in the curr
		// directory and sets the values to send that file to the connection
		if (fileName == "index.html")
		{
			boost::filesystem::ifstream TestFile;
			TestFile.open(fileName.c_str(), std::ios::in | std::ios::binary);
			if (!TestFile)
			{
				boost::filesystem::ofstream outFile;
				outFile.open("file_list.txt",std::ios::out | std::ios::binary);

				boost::filesystem::path Dir(boost::filesystem::current_path());
				boost::filesystem::directory_iterator end_iter;

				std::multimap<std::time_t, boost::filesystem::path> result_set;

				if ( boost::filesystem::exists(Dir) && boost::filesystem::is_directory(Dir))
				{
					for (boost::filesystem::directory_iterator dir_iter(Dir); dir_iter != end_iter; ++dir_iter)
					{
						//if (fs::is_regular_file(dir_iter->status()))
						{
							outFile<<dir_iter->path()<<"\n";
							//result_set.insert(result_set_t::value_type(fs::last_write_time(dir_iter->status()), *dir_iter));
						}
					}
					outFile.close();
				}
				fileName = "file_list.txt";
				fileType = "txt";
			}
			TestFile.close();
	  }

		// open the file
		fileIn.open(fileName.c_str(), std::ios::app|std::ios::in|std::ios::binary);

		// if the file exists
		if (fileIn.is_open())
		{
			// getting the size of the file
			fileIn.seekg(0, std::ios::end);
			fileLength = (int)fileIn.tellg();
			fileIn.seekg(0, std::ios::beg);

			// determining the file type
			if(fileType == "txt")
				fileType = "text/plain\r\n";
			else if(fileType == "html")
				fileType = "text/html\r\n";
			else if(fileType == "gif")
				fileType = "image/gif\r\n";
			else if(fileType == "jpeg")
				fileType = "image/jpeg\r\n";
			else if(fileType == "png")
				fileType = "image/png\r\n";

			// Sending the HTTP 200 OK response to the server
			memcpy(buffer.data(), "HTTP/1.0 200 OK\r\nContent-type: ", 31);
			fileRequest += "Request Status: 200 OK\r\n\r\n";
			connection.get()->send(buffer, 31);

			// sending the file extension to the server
			buffer.fill(0);
			memcpy(buffer.data(), fileType.c_str(), fileType.size());
			connection.get()->send(buffer, fileType.size());
		
			// adds the file length to the string
			buffer.fill(0);
			
			try
			{
				fileType = "Content-length: " + boost::lexical_cast<std::string>(fileLength) + "\r\n\r\n";
			}
			catch(std::exception&) 
			{
				fileType = "Content-length: 0\r\n\r\n";
			}

			// sending the file size to the server
			memcpy(buffer.data(), fileType.c_str(), fileType.size());
			connection.get()->send(buffer, fileType.size());

			// sending the file to the log file
			while(fileLength != 0)
			{
				buffer.fill(0);
				fileIn.read(buffer.data(), 1);
				connection.get()->send(buffer, 1);
				fileLength--;
			}
		}
		else // if the file does not exist
		{
			// if the file couldn't not be found or opened send the 
			// HTTP 404 NOT FOUND response
			buffer.fill(0);
			memcpy(buffer.data(), "HTTP/1.0 404 NOT FOUND\r\n\r\n", 24);
			fileRequest += "Request Status: 404 NOT FOUND\r\n\r\n";
			connection.get()->send(buffer, 17);
		}
		fileIn.close();

		loggingQueue->enqueue(fileRequest);
	}
}// End handle function
	
/*
 * This function logs all the requests that have come in
*/
void logging(ThreadSafeQueue<std::string>* loggingQueue)
{
	while(true)
	{
		auto request = loggingQueue->dequeue();
		if(request)
		{
			std::ofstream fileOut("log.txt", std::ios::app|std::ios::out|std::ios::binary);
			fileOut.write(request.get().c_str(), request.get().size());
			fileOut.close();
		}
		else
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		}
	}
}// End logging function