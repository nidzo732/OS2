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
	float evictionRating();
	friend class KernelSystem;
	friend class Process;
	int allocPage(PageNum page, AccessRight flags);
	void deallocPage(PageNum page);
	void sacrificePage();
	void evict(std::list<PageDescriptor*>::iterator page);
	void loadPage(PageDescriptor& descriptor);
private:
	KernelSystem *system;
	ProcessId pid;
	PageDirectory *directory;
	std::list<std::shared_ptr<Segment>> segments;
	std::list<PageDescriptor*> loadedPages;
	std::list<PageDescriptor*>::iterator clockHand;

	AccessType faultAccess;

	unsigned long wsetSize = 1000000;

	unsigned long accessCount = 0;
	unsigned long faultCount = 0;
};
