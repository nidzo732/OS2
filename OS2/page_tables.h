#pragma once
#include "vm_declarations.h"

struct PageDescriptor
{
	Frame frame;
	Frame next;
	AccessRight privileges : 2;
	unsigned char dirty : 1;
	unsigned char reference : 1;
	unsigned char loaded : 1;
	unsigned char used : 1;
	unsigned char swapped : 1;
	bool validAccess(AccessType type)
	{
		switch (type)
		{
		case AccessType::EXECUTE:
			if (privileges == EXECUTE) break; else return false;
		case AccessType::READ:
			if (privileges == READ || privileges == READ_WRITE) break; else return false;
		case AccessType::WRITE:
			if (privileges == WRITE || privileges == READ_WRITE) break; else return false;
		case AccessType::READ_WRITE:
			if (privileges == READ_WRITE) break; else return false;
		}
		return true;
	}
};
struct PageTable
{
	PageDescriptor descriptors[TABLE_SIZE];
	bool empty()
	{
		for (int i = 0; i < TABLE_SIZE; i++) if (descriptors[i].used) return false;
		return true;
	}
};
struct PageDirectory
{
	Frame tables[DIRECTORY_SIZE];
};
struct FreePage
{
	Frame next:32;
	unsigned char _padding[1020];
};
