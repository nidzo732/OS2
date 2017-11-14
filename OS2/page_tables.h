#pragma once

#define DIRECTORY_SIZE 256
#define TABLE_SIZE 64
#define VA_SIZE 24
#define PAGE_BITS 14
#define OFFSET_BITS 10
#define NO_OF_PAGES (1<<PAGE_BITS)
#define DESCRIPTOR_SIZE 16

struct PageDescriptor
{
	void *frame;
	PageDescriptor *next;
	unsigned long cluster;
	unsigned char read : 1;
	unsigned char write : 1;
	unsigned char execute : 1;
	unsigned char dirty : 1;
};
class PageTable
{
	PageDescriptor descriptors[TABLE_SIZE];
};
class PageDirectory
{
	PageTable *tables[DIRECTORY_SIZE];
};
class FreePage
{
	FreePage *next;
	unsigned char _padding[1020];
};
