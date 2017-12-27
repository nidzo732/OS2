#include "KernelProcess.h"
#include "KernelSystem.h"
#include "Segment.h"
#include "Process.h"
#include "constants.h"

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
		deleteSegment(segment->start);
	}
	if(directory!=nullptr) system->freePage((FreePage*)directory, true);
	system->processes[this->pid-system->pdGelta] = nullptr;
}

ProcessId KernelProcess::getProcessId() const
{
	return pid;
}
Status KernelProcess::allocSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags)
{
	if (startAddress & ((1 << OFFSET_BITS) - 1))
	{
		return TRAP;
	}
	if (startAddress + segmentSize*PAGE_SIZE > MAX_VA)
	{
		return TRAP;
	}
	for (auto seg : segments)
	{
		if (seg->overlap(startAddress, segmentSize))
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
	std::shared_ptr<Segment> newSegment = std::make_shared<Segment>(startAddress, segmentSize);
	segments.push_back(newSegment);
	system->pagesAllocated(segmentSize);
	return OK;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags)
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
		descriptor.reference = 1;
		return OK;
	}
	else return PAGE_FAULT;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void * content)
{

	auto status = allocSegment(startAddress, segmentSize, flags);
	if (status != OK) return status;
	for (PageNum i = 0; i < segmentSize; i++)
	{
		accessCount++;
		void *ptr = resolveAddress(startAddress + i*PAGE_SIZE);
		if (ptr == 0)
		{
			if (handlePageFault(startAddress + i*PAGE_SIZE) != OK)
			{
				return TRAP;
			}
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
		if (seg->start == startAddress)
		{
			VirtualAddress startPage = startAddress >> OFFSET_BITS;
			for (unsigned long i = startPage; i < startPage + seg->size; i++)
			{
				deallocPage(i);
			}
			segments.remove(seg);
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
	if (!descriptor.loaded)
	{
		if(loadPage(descriptor, address >> OFFSET_BITS)!=OK) return TRAP;
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

int KernelProcess::allocPage(PageNum page, AccessType flags)
{
	if (directory == nullptr) directory = system->fetchDirectory();
	if (directory == nullptr) return 0;
	auto table = page >> PAGE_NUMBER_BITS;
	if (table >= DIRECTORY_SIZE) return 0;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr)
	{
		pTable = system->fetchTable();
		if (pTable == nullptr) return 0;
		directory->tables[table] = pTable;
	}
	page = page&((1 << PAGE_NUMBER_BITS) - 1);
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
	auto table = page >> PAGE_NUMBER_BITS;
	PageTable *pTable = directory->tables[table];
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_NUMBER_BITS) - 1);
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
	system->pagesFreed(1);
	pTable->descriptors[page].used = 0;
	if (pTable->empty())
	{
		system->freeTable(pTable);
		directory->tables[table] = nullptr;
	}
}
Status KernelProcess::sacrificePage()
{
	if (loadedPages.size() == 0) return TRAP;
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
	return evict(target, pn);
}

Status KernelProcess::evict(std::list<ClockListItem>::iterator page, PageNum pg)
{
	if (!system->swap.clustersAvailable()) return TRAP;
	PageDescriptor &descriptor = *((*page).descriptor);
	FreePage *targetPage = (FreePage*)system->processMemory.getPointer(descriptor.frame);
	ClusterNo swapBlock = system->swap.get();
	system->swap.write(swapBlock, targetPage);
	descriptor.frame = swapBlock;
	descriptor.loaded = 0;
	descriptor.swapped = 1;
	system->freePage(targetPage);
	loadedPages.erase(page);
	return OK;
}

Status KernelProcess::loadPage(PageDescriptor& descriptor, PageNum pg)
{
	if (system->beganSwapping() && this->loadedPages.size() >= this->wsetSize)
	{
		sacrificePage();
	}
	FreePage *page = system->getFreePage();
	if (page == nullptr)
	{
		return TRAP;
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
	loadedPages.push_back(ClockListItem(&descriptor, pg));
	return OK;
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