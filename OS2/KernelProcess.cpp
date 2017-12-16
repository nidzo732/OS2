#include "KernelProcess.h"
#include "KernelSystem.h"
#include "Segment.h"
#include "Process.h"

#include<iostream>

KernelProcess::KernelProcess(ProcessId pid, KernelSystem * system)
	:pid(pid), system(system), directory(nullptr)
{
	clockHand = loadedPages.begin();
}

KernelProcess::~KernelProcess()
{
	auto iter = segments.begin();
	while (iter != segments.end())
	{
		auto segment = *iter;
		iter++;
		if (segment.segment->shared())
		{
			disconnectSharedSegment(segment.segment->name.c_str());
		}
		else
		{
			deleteSegment(segment.start);
		}
	}
	system->freePage((FreePage*)directory, true);
	system->processes.erase(this->pid);
}

ProcessId KernelProcess::getProcessId() const
{
	return pid;
}
Status KernelProcess::allocSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, int shared)
{
	if (startAddress & ((1 << OFFSET_BITS) - 1))
	{
		return TRAP;
	}
	for (auto seg : segments)
	{
		if (seg.segment->overlap(startAddress, segmentSize, seg.start))
		{
			return TRAP;
		}
	}
	VirtualAddress startPage = startAddress >> OFFSET_BITS;
	for (unsigned long i = startPage; i < startPage + segmentSize; i++)
	{
		if (!allocPage(i, flags, shared))
		{
			for (unsigned long j = startPage; j < i; j++)
			{
				deallocPage(j);
			}
			return TRAP;
		}
	}
	if (!shared)
	{
		std::shared_ptr<Segment> newSegment = std::make_shared<Segment>(segmentSize);
		newSegment->users.push_back(Segment::SegmentUser(this, startAddress));
		segments.push_back(SegmentReference(std::shared_ptr<Segment>(newSegment), startAddress));
		system->pagesAllocated(segmentSize);
	}
	return OK;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags)
{
	return allocSegment(startAddress, segmentSize, flags);
}
void KernelProcess::logResult(Status status, AccessType type)
{
	if (status == PAGE_FAULT)
	{
		this->faultAccess = type;
		faultCount++;
	}
}
Status KernelProcess::access(VirtualAddress address, AccessType type)
{
	accessCount++;
	if (this->directory == nullptr) return PAGE_FAULT;
	PageTable *pTable = (*this->directory)[address];
	if (pTable == nullptr) return PAGE_FAULT;
	PageDescriptor& descriptor = (*pTable)[address];
	if (!(descriptor.used)) return PAGE_FAULT;
	if (descriptor.loaded)
	{
		if (!descriptor.validAccess(type)) return PAGE_FAULT;
		if (descriptor.cow && (type == AccessType::WRITE || type == AccessType::READ_WRITE)) return PAGE_FAULT;
		descriptor.reference = 1;
		return OK;
	}
	else return PAGE_FAULT;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, void * content)
{

	auto status = allocSegment(startAddress, segmentSize, flags);
	if (status != OK) return status;
	for (PageNum i = 0; i < segmentSize; i++)
	{
		accessCount++;
		void *ptr = resolveAddress(startAddress + i*PAGE_SIZE);
		if (ptr == 0)
		{
			handlePageFault(startAddress + i*PAGE_SIZE);
			accessCount++;
			ptr = resolveAddress(startAddress + i*PAGE_SIZE);
			if (ptr == 0) return TRAP;
		}
		memcpy(ptr, &((char*)content)[i*PAGE_SIZE], PAGE_SIZE);
	}
	return OK;
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress, bool ignoreShared, bool allowShare)
{
	for (auto seg : segments)
	{
		if (seg.start == startAddress)
		{
			if (seg.segment->shared() && !allowShare) return TRAP;
			VirtualAddress startPage = startAddress >> OFFSET_BITS;
			for (unsigned long i = startPage; i < startPage + seg.segment->size; i++)
			{
				deallocPage(i, ignoreShared);
			}
			segments.remove(seg);
			seg.segment->users.remove(Segment::SegmentUser(this));
			return OK;
		}
	}
	return TRAP;
}

