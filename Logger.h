#pragma once

#include <string>
#include <fstream>
#include <Windows.h>

class CLogger
{
public:
	CLogger();
	~CLogger(); 

	void Log(std::string text);

private:
	std::ofstream logFile;
};

