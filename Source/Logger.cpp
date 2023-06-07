#include "Logger.h"

std::mutex Logger::Mutex;

void Logger::Print(const std::string& Text)
{
	Mutex.lock();
	std::cout << Text << std::endl;
	std::cout.flush();
	Mutex.unlock();
}

void Logger::Info(const std::string& Text)
{
	Mutex.lock();
	std::cout << "[INFO] " << Text << std::endl;
	std::cout.flush();
	Mutex.unlock();
}

void Logger::Error(const std::string& Text)
{
	Mutex.lock();
	std::cout << std::endl;
	std::cout << "[ERROR] " << Text << std::endl;
	std::cout << std::endl;
	std::cout.flush();
	Mutex.unlock();
}
