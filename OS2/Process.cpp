#include "Process.h"
#include "KernelProcess.h"
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
	return pProcess->createSegment(startAddress, segmentSize, flags);
}

Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, void * content)
{
	return pProcess->loadSegment(startAddress, segmentSize, flags, content);
}

Status Process::deleteSegment(VirtualAddress startAddress)
{
	return pProcess->deleteSegment(startAddress);
}

Status Process::pageFault(VirtualAddress address)
{
	return pProcess->pageFault(address);
}

PhysicalAddress Process::getPhysicalAddress(VirtualAddress address)
{
	return pProcess->getPhysicalAddress(address);
}