Status KernelProcess::handlePageFault(VirtualAddress address)
{
	if (this->directory == nullptr) return TRAP;
	PageTable *pTable = (*this->directory)[address];
	if (pTable == nullptr) return TRAP;
	PageDescriptor& descriptor = (*pTable)[address];
	if (!(descriptor.used)) return TRAP;
	if (!descriptor.validAccess(faultAccess)) return TRAP;
	if (descriptor.cow && (faultAccess == AccessType::READ_WRITE || faultAccess == AccessType::WRITE))
	{
		auto newPage = system->getFreePage();
		if (newPage == nullptr)
		{
			return TRAP;
		}
		system->pagesAllocated(1);
		if (descriptor.loaded)
		{
			auto oldPage = getPhysicalAddress((address >> OFFSET_BITS) * 1024);
			memcpy(newPage, oldPage, PAGE_SIZE);
			descriptor.frame = system->processMemory.getFrame(newPage);
		}
		else
		{
			system->swap.read(descriptor.frame, (char*)newPage);
			descriptor.frame = system->processMemory.getFrame(newPage);
			descriptor.swapped = 0;
		}
		descriptor.cow = 0;
		descriptor.cowSeq++;
		cowAnnounceReferenceChange(address >> OFFSET_BITS, descriptor.cowCount - 1, descriptor.cowSeq-1);
		descriptor.cowCount = 0;
		if (descriptor.loaded)
		{
			if (clockHand != loadedPages.end() && clockHand->page == address >> OFFSET_BITS)
			{
				clockHand = loadedPages.end();
			}
			loadedPages.remove(ClockListItem(nullptr, address >> OFFSET_BITS));
			loadedPages.push_back(ClockListItem(&descriptor, address >> OFFSET_BITS));
		}
		else
		{
			loadedPages.push_back(ClockListItem(&descriptor, address >> OFFSET_BITS));
		}
		descriptor.loaded = 1;
	}
	else if (!descriptor.loaded)
	{
		loadPage(descriptor, address >> OFFSET_BITS);
	}
	if (descriptor.loaded) return OK;
	else
	{
		return TRAP;
	}

}
Status KernelProcess::pageFault(VirtualAddress address)
{
	return handlePageFault(address);
}
PhysicalAddress KernelProcess::resolveAddress(VirtualAddress address)
{
	if (directory == nullptr) return 0;
	PageTable *pTable = (*this->directory)[address];
	if (pTable == nullptr) return 0;
	PageDescriptor& descriptor = (*pTable)[address];
	if (!(descriptor.loaded)) return 0;
	return (char*)(system->processMemory.getPointer(descriptor.frame)) + (address&OFFSET_MASK);
}
PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{

	return resolveAddress(address);
}

float KernelProcess::faultFrequency()
{
	if (accessCount == 0) return 0;
	return ((float)faultCount) / accessCount;
}

float KernelProcess::evictionRating()
{
	float rating1 = (1 - faultFrequency());
	float rating2 = ((float)loadedPages.size()) / system->processMemory.size();
	float rating3 = ((float)loadedPages.size()) / wsetSize;
	return rating1*rating2*rating3;
}

int KernelProcess::allocPage(PageNum page, AccessRight flags, int shared)
{
	if (directory == nullptr) directory = system->fetchDirectory();
	if (directory == nullptr) return 0;
	auto table = page >> PAGE_TABLE_BITS;
	if (table >= 256) return 0;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr)
	{
		pTable = system->fetchTable();
		if (pTable == nullptr) return 0;
		directory->tables[table] = pTable;
	}
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	pTable->descriptors[page].privileges = flags;
	pTable->descriptors[page].used = 1;
	pTable->descriptors[page].loaded = 0;
	pTable->descriptors[page].swapped = 0;
	pTable->descriptors[page].shr = shared;
	return 1;
}

