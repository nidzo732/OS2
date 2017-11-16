#include<iostream>
#include<cassert>
#include "page_tables.h"
#include<mutex>
#include "vm_declarations.h"
#include "KernelSystem.h"
using namespace std;

#ifdef _DEBUG
int assertions()
{
	assert(sizeof(PageDescriptor) == DESCRIPTOR_SIZE);
	assert(sizeof(PageTable) == PAGE_SIZE);
	assert(sizeof(PageDirectory) == PAGE_SIZE);
	assert(sizeof(FreePage) == PAGE_SIZE);
	assert(NO_OF_PAGES == DIRECTORY_SIZE*TABLE_SIZE);
	char* memory = new char[10 * (1 << OFFSET_BITS)];
	PhysicalMemory mem(memory, 10);
	for (int i = 0; i < 10 * OFFSET_BITS; i += (1 << OFFSET_BITS))
	{
		Frame f = mem.getFrame(&(memory[i]));
		char *ptr = (char*)mem.getPointer(f);
		assert(ptr == &(memory[i]));
	}
	assert(mem.getPointer(mem.getFrame(nullptr)) == nullptr);
	cout << "\nASSERTIONS OK\n";
	delete[] memory;
	return 0;
}
int z = assertions();
#endif // DEBUG

int main()
{
	cout << "\n\nALL OK\n\n";
}