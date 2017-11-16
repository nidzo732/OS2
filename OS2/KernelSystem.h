#pragma once
#include<mutex>
#include "vm_declarations.h"
#include "page_tables.h"
#include "part.h"
#include "PhysicalMemory.h"

class KernelSystem
{
	std::mutex guard;
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition);
	void createProcess();
	FreePage *getFreePage(bool recurse=true);
	void freePage(FreePage *page);
	void attemptSwapping();
	~KernelSystem();
	std::mutex& getMutex();
private:
	
private:
	PhysicalMemory processMemory;
	PhysicalMemory pmtMemory;
	Partition *swap;
};