void KernelProcess::deallocPage(PageNum page, bool ignoreShare)
{
	if (directory == nullptr) throw std::exception();
	PageNum originalPage = page;
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (pTable->descriptors[page].swapped && !pTable->descriptors[page].cow && (!pTable->descriptors[page].shr || ignoreShare))
	{
		system->swap.release(pTable->descriptors[page].frame);
	}
	if (pTable->descriptors[page].loaded)
	{
		if (!pTable->descriptors[page].cow && (!pTable->descriptors[page].shr || ignoreShare))
		{
			system->freePage((FreePage*)system->processMemory.getPointer(pTable->descriptors[page].frame));
		}
		if (clockHand != loadedPages.end())
		{
			if ((*clockHand).page == originalPage)
			{
				clockHand = loadedPages.end();
			}
		}
		loadedPages.remove(ClockListItem(&(pTable->descriptors[page]), originalPage));
	}
	if (pTable->descriptors[page].cow)
	{
		cowAnnounceReferenceChange(originalPage, pTable->descriptors[page].cowCount - 1, pTable->descriptors[page].cowSeq);
	}
	else
	{
		system->pagesFreed(1);
	}
	pTable->descriptors[page].used = 0;
	if (pTable->empty())
	{
		system->freeTable(pTable);
		directory->tables[table] = nullptr;
	}
}

void KernelProcess::cowAnnounceReferenceChange(PageNum page, unsigned long reference, int seq)
{
	auto segment = getSegment(page);
	if (segment.segment == nullptr) return;
	for (auto user : segment.segment->users)
	{
		user.user->cowNotifyReferenceChange(page, reference, seq);
	}
}

void KernelProcess::cowNotifyReferenceChange(PageNum page, unsigned long reference, int seq)
{
	if (directory == nullptr) throw std::exception();
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (!pTable->descriptors[page].cow) return;
	if (pTable->descriptors[page].cowSeq != seq) return;
	pTable->descriptors[page].cowCount = reference;
	if (pTable->descriptors[page].cowCount == 0)
	{
		pTable->descriptors[page].cow = 0;
		pTable->descriptors[page].cowSeq++;
	}
}

void KernelProcess::notifySharedChange(PageNum page, int loaded, int swapped, Frame frame, int seq)
{
	auto originalPage = page;
	if (directory == nullptr) throw std::exception();
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (!pTable->descriptors[page].cow && !pTable->descriptors[page].shr) return;
	if (seq != -1 && pTable->descriptors[page].cowSeq != seq) return;
	if (pTable->descriptors[page].loaded && !loaded)
	{
		loadedPages.remove(ClockListItem(nullptr, originalPage));
		clockHand = loadedPages.begin();
	}
	if (!pTable->descriptors[page].loaded && loaded)
	{
		loadedPages.remove(ClockListItem(nullptr, originalPage));
		loadedPages.push_back(ClockListItem(&pTable->descriptors[page], originalPage));
		clockHand = loadedPages.begin();
	}
	pTable->descriptors[page].loaded = loaded;
	pTable->descriptors[page].frame = frame;
	pTable->descriptors[page].swapped = swapped;
}

void KernelProcess::broadcastSharedChange(PageNum pg, Frame frame, int loaded, int swapped, int seq)
{
	auto segment = getSegment(pg);
	if (segment.segment == nullptr) return;
	auto offset = pg - (segment.start / PAGE_SIZE);
	for (auto user : segment.segment->users)
	{
		user.user->notifySharedChange((user.start / PAGE_SIZE) + offset, loaded, swapped, frame, seq);
	}
}

void KernelProcess::sacrificePage()
{
	if (loadedPages.size() == 0) return;
	if (clockHand == loadedPages.end()) clockHand = loadedPages.begin();
	while ((*clockHand).descriptor->reference)
	{
		(*clockHand).descriptor->reference = 0;
		clockHand++;
		if (clockHand == loadedPages.end()) clockHand = loadedPages.begin();
	}
	std::list<ClockListItem>::iterator target = clockHand;
	PageNum pn = clockHand->page;
	clockHand++;
	evict(target, pn);
}

