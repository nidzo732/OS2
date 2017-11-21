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
	struct ClockListItem
	{
		PageDescriptor *descriptor;
		PageNum page;
		ClockListItem(PageDescriptor* descriptor, PageNum page)
			:descriptor(descriptor), page(page)
		{

		}
		bool operator==(const ClockListItem& other) const
		{
			return page == other.page;
		}
	};
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
	void cowAnnounceReferenceChange(PageNum page, unsigned long reference);
	void cowNotifyReferenceChange(PageNum page, unsigned long reference);
	void notifyCowChange(PageNum page, int loaded, int swapped, Frame frame);
	void announceCowChange(PageNum pg, Frame frame, int loaded, int swapped);
	void sacrificePage();
	void evict(std::list<ClockListItem>::iterator page, PageNum pg);
	void loadPage(PageDescriptor& descriptor, PageNum pg);
	void updateWsetSize();
	void resetReferenceBits();
	bool clonePage(PageNum page, KernelProcess *target);
	Segment* getSegment(PageNum page);
	KernelProcess *clone();
private:
	KernelSystem *system;
	ProcessId pid;
	PageDirectory *directory;
	std::list<std::shared_ptr<Segment>> segments;
	std::list<ClockListItem> loadedPages;
	std::list<ClockListItem>::iterator clockHand;

	AccessType faultAccess;

	unsigned long wsetSize = 10;

	unsigned long accessCount = 0;
	unsigned long faultCount = 0;
};
