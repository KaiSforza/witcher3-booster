#include "Logger.h"
#include <time.h>
#include <consoleapi.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

	
CLogger::CLogger()
{
	logFile.open("log.txt");

	// Create console
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
}

CLogger::~CLogger()
{
	logFile.close();
	FreeConsole();
}

void CLogger::Log(std::string test)
{
	time_t t = time(0);   // get time now
	struct tm * now = localtime(&t);
	std::string payload = "[" + std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min) + ":" + std::to_string(now->tm_sec) + "] " + test + "\n";
	printf(payload.c_str());
	logFile << payload << std::flush;
}
