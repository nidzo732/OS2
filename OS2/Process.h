#pragma once
#include "vm_declarations.h"
class KernelProcess;
class System;
class KernelSystem;
class Process {
public:
	Process(ProcessId pid);
	~Process();
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	Process* clone(ProcessId pid);
	Status createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessRight flags);
	Status disconnectSharedSegment(const char* name);
	Status deleteSharedSegment(const char* name);
private:
	KernelProcess *pProcess;
	friend class System;
	friend class KernelSystem;
};
