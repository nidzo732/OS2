#include "Segment.h"
#include<cstring>


Segment::Segment(VirtualAddress start, PageNum size, char * name)
	:start(start), size(size)
{
	if (name != nullptr)
	{
		this->name = new char[strlen(name) + 1];
		memcpy(this->name, name, strlen(name) + 1);
	}
	else
	{
		this->name = nullptr;
	}
}

bool Segment::overlap(VirtualAddress start, PageNum size)
{
	VirtualAddress ourEnd = this->start + this->size*PAGE_SIZE;
	VirtualAddress end = start + size*PAGE_SIZE;
	return (start >= this->start&&start < ourEnd) || (end >= this->start&&end < ourEnd);
}

bool Segment::contains(PageNum page)
{
	auto pageLo = start / PAGE_SIZE;
	return page >= pageLo && page < pageLo + size;
}

Segment::~Segment()
{
}
