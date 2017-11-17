#include "vm_declarations.h"
#include "page_tables.h"
#pragma once
#define MIN_WSET_SIZE 2
class KernelSystem;
class Segment;
class KernelProcess
{
public:
	KernelProcess(ProcessId pid, KernelSystem *system);
	~KernelProcess();
	ProcessId getProcessId() const;
	Status allocSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags);
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags);
	void logResult(Status status, AccessType type);
	Status access(VirtualAddress address, AccessType type);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status handlePageFault(VirtualAddress address);
	Status pageFault(VirtualAddress address);
	PhysicalAddress resolveAddress(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	friend class KernelSystem;
	int allocPage(PageNum page, AccessRight flags);
	void deallocPage(PageNum page);
	void advanceClock();
	void sacrificePage();
	void loadPage(PageDescriptor& descriptor);
private:
	KernelSystem *system;
	KernelProcess *next;
	ProcessId pid;
	PageDirectory *directory;
	PageDescriptor *hand=nullptr;
	PageDescriptor *prev=nullptr;
	Segment *segments = nullptr;

	AccessType faultAccess;

	unsigned long wsetSize = 2;
	unsigned long pagesUsed = 0;
};
