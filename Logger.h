#pragma once
#include <Windows.h>
#include <string>
#include<format>
#include<filesystem>
#include<fstream>
#include<chrono>

void Log(std::ostream& os, const std::string& message);

void Log(const std::string& message);

std::wstring ConvertString(const std::string& str);

std::string ConvertString(const std::wstring& wstr);

void InitializeLogger();

void createLogFile();
