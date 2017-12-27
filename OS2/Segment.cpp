#include "Segment.h"
#include<cstring>


Segment::Segment(VirtualAddress start, PageNum size)
	:start(start), size(size)
{
}

bool Segment::overlap(VirtualAddress start, PageNum size)
{
	VirtualAddress ourEnd = this->start + this->size*PAGE_SIZE;
	VirtualAddress end = start + size*PAGE_SIZE;
	return (start >= this->start&&start < ourEnd) || (end >= this->start&&end < ourEnd);
}

Segment::~Segment()
{
}
