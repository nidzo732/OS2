#include "KernelSystem.h"
#include "vm_declarations.h"
#include "Process.h"
#include "KernelProcess.h"
#include<cstring>

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition * partition)
	:processMemory(processVMSpace, processVMSpaceSize), pmtMemory(pmtSpace, pmtSpaceSize), swap(partition)
{
}

Time KernelSystem::periodicJob()
{
	
	return Time();
}

Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	
	if (!processes.count(pid)) return TRAP;
	KernelProcess *proc = processes[pid];
	auto status= proc->access(address, type);
	proc->logResult(status, type);
	return status;
}

FreePage * KernelSystem::getFreePage(bool onPagingTable, bool recurse)
{
	FreePage *page=nullptr;
	if (onPagingTable)
	{
		if (pmtMemory.pagesAvailable())
		{
			page = pmtMemory.fetchFreePage();
		}
		return page;
	}
	else
	{
		if (processMemory.pagesAvailable())
		{
			page = processMemory.fetchFreePage();
		}
		else if (recurse)
		{
			attemptSwapping();
			page = getFreePage(false);
		}
		return page;
	}
}

PageTable * KernelSystem::fetchTable()
{
	PageTable *pt=(PageTable*)getFreePage(true);
	if (pt == nullptr) return nullptr;
	memset(pt, 0, sizeof(PageTable));
	for (int i = 0; i < TABLE_SIZE; i++)
	{
		pt->descriptors[i].next=pmtMemory.getFrame(nullptr);
	}
	return pt;
}
PageDirectory * KernelSystem::fetchDirectory()
{
	PageDirectory *pt = (PageDirectory*)getFreePage(true);
	if (pt == nullptr) return nullptr;
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		pt->tables[i] = pmtMemory.getFrame(nullptr);
	}
	return pt;
}

void KernelSystem::freeTable(PageTable * table)
{
	freePage((FreePage*)table, true);
}

void KernelSystem::freePage(FreePage * page, bool onPmtMemory)
{
	if (onPmtMemory)
	{
		pmtMemory.reclaimPage(page);
	}
	else
	{
		processMemory.reclaimPage(page);
	}
}

void KernelSystem::attemptSwapping()
{
	if (!swap.clustersAvailable()) return;
	float maxEr = -1;
	KernelProcess* targetProcess;
	for (auto process : processes)
	{
		if (process.second->evictionRating() > maxEr)
		{
			maxEr = process.second->evictionRating();
			targetProcess = process.second;
		}
	}
	if (targetProcess != nullptr) targetProcess->sacrificePage();
}


KernelSystem::~KernelSystem()
{
}

Process * KernelSystem::createProcess()
{
	
	auto pid = nextId++;
	Process *process = new Process(pid);
	KernelProcess *kProcess = new KernelProcess(pid, this);
	processes[pid]=kProcess;
	process->pProcess = kProcess;
	return process;
}

std::mutex & KernelSystem::getMutex()
{
	return guard;
}


