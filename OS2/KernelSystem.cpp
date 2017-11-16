#include "KernelSystem.h"
#include "vm_declarations.h"

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition * partition)
	:processMemory(processVMSpace, processVMSpaceSize), pmtMemory(pmtSpace, pmtSpaceSize), swap(partition)
{
}

void KernelSystem::createProcess()
{
	std::lock_guard<std::mutex> _guard(guard);
}

FreePage * KernelSystem::getFreePage(bool recurse)
{
	FreePage *page;
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

void KernelSystem::freePage(FreePage * page)
{
	processMemory.reclaimPage(page);
}

void KernelSystem::attemptSwapping()
{
}


KernelSystem::~KernelSystem()
{
}

std::mutex & KernelSystem::getMutex()
{
	return guard;
}