void KernelProcess::evict(std::list<ClockListItem>::iterator page, PageNum pg)
{
	if (!system->swap.clustersAvailable()) return;
	PageDescriptor &descriptor = *((*page).descriptor);
	FreePage *targetPage = (FreePage*)system->processMemory.getPointer(descriptor.frame);
	ClusterNo swapBlock = system->swap.get();
	system->swap.write(swapBlock, targetPage);
	descriptor.frame = swapBlock;
	descriptor.loaded = 0;
	descriptor.swapped = 1;
	system->freePage(targetPage);
	loadedPages.erase(page);
	if (descriptor.cow || descriptor.shr)
	{
		int seq = -1;
		if (descriptor.cow) seq = descriptor.cowSeq;
		broadcastSharedChange(pg, swapBlock, 0, 1, seq);
	}
}

void KernelProcess::loadPage(PageDescriptor& descriptor, PageNum pg)
{
	if (system->beganSwapping() && this->loadedPages.size() >= this->wsetSize)
	{
		sacrificePage();
	}
	FreePage *page = system->getFreePage();
	if (page == nullptr)
	{
		return;
	}
	descriptor.loaded = 1;
	if (descriptor.swapped)
	{
		ClusterNo cluster = descriptor.frame;
		system->swap.read(cluster, (char*)page);
		descriptor.swapped = 0;
		system->swap.release(cluster);
	}
	descriptor.frame = system->processMemory.getFrame(page);
	descriptor.loaded = 1;
	descriptor.reference = 0;
	if (descriptor.cow || descriptor.shr)
	{
		int seq = -1;
		if (descriptor.cow) seq = descriptor.cowSeq;
		broadcastSharedChange(pg, descriptor.frame, descriptor.loaded, 0, seq);
	}
	loadedPages.push_back(ClockListItem(&descriptor, pg));
}

void KernelProcess::updateWsetSize()
{
	if (faultFrequency() < VERY_SMALL_FAULTRATE)
	{
		wsetSize = wsetSize / WORKING_SET_DECREMENT;
		if (wsetSize < 5) wsetSize = MIN_WORKINGSET;
	}
	if (faultFrequency() > TOLERABLE_FAULTRATE)
	{
		wsetSize = wsetSize*WORKING_SET_INCREMENT;
		if (wsetSize > MAX_WORKINGSET) wsetSize = MAX_WORKINGSET;
	}
	accessCount = 0;
	faultCount = 0;
}

void KernelProcess::resetReferenceBits()
{
	for (auto page : loadedPages)
	{
		page.descriptor->reference = 0;
	}
}

bool KernelProcess::clonePage(PageNum pageFrom, PageNum pageTo, KernelProcess * target)
{
	VirtualAddress addressFrom = pageFrom << OFFSET_BITS;
	VirtualAddress addressTo = pageTo << OFFSET_BITS;
	PageTable *pTable = (*this->directory)[addressFrom];
	if (pTable == nullptr) return true;
	PageDescriptor& descriptor = (*pTable)[addressFrom];
	if (target->directory == nullptr) target->directory = system->fetchDirectory();
	if (target->directory == nullptr) return false;
	if ((*target->directory)[addressTo] == nullptr)
	{
		PageTable *table = system->fetchTable();
		if (table == nullptr) return false;
		(*target->directory)[addressTo] = table;
	}
	PageTable *newTable = (*target->directory)[addressTo];
	PageDescriptor& newDescriptor = (*newTable)[addressTo];
	if (descriptor.used)
	{
		newDescriptor = descriptor;
		if (newDescriptor.cow)
		{
			newDescriptor.cowCount += 1;
			cowAnnounceReferenceChange(pageFrom, newDescriptor.cowCount, newDescriptor.cowSeq);
		}
		else
		{
			if (descriptor.shr)
			{
				newDescriptor.shr = 1;
			}
			else
			{
				newDescriptor.cow = 1;
				descriptor.cow = 1;
				descriptor.cowCount = 1;
				newDescriptor.cowCount = 1;
			}
		}
	}
	if (newDescriptor.loaded)
	{
		target->loadedPages.push_back(ClockListItem(&newDescriptor, pageTo));
	}
	return true;
}

KernelProcess::SegmentReference KernelProcess::getSegment(PageNum page)
{
	for (auto seg : segments)
	{
		if (seg.segment->contains(page, seg.start))
		{
			return seg;
		}
	}
	return SegmentReference(std::shared_ptr<Segment>(nullptr), 0);
}

