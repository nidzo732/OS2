#include "vm_declarations.h"
#include "page_tables.h"
#include<list>
#include<memory>
#pragma once
#define MIN_WSET_SIZE 2
class KernelSystem;
class Segment;
class KernelProcess
{
public:
	KernelProcess(ProcessId pid, KernelSystem *system);
	KernelProcess(const KernelProcess &p) = delete;
	KernelProcess(KernelProcess &&p) = delete;
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
	float faultFrequency();
	friend class KernelSystem;
	int allocPage(PageNum page, AccessRight flags);
	void deallocPage(PageNum page);
	void advanceClock();
	void sacrificePage();
	void evict(PageDescriptor& decriptor);
	void loadPage(PageDescriptor& descriptor);
private:
	KernelSystem *system;
	ProcessId pid;
	PageDirectory *directory;
	PageDescriptor *hand=nullptr;
	PageDescriptor *prev=nullptr;
	std::list<std::shared_ptr<Segment>> segments;

	AccessType faultAccess;

	unsigned long wsetSize = 2;
	unsigned long pagesUsed = 0;

	unsigned long accessCount = 0;
	unsigned long faultCount = 0;
};
