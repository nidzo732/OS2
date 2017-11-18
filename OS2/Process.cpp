#include "Process.h"
#include "KernelProcess.h"
#include "KernelSystem.h"
#include<mutex>
#include<iostream>
Process::Process(ProcessId pid)
{
}

Process::~Process()
{
}

ProcessId Process::getProcessId() const
{
	return pProcess->getProcessId();
}

Status Process::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags)
{
	std::lock_guard<std::mutex> _guard(pProcess->system->getMutex());
	return pProcess->createSegment(startAddress, segmentSize, flags);
}

Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, void * content)
{
	std::lock_guard<std::mutex> _guard(pProcess->system->getMutex());
	auto result= pProcess->loadSegment(startAddress, segmentSize, flags, content);
	return result;
}

Status Process::deleteSegment(VirtualAddress startAddress)
{
	std::lock_guard<std::mutex> _guard(pProcess->system->getMutex());
	return pProcess->deleteSegment(startAddress);
}

Status Process::pageFault(VirtualAddress address)
{
	std::lock_guard<std::mutex> _guard(pProcess->system->getMutex());
	return pProcess->pageFault(address);
}

PhysicalAddress Process::getPhysicalAddress(VirtualAddress address)
{
	std::lock_guard<std::mutex> _guard(pProcess->system->getMutex());
	return pProcess->getPhysicalAddress(address);
}
