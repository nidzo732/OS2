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
	struct SegmentReference
	{
		std::shared_ptr<Segment> segment;
		VirtualAddress start;
		explicit SegmentReference(std::shared_ptr<Segment> segment, VirtualAddress start)
			:segment(segment), start(start)
		{

		}
		bool operator==(const SegmentReference& other) const
		{
			return segment == other.segment;
		}
	};
public:
	KernelProcess(ProcessId pid, KernelSystem *system);
	KernelProcess(const KernelProcess &p) = delete;
	KernelProcess(KernelProcess &&p) = delete;
	~KernelProcess();
	ProcessId getProcessId() const;
	Status allocSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, int shared=0);
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags);
	void logResult(Status status, AccessType type);
	Status access(VirtualAddress address, AccessType type);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		AccessRight flags, void* content);
	Status deleteSegment(VirtualAddress startAddress, bool ignoreShare = false, bool allowShare=false);
	Status handlePageFault(VirtualAddress address);
	Status pageFault(VirtualAddress address);
	PhysicalAddress resolveAddress(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	float faultFrequency();
	float evictionRating();
	friend class KernelSystem;
	friend class Process;
	int allocPage(PageNum page, AccessRight flags, int shared);
	void deallocPage(PageNum page, bool ignoreShare=false);
	void cowAnnounceReferenceChange(PageNum page, unsigned long reference, int seq);
	void cowNotifyReferenceChange(PageNum page, unsigned long reference, int seq);
	void notifySharedChange(PageNum page, int loaded, int swapped, Frame frame, int seq);
	void broadcastSharedChange(PageNum pg, Frame frame, int loaded, int swapped, int seq);
	void sacrificePage();
	void evict(std::list<ClockListItem>::iterator page, PageNum pg);
	void loadPage(PageDescriptor& descriptor, PageNum pg);
	void updateWsetSize();
	void resetReferenceBits();
	bool clonePage(PageNum pageFrom, PageNum pageTo, KernelProcess *target);
	SegmentReference getSegment(PageNum page);
	KernelProcess *clone();
	Status createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessRight flags, bool createDummy=true);
	Status disconnectSharedSegment(const char* name);
	Status deleteSharedSegment(const char* name);
private:
	KernelSystem *system;
	ProcessId pid;
	PageDirectory *directory;
	std::list<SegmentReference> segments;
	std::list<ClockListItem> loadedPages;
	std::list<ClockListItem>::iterator clockHand;

	AccessType faultAccess;

	unsigned long wsetSize = 10;

	unsigned long accessCount = 0;
	unsigned long faultCount = 0;
};
