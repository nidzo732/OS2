#include "KernelProcess.h"
#include "KernelSystem.h"
#include "Segment.h"
#include<iostream>

KernelProcess::KernelProcess(ProcessId pid, KernelSystem * system)
	:pid(pid), system(system), directory(nullptr)
{
	clockHand = loadedPages.begin();
}

KernelProcess::~KernelProcess()
{
}

ProcessId KernelProcess::getProcessId() const
{
	return pid;
}
Status KernelProcess::allocSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags)
{
	if (startAddress & ((1 << OFFSET_BITS) - 1))
	{
		return TRAP;
	}
	for (auto seg : segments)
	{
		if (seg.get()->overlap(startAddress, segmentSize))
		{
			return TRAP;
		}
	}
	VirtualAddress startPage = startAddress >> OFFSET_BITS;
	for (unsigned long i = startPage; i < startPage + segmentSize; i++)
	{
		if (!allocPage(i, flags))
		{
			for (unsigned long j = startPage; j < i; j++)
			{
				deallocPage(j);
			}
			return TRAP;
		}
	}
	Segment *newSegment = new Segment(startAddress, segmentSize);
	newSegment->users.push_back(this);
	segments.push_back(std::shared_ptr<Segment>(newSegment));
	system->pagesAllocated(segmentSize);
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

Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{
	for (auto seg : segments)
	{
		if (seg.get()->start == startAddress)
		{
			VirtualAddress startPage = startAddress >> OFFSET_BITS;
			for (unsigned long i = startPage; i < startPage + seg.get()->size; i++)
			{
				deallocPage(i);
			}
			auto iter = loadedPages.begin();
			system->pagesFreed(seg.get()->size);
			segments.remove(seg);
			seg.get()->users.remove(this);
			return OK;
		}
	}
	return TRAP;
}

Status KernelProcess::handlePageFault(VirtualAddress address)
{
	if (this->directory == nullptr) return TRAP;
	PageTable *pTable = (*this->directory)[address];
	if (pTable == nullptr) return (Status)44;// TRAP;
	PageDescriptor& descriptor = (*pTable)[address];
	if (!(descriptor.used)) return TRAP;
	if (!descriptor.validAccess(faultAccess)) return (Status)45;// TRAP;
	if (descriptor.cow && (faultAccess == AccessType::READ_WRITE || faultAccess == AccessType::WRITE))
	{
		auto newPage = system->getFreePage();
		if (newPage == nullptr)
		{
			return (Status)46;
		}
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
		cowAnnounceReferenceChange(address >> OFFSET_BITS, descriptor.cowCount - 1);
		descriptor.cowCount = 0;
		descriptor.loaded = 1;
		loadedPages.remove(ClockListItem(nullptr, address>>OFFSET_BITS));
		loadedPages.push_back(ClockListItem(&descriptor, address>>OFFSET_BITS));
		clockHand = loadedPages.begin();
	}
	else if(!descriptor.loaded)
	{
		loadPage(descriptor, address >> OFFSET_BITS);
	}
	if (descriptor.loaded) return OK;
	else
	{
		return (Status)47;
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

int KernelProcess::allocPage(PageNum page, AccessRight flags)
{
	if (directory == nullptr) directory = system->fetchDirectory();
	if (directory == nullptr) return 0;
	auto table = page >> PAGE_TABLE_BITS;
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
	return 1;
}

void KernelProcess::deallocPage(PageNum page)
{
	if (directory == nullptr) throw std::exception();
	PageNum originalPage = page;
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (pTable->descriptors[page].swapped)
	{
		system->swap.release(pTable->descriptors[page].frame);
	}
	if (pTable->descriptors[page].loaded)
	{
		system->freePage((FreePage*)system->processMemory.getPointer(pTable->descriptors[page].frame));
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
		cowAnnounceReferenceChange(originalPage, pTable->descriptors[page].cowCount - 1);
	}
	pTable->descriptors[page].used = 0;
	if (pTable->empty())
	{
		system->freeTable(pTable);
		directory->tables[table] = nullptr;
	}
}

void KernelProcess::cowAnnounceReferenceChange(PageNum page, unsigned long reference)
{
	auto segment = getSegment(page);
	if (segment == nullptr) return;
	for (auto user : segment->users)
	{
		user->cowNotifyReferenceChange(page, reference);
	}
}

void KernelProcess::cowNotifyReferenceChange(PageNum page, unsigned long reference)
{
	if (directory == nullptr) throw std::exception();
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (!pTable->descriptors[page].cow) return;
	pTable->descriptors[page].cowCount = reference;
	if (pTable->descriptors[page].cowCount == 0)
	{
		pTable->descriptors[page].cow = 0;
	}
}

void KernelProcess::notifyCowChange(PageNum page, int loaded, int swapped, Frame frame)
{
	auto originalPage = page;
	if (directory == nullptr) throw std::exception();
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if (!pTable->descriptors[page].used) throw std::exception();
	if (!pTable->descriptors[page].cow) return;
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

void KernelProcess::announceCowChange(PageNum pg, Frame frame, int loaded, int swapped)
{
	auto segment = getSegment(pg);
	if (segment == nullptr) return;
	for (auto user : segment->users)
	{
		user->notifyCowChange(pg, loaded, swapped, frame);
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
	if (descriptor.cow)
	{
		announceCowChange(pg, swapBlock, 0, 1);
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
	if (descriptor.cow)
	{
		announceCowChange(pg, descriptor.frame, descriptor.loaded, 0);
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

bool KernelProcess::clonePage(PageNum page, KernelProcess * target)
{
	VirtualAddress address = page << OFFSET_BITS;
	PageTable *pTable = (*this->directory)[address];
	if (pTable == nullptr) return true;
	PageDescriptor& descriptor = (*pTable)[address];
	if (target->directory == nullptr) target->directory = system->fetchDirectory();
	if (target->directory == nullptr) return false;
	if ((*target->directory)[address] == nullptr)
	{
		PageTable *table = system->fetchTable();
		if (table == nullptr) return TRAP;
		(*target->directory)[address] = table;
	}
	PageTable *newTable = (*target->directory)[address];
	PageDescriptor& newDescriptor = (*newTable)[address];
	if (descriptor.used)
	{
		newDescriptor = descriptor;
		if (newDescriptor.cow)
		{
			newDescriptor.cowCount += 1;
			cowAnnounceReferenceChange(page, newDescriptor.cowCount);
		}
		else
		{
			newDescriptor.cow = 1;
			descriptor.cow = 1;
			descriptor.cowCount = 1;
			newDescriptor.cowCount = 1;
		}
	}
	if (newDescriptor.loaded)
	{
		target->loadedPages.push_back(ClockListItem(&newDescriptor, page));
	}
	return true;
}

Segment * KernelProcess::getSegment(PageNum page)
{
	for (auto seg : segments)
	{
		if (seg.get()->contains(page))
		{
			return seg.get();
		}
	}
	return nullptr;
}

KernelProcess * KernelProcess::clone()
{
	KernelProcess *newOne = new KernelProcess(system->getNextPid(), system);
	for (auto segment : segments)
	{
		for (PageNum i = segment.get()->start; i < segment.get()->start + segment.get()->size; i++)
		{
			if (!clonePage(i, newOne))
			{
				newOne->segments.push_back(segment);
				for (unsigned long j = segment.get()->start; j < i; j++)
				{
					newOne->deallocPage(j);
				}
				for (auto prevSegment : segments)
				{
					if (segment.get()->start == prevSegment.get()->start) break;
					newOne->deleteSegment(prevSegment.get()->start);
				}
				delete newOne;
				return nullptr;
			}
		}
		segment.get()->users.push_back(newOne);
		newOne->segments.push_back(segment);
		newOne->clockHand = newOne->loadedPages.end();
	}
	return newOne;
}
