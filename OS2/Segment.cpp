#include "Segment.h"
#include<cstring>


Segment::Segment(PageNum size, const char * name)
	:size(size)
{
	if (name != nullptr)
	{
		this->name = name;
	}
	else
	{
		this->name = "";
	}
}

bool Segment::overlap(VirtualAddress start, PageNum size, VirtualAddress myStart)
{
	VirtualAddress ourEnd = myStart + this->size*PAGE_SIZE;
	VirtualAddress end = start + size*PAGE_SIZE;
	return (start >= myStart&&start < ourEnd) || (end >= myStart&&end < ourEnd);
}

bool Segment::contains(PageNum page, VirtualAddress myStart)
{
	auto pageLo = myStart / PAGE_SIZE;
	return page >= pageLo && page < pageLo + size;
}

bool Segment::shared()
{
	return name != "";
}

Segment::~Segment()
{
}

bool Segment::operator==(const char* other) const
{
	if (name == "") return false;
	return name == other;
}
