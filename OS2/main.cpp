#include<iostream>
#include<cassert>
#include "page_tables.h"
#include<mutex>
#include<list>
#include<cstdlib>
#include<ctime>
#include<thread>
#include "vm_declarations.h"
#include "KernelSystem.h"
#include "Swap.h"
#include "Process.h"
#include "System.h"
using namespace std;

#ifdef _DEBUG
int assertions()
{
	srand(time(NULL));
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
std::mutex mtx;

#define RQD(r,t) if((r)==(t)){std::cout<<(r)<<' '<<(t)<<' '<<__LINE__<<'\n';exit(1);}
#define RQE(r,t) assert((r)==(t));if((r)!=(t)){std::cout<<(r)<<' '<<(t)<<' '<<__LINE__<<'\n';exit(1);}

void allocat(Process *proc, char* initial, System *sys)
{
	auto result = proc->loadSegment(511 * 1024, 100, READ_WRITE, initial);
	RQE(result, OK);
}
void deallocat(Process *proc, char* initial, System *sys)
{
	auto result = proc->deleteSegment(511 * 1024);
	RQE(result, OK);
}
int fc = 0;
void runProcess(Process *proc, char* initial, System *sys)
{
	for (int i = 0; i < 10000; i++)
	{
		mtx.lock();
		VirtualAddress addr = 511 * 1024 + (rand() % 100)*1024;
		auto result = sys->access(proc->getProcessId(), addr, READ);
		RQD(result, TRAP)
		if (result == PAGE_FAULT)
		{
			result = proc->pageFault(addr);
			fc++;
			RQE(result, OK);
		}
		result = sys->access(proc->getProcessId(), addr, READ);
		if (result != OK) std::cout << i << '\n';
		RQE(result, OK);
		void *stored = proc->getPhysicalAddress(addr);
		RQD(stored, (void*)nullptr);
		char *storedc = (char*)stored;
		RQE(*storedc, initial[addr - 511 * 1024]);
		//cout << storedc << '\n';
		//system("cls");
		mtx.unlock();
	}
	cout << "\n\nALL OK\n\n";
}
int main()
{
	char* pmtSpace = new char[1024*10000];
	char* vmSpace = new char[1024 * 10000];
	char *initial = new char[102400];
	char txt[] = "Vucicu, pederu!";
	int l = strlen(txt) + 1;
	for (int i = 0; i < 102400; i++)
	{
		initial[i] = rand() % 128;
	}
	initial[10239] = '\0';
	int c = 80;
	auto sys=new System(vmSpace, 37, pmtSpace, 10000, new Partition("pera"));
	std::list<std::thread*> threads;
	std::list<Process*> processes;
	for (int i = 0; i < c; i++)
	{
		auto p = sys->createProcess();
		processes.push_back(p);
	}
	for (int z = 0; z < 100; z++)
	{
		for (auto p : processes)
		{
			auto t = new std::thread(allocat, p, initial, sys);
			threads.push_back(t);
		}
		for (auto t : threads)
		{
			t->join();
		}
		threads.clear();
		for (auto p : processes)
		{
			auto t = new std::thread(runProcess, p, initial, sys);
			threads.push_back(t);
		}
		for (auto t : threads)
		{
			t->join();
		}
		threads.clear();
		for (auto p : processes)
		{
			auto t = new std::thread(deallocat, p, initial, sys);
			threads.push_back(t);
		}
		for (auto t : threads)
		{
			t->join();
		}
		threads.clear();
	}
	cout << fc<<"\n\nMAPPING OK\n\n";
	/*auto result3 = proc->createSegment(511 * 1024, 1, READ_WRITE);
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
	cout << proc->getPhysicalAddress(513 * 1024 + 2) << ' ' << "\n" << proc2->getPhysicalAddress(513 * 1024 + 2);*/
}