#include "System.h"
#include "KernelSystem.h"
#include<mutex>

System::System(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition * partition)
{
	pSystem = new KernelSystem(processVMSpace, processVMSpaceSize, pmtSpace, pmtSpaceSize, partition);
}

System::~System()
{
	delete pSystem;
}

Process * System::createProcess()
{
	std::lock_guard<std::mutex> _guard(pSystem->getMutex());
	return pSystem->createProcess();
}

Time System::periodicJob()
{
	std::lock_guard<std::mutex> _guard(pSystem->getMutex());
	return pSystem->periodicJob();
}

Status System::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	std::lock_guard<std::mutex> _guard(pSystem->getMutex());
	return pSystem->access(pid, address, type);
}