KernelProcess * KernelProcess::clone()
{
	KernelProcess *newOne = new KernelProcess(system->getNextPid(), system);
	for (auto segment : segments)
	{
		for (PageNum i = segment.start / PAGE_SIZE; i < segment.start / PAGE_SIZE + segment.segment->size; i++)
		{
			if (!clonePage(i, i, newOne))
			{
				newOne->segments.push_back(segment);
				for (PageNum j = segment.start / PAGE_SIZE; j < i; j++)
				{
					newOne->deallocPage(j);
				}
				for (auto prevSegment : segments)
				{
					if (segment.start == prevSegment.start) break;
					newOne->deleteSegment(prevSegment.start);
				}
				delete newOne;
				return nullptr;
			}
		}
		segment.segment->users.push_back(Segment::SegmentUser(newOne, segment.start));
		newOne->segments.push_back(segment);
		newOne->clockHand = newOne->loadedPages.end();
	}
	return newOne;
}

Status KernelProcess::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char * name, AccessRight flags, bool createDummy)
{
	for (auto segment : system->sharedSegments)
	{
		if (segment->operator==(name))
		{
			if (segment->size != segmentSize) return TRAP;
		}
	}
	auto result = allocSegment(startAddress, segmentSize, flags, 1);
	if (result != OK) return result;
	bool alreadyExists = false;
	for (auto segment : system->sharedSegments)
	{
		if (segment->operator==(name))
		{
			alreadyExists = true;
			if (segment->size < segmentSize) return TRAP;
			segment->users.push_back(Segment::SegmentUser(this, startAddress));
			segment->references++;
			segments.push_back(SegmentReference(segment, startAddress));
			auto firstUser = *(segment->users.begin());
			for (PageNum i = startAddress / PAGE_SIZE; i < startAddress / PAGE_SIZE + segment->size; i++)
			{
				auto offset = i - startAddress / PAGE_SIZE;
				auto srcPage = firstUser.start / PAGE_SIZE + offset;
				firstUser.user->clonePage(srcPage, i, this);
			}
		}
	}
	if (!alreadyExists)
	{
		if (createDummy)
		{
			auto pid = system->nextId++;
			Process *process = new Process(pid);
			KernelProcess *kProcess = new KernelProcess(pid, system);
			system->processes[pid] = kProcess;
			kProcess->createSharedSegment(startAddress, segmentSize, name, flags, false);
			return this->createSharedSegment(startAddress, segmentSize, name, flags, false);
		}
		else
		{
			std::shared_ptr<Segment> seg = std::make_shared<Segment>(segmentSize, name);
			seg->references = 1;
			seg->users.push_back(Segment::SegmentUser(this, startAddress));
			system->sharedSegments.push_back(std::shared_ptr<Segment>(seg));
			segments.push_back(SegmentReference(std::shared_ptr<Segment>(seg), startAddress));
			seg->dummyUser = this;
		}
	}
	return OK;
}

Status KernelProcess::disconnectSharedSegment(const char * name)
{
	std::shared_ptr<Segment> seg = nullptr;
	VirtualAddress start = 0;
	for (auto segment : segments)
	{
		if (segment.segment->operator==(name))
		{
			seg = std::shared_ptr<Segment>(segment.segment);
			start = segment.start;
			break;
		}
	}
	if (seg == nullptr) return TRAP;
	auto status = deleteSegment(start, seg->references == 1, true);
	if (status != OK) return status;
	seg->references--;
	if (seg->references == 0)
	{
		system->sharedSegments.remove(seg);
	}
	return OK;
}

Status KernelProcess::deleteSharedSegment(const char * name)
{
	std::shared_ptr<Segment> seg = nullptr;
	for (auto segment : segments)
	{
		if (segment.segment->operator==(name))
		{
			seg = segment.segment;
		}
	}
	if (seg == nullptr) return TRAP;
	for (auto iter = seg->users.begin(); iter != seg->users.end();)
	{
		auto oldUser = iter;
		iter++;
		oldUser->user->disconnectSharedSegment(name);
	}
	return OK;
}
