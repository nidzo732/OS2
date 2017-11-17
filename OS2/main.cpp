#include<iostream>
#include<cassert>
#include "page_tables.h"
#include<mutex>
#include "vm_declarations.h"
#include "KernelSystem.h"
#include "Swap.h"
#include "Process.h"
#include "System.h"
using namespace std;

#ifdef _DEBUG
int assertions()
{
	cout << sizeof(PageDescriptor);
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

	Partition *p = new Partition("perica");
	Swap *s = new Swap(p);
	ClusterNo blk1 = s->get();
	ClusterNo blk2 = s->get();
	ClusterNo blk3 = s->get();
	s->release(blk1);
	s->release(blk2);
	s->release(blk3);
	assert(s->get() == blk3);
	assert(s->get() == blk2);
	assert(s->get() == blk1);
	blk1 = s->get();
	auto blk1b = s->read(blk1);
	auto blk2b = s->read(blk2);
	int *cl1 = (int*)blk1b.get();
	*cl1 = 42;
	int *cl2 = (int*)blk2b.get();
	*cl2 = 84;
	s->write(blk1, blk1b.get());
	s->write(blk2, blk2b.get());
	blk1b = s->read(blk1);
	blk2b = s->read(blk2);
	assert(*((int*)blk1b.get()) == 42);
	assert(*((int*)blk2b.get()) == 84);
	delete s;
	delete p;
	cout << "\nASSERTIONS OK\n";
	delete[] memory;
	return 0;
}
int z = assertions();
#endif // DEBUG

int main()
{
	char* pmtSpace = new char[51200];
	char* vmSpace = new char[1024 * 1024];
	char *initial = new char[10240];
	for (int i = 0; i < 10239; i++)
	{
		initial[i] = i%131;
	}
	initial[10239] = '\0';
	auto sys=new System(vmSpace, 1024, pmtSpace, 50, new Partition("pera"));
	auto proc = sys->createProcess();
	auto proc2 = sys->createProcess();
	auto result = proc->createSegment(0, 10, READ_WRITE);
	auto result2 = proc->loadSegment(511*1024, 10, READ_WRITE, initial);
	for (int i = 0; i < 10240;i++)
	{
		void *stored = proc->getPhysicalAddress(511 * 1024 + i);
		assert(stored != nullptr);
		char *storedc = (char*)stored;
		assert(*storedc == initial[i]);
	}
	cout << "\n\nMAPPING OK\n\n";
	auto result3 = proc->createSegment(511 * 1024, 1, READ_WRITE);
	proc->getPhysicalAddress(511 * 1024 + 13);
	proc->pageFault(511 * 1024 + 13);
	proc->pageFault(13);
	proc->pageFault(517);
	proc2->createSegment(513 * 1024, 3, READ);
	sys->access(proc2->getProcessId(), 513 * 1024 + 13, READ);
	proc2->pageFault(513 * 1024 + 13);
	sys->access(proc2->getProcessId(), 513 * 1024 + 13, EXECUTE);
	proc2->getPhysicalAddress(513 * 1024 + 2);
	proc->pageFault(1027);
	proc->pageFault(3075);
	proc->getPhysicalAddress(511 * 1024 + 13);
	proc->getPhysicalAddress(13);
	proc->getPhysicalAddress(517);
	proc->getPhysicalAddress(1027);
	proc->getPhysicalAddress(3075);
	proc->getPhysicalAddress(4100);
	cout << proc->getPhysicalAddress(513 * 1024 + 2) << ' ' << "\n" << proc2->getPhysicalAddress(513 * 1024 + 2);
}