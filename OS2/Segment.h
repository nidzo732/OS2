#pragma once
#include "vm_declarations.h"
#include<list>
class KernelSystem;
class KernelProcess;
class Segment
{
public:
	Segment(VirtualAddress start, PageNum size, char *name = nullptr);
	Segment(const Segment &s) = delete;
	Segment(Segment &&s) = delete;
	bool overlap(VirtualAddress start, PageNum size);
	bool contains(PageNum page);
	~Segment();
	friend class KernelSystem;
	friend class KernelProcess;
private:
	char *name;
	Segment *origin = nullptr;
	std::list<KernelProcess*> users;
	VirtualAddress start;
	PageNum size;
	int references = 0;
};

