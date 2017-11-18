#pragma once
#include "vm_declarations.h"
class KernelSystem;
class KernelProcess;
class Segment
{
public:
	Segment(VirtualAddress start, PageNum size, char *name = nullptr);
	Segment(const Segment &s) = delete;
	Segment(Segment &&s) = delete;
	bool overlap(VirtualAddress start, PageNum size);
	~Segment();
	friend class KernelSystem;
	friend class KernelProcess;
private:
	char *name;
	Segment *next;
	Segment *globalSegment = nullptr;
	VirtualAddress start;
	PageNum size;
	int references = 0;
};

