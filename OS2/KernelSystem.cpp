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
	std::lock_guard<std::mutex> _guard(guard);
	return Time();
}

Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	std::lock_guard<std::mutex> _guard(guard);
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
}


KernelSystem::~KernelSystem()
{
}

Process * KernelSystem::createProcess()
{
	std::lock_guard<std::mutex> _guard(guard);
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


