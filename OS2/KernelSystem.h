#pragma once
#include<mutex>
class KernelSystem
{
	std::mutex guard;
public:
	KernelSystem();
	void createProcess();
	~KernelSystem();
	std::mutex& getMutex();
};

