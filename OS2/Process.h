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
private:
	KernelProcess *pProcess;
	friend class System;
	friend class KernelSystem;
};
