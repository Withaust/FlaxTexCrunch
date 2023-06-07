#pragma once

#include <iostream>
#include <string>
#include <mutex>

class Logger
{
private:

	static std::mutex Mutex;

public:

	static void Print(const std::string& Text);
	static void Info(const std::string& Text);
	static void Error(const std::string& Text);
};
