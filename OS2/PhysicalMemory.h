#pragma once
#include "page_tables.h"
#include "vm_declarations.h"

class PhysicalMemory
{
public:
	PhysicalMemory(void *mem, PageNum capacity);
	~PhysicalMemory();
	FreePage* fetchFreePage(bool recurse = true);
	void reclaimPage(FreePage *page);
	void claimPage(FreePage *page);
	bool pagesAvailable();
	Frame getFrame(void *ptr);
	void * getPointer(Frame frame);
private:
	void mapMorePages();
	void mapAnotherPage();
	void *mem;
	PageNum capacity;
	PageNum mapped=0;
	PageNum used = 0;
	FreePage *freePagesHead = nullptr;
	FreePage *freePagesTail = nullptr;
};

