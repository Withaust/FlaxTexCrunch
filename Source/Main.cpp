
#include <string>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <chrono>

#include "Processor.h"

int main(int argc, char** argv)
{
	if (!ConstData::HandleArgs(argc, argv))
	{
		return 1;
	}

	Processor::LookForJobs();
	Processor::ProcessJobs();

	return 0;
}
