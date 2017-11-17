#pragma once
#include<mutex>
#include "vm_declarations.h"
#include "page_tables.h"
#include "part.h"
#include "PhysicalMemory.h"
#include "Swap.h"
class Process;
class KernelProcess;

class KernelSystem
{
	std::mutex guard;
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition);
	FreePage *getFreePage(bool onPagingTable=false, bool recurse=true);
	PageTable *fetchTable();
	PageDirectory * fetchDirectory();
	void freeTable(PageTable *table);
	void freePage(FreePage *page, bool onPagingTable=false);
	void attemptSwapping();
	~KernelSystem();
	Process* createProcess();
	Time periodicJob();
	Status access(ProcessId pid, VirtualAddress address, AccessType type);
	std::mutex& getMutex();
	friend class KernelProcess;
private:
	
private:
	PhysicalMemory processMemory;
	PhysicalMemory pmtMemory;
	Swap swap;
	ProcessId nextId = 0;
	KernelProcess *processes = nullptr;;
};

