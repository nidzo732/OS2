#pragma once
#include "vm_declarations.h"

struct PageDescriptor
{
	Frame frame;
	Frame next;
	unsigned char read : 1;
	unsigned char write : 1;
	unsigned char execute : 1;
	unsigned char dirty : 1;
	unsigned char reference : 1;
	unsigned char loaded : 1;
	unsigned long padding : 32;
};
struct PageTable
{
	PageDescriptor descriptors[TABLE_SIZE];
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
