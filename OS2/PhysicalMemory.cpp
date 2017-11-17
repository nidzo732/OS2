#include "PhysicalMemory.h"
#include<exception>

PhysicalMemory::PhysicalMemory(void * mem, PageNum capacity)
	:mem(mem), capacity(capacity)
{
}

PhysicalMemory::~PhysicalMemory()
{
}

FreePage * PhysicalMemory::fetchFreePage(bool recurse)
{
	if (!pagesAvailable()) throw std::exception("Attempted fetchFreePage on full PhysicalMemory");
	FreePage *page = nullptr;
	if (freePagesHead != nullptr)
	{
		page = freePagesHead;
		freePagesHead = (FreePage*)getPointer(freePagesHead->next);
		if (freePagesHead == nullptr) freePagesTail = nullptr;
		used++;
	}
	else if (recurse)
	{
		mapMorePages();
		page = fetchFreePage(false);
	}
	return page;
}
void PhysicalMemory::reclaimPage(FreePage * page)
{
	claimPage(page);
	used--;
}
void PhysicalMemory::claimPage(FreePage * page)
{
	if (getFrame(page) >= capacity) throw std::exception("PhysicalMemory::claimPage got someone else's page");
	if (freePagesHead == nullptr)
	{
		freePagesHead = freePagesTail = page;
	}
	else
	{
		freePagesTail->next = getFrame(page);
		freePagesTail = page;
	}
	freePagesTail->next = getFrame(nullptr);
}

void * PhysicalMemory::getPointer(Frame frame)
{
	if (frame == NULL_FRAME) return nullptr;
	if (frame >= capacity) throw std::exception("PhysicalMemory::getPointer got someone else's frame");
	auto addr = frame << OFFSET_BITS;
	return (unsigned char*)mem + addr;
}

void PhysicalMemory::mapMorePages()
{
	for (int i = 0; i < PAGEMAP_CHUNK && mapped < capacity; i++) mapAnotherPage();
}

void PhysicalMemory::mapAnotherPage()
{
	FreePage* page = &((FreePage*)(mem))[mapped];
	claimPage(page);
	this->mapped++;
}

bool PhysicalMemory::pagesAvailable()
{
	return used<capacity;
}

Frame PhysicalMemory::getFrame(void * ptr)
{
	if (ptr == nullptr) return NULL_FRAME;
	auto diff = (unsigned long long)((unsigned char*)ptr - (unsigned char*)mem);
	auto frame= diff >> OFFSET_BITS;
	if (frame >= capacity) throw std::exception("PhysicalMemory::getFrame got someone else's page");
	return frame;
}
