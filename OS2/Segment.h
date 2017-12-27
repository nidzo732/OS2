#pragma once
#include "vm_declarations.h"
class KernelSystem;
class KernelProcess;
class Segment
{
public:
	Segment(VirtualAddress start, PageNum size);
	bool overlap(VirtualAddress start, PageNum size);
	~Segment();
	friend class KernelSystem;
	friend class KernelProcess;
private:
	VirtualAddress start;
	PageNum size;
	int references = 0;
};

