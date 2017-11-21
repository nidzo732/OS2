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
	for (auto process : processes)
	{
		process.second->updateWsetSize();
		process.second->resetReferenceBits();
	}
	return 50000;
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
			page = getFreePage(false, false);
		}
		return page;
	}
}

PageTable * KernelSystem::fetchTable()
{
	PageTable *pt=(PageTable*)getFreePage(true);
	if (pt == nullptr) return nullptr;
	memset(pt, 0, sizeof(PageTable));
	return pt;
}
PageDirectory * KernelSystem::fetchDirectory()
{
	PageDirectory *pt = (PageDirectory*)getFreePage(true);
	if (pt == nullptr) return nullptr;
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		pt->tables[i] = nullptr;
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
	KernelProcess* targetProcess=nullptr;
	for (auto process : processes)
	{
		if (process.second->evictionRating() > maxEr && process.second->loadedPages.size()>0)
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

void KernelSystem::pagesAllocated(PageNum count)
{
	pagesUsed += count;
}

void KernelSystem::pagesFreed(PageNum count)
{
	pagesUsed -= count;
}

bool KernelSystem::beganSwapping()
{
	return pagesUsed > SWAPPINES*processMemory.size();
}

ProcessId KernelSystem::getNextPid()
{
	return nextId++;
}

Process * KernelSystem::clone(ProcessId pid)
{
	if (processes.count(pid) == 0) return nullptr;
	auto proc = processes[pid];
	auto newProc = proc->clone();
	if (newProc == nullptr) return nullptr;
	Process *process = new Process(newProc->pid);
	processes[newProc->pid] = newProc;
	process->pProcess = newProc;
	return process;
}


