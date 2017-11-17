#include "KernelProcess.h"
#include "KernelSystem.h"
#include "Segment.h"
KernelProcess::KernelProcess(ProcessId pid, KernelSystem * system)
	:pid(pid), system(system), directory(nullptr)
{
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
	if (startAddress & ((1 << OFFSET_BITS) - 1)) return TRAP;
	Segment *existing = segments;
	while (existing)
	{
		if (existing->overlap(startAddress, segmentSize)) return TRAP;
		existing = existing->next;
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
	newSegment->next = segments;
	segments = newSegment;
	return OK;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags)
{
	std::lock_guard<std::mutex> _guard(system->getMutex());
	return allocSegment(startAddress, segmentSize, flags);
}
void KernelProcess::logResult(Status status, AccessType type)
{
	if (status == PAGE_FAULT)
	{
		this->faultAccess = type;
	}
}
Status KernelProcess::access(VirtualAddress address, AccessType type)
{
	auto offset = address&OFFSET_MASK;
	auto page = (address&PAGE_MASK) >> OFFSET_BITS;
	auto table = (address&TABLE_MASK) >> (OFFSET_BITS + PAGE_TABLE_BITS);
	if (this->directory == nullptr) return PAGE_FAULT;
	PageTable *pTable = (PageTable*)system->pmtMemory.getPointer(this->directory->tables[table]);
	if (pTable == nullptr) return PAGE_FAULT;
	PageDescriptor& descriptor = pTable->descriptors[page];
	if (!(descriptor.used)) return PAGE_FAULT;
	if (descriptor.loaded)
	{
		if (!descriptor.validAccess(type)) return PAGE_FAULT;
		return OK;
	}
	else return PAGE_FAULT;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessRight flags, void * content)
{
	std::lock_guard<std::mutex> _guard(system->getMutex());
	auto status = allocSegment(startAddress, segmentSize, flags);
	if (status != OK) return status;
	for (PageNum i = 0; i < segmentSize; i++)
	{
		void *ptr = resolveAddress(startAddress + i*PAGE_SIZE);
		if (ptr == 0)
		{
			handlePageFault(startAddress + i*PAGE_SIZE);
			ptr = resolveAddress(startAddress + i*PAGE_SIZE);
			if (ptr == 0) return TRAP;
		}
		memcpy(ptr, &((char*)content)[i*PAGE_SIZE], PAGE_SIZE);
	}
	return OK;
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress)
{
	std::lock_guard<std::mutex> _guard(system->getMutex());
	return Status();
}

Status KernelProcess::handlePageFault(VirtualAddress address)
{
	auto offset = address&OFFSET_MASK;
	auto page = (address&PAGE_MASK) >> OFFSET_BITS;
	auto table = (address&TABLE_MASK) >> (OFFSET_BITS + PAGE_TABLE_BITS);
	PageTable *pTable = (PageTable*)system->pmtMemory.getPointer(this->directory->tables[table]);
	if (pTable == nullptr) return TRAP;
	PageDescriptor& descriptor = pTable->descriptors[page];
	if (!(descriptor.used)) return TRAP;
	if (!descriptor.validAccess(faultAccess)) return TRAP;
	loadPage(descriptor);
	if (descriptor.loaded) return OK;
	else return TRAP;
}
Status KernelProcess::pageFault(VirtualAddress address)
{
	std::lock_guard<std::mutex> _guard(system->getMutex());
	return handlePageFault(address);
}
PhysicalAddress KernelProcess::resolveAddress(VirtualAddress address)
{
	if (directory == nullptr) return 0;
	auto offset = address&OFFSET_MASK;
	auto page = (address&PAGE_MASK) >> OFFSET_BITS;
	auto table = (address&TABLE_MASK) >> (OFFSET_BITS + PAGE_TABLE_BITS);
	PageTable *pTable = (PageTable*)system->pmtMemory.getPointer(this->directory->tables[table]);
	if (pTable == nullptr) return 0;
	PageDescriptor& descriptor = pTable->descriptors[page];
	if (!(descriptor.loaded)) return 0;
	return (char*)system->processMemory.getPointer(descriptor.frame) + offset;
}
PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{
	std::lock_guard<std::mutex> _guard(system->getMutex());
	return resolveAddress(address);
}

int KernelProcess::allocPage(PageNum page, AccessRight flags)
{
	if (directory == nullptr) directory = system->fetchDirectory();
	if (directory == nullptr) return 0;
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = (PageTable*)system->pmtMemory.getPointer(directory->tables[table]);
	if (pTable== nullptr)
	{
		pTable = system->fetchTable();
		if (pTable == nullptr) return 0;
		directory->tables[table] = system->pmtMemory.getFrame(pTable);
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
	auto table = page >> PAGE_TABLE_BITS;
	PageTable *pTable = (PageTable*)system->pmtMemory.getPointer(directory->tables[table]);
	if (pTable == nullptr) throw std::exception();
	page = page&((1 << PAGE_TABLE_BITS) - 1);
	if(!pTable->descriptors[page].used) throw std::exception();
	pTable->descriptors[page].used = 0;
	if (pTable->empty()) system->freeTable(pTable);
	directory->tables[table] = system->pmtMemory.getFrame(nullptr);
}

void KernelProcess::advanceClock()
{
}

void KernelProcess::sacrificePage()
{
}

void KernelProcess::loadPage(PageDescriptor& descriptor)
{
	if (this->pagesUsed == this->wsetSize)
	{
		sacrificePage();
	}
	FreePage *page = system->processMemory.fetchFreePage();
	if (page == nullptr) return;
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
	pagesUsed++;
}
