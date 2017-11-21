#include<iostream>
#include<cassert>
#include "page_tables.h"
#include<mutex>
#include<list>
#include<cstdlib>
#include<ctime>
#include<thread>
#include<chrono>
#include "vm_declarations.h"
#include "KernelSystem.h"
#include "Swap.h"
#include "Process.h"
#include "System.h"
using namespace std;

#ifdef _DEBUG___2
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

#define MEMORY_CAPACITY_P 1000
#define MEMORY_CAPACITY (MEMORY_CAPACITY_P*1024)
#define PROC_COUNT 10
#define ITER1 1000
#define ITER2 1000
#define LOOPS 1

volatile bool running = true;
int p = 0;
void periodical(System *sys)
{
	while (running)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(sys->periodicJob()));
		p++;
	}
}
unsigned long long fc = 0;
void processBody(System *sys, Process *proc, char *initial, int workset)
{
	int *w = new int[workset];
	for (int i = 0; i < ITER1; i++)
	{
		for (int j = 0; j < workset; j++) w[j] = rand() % MEMORY_CAPACITY_P;
		for (int j = 0; j<ITER2; j++)
		{
			{
				mtx.lock();
				VirtualAddress addr = w[rand() % workset] * 1024 + rand() % 1024;
				auto result = sys->access(proc->getProcessId(), addr, READ);
				RQD(result, TRAP);
				if (result == PAGE_FAULT)
				{
					fc++;
					result = proc->pageFault(addr);
					RQE(result, OK);
				}
				result = sys->access(proc->getProcessId(), addr, READ);
				if (result != OK) std::cout << i << '\n';
				RQE(result, OK);
				void *stored = proc->getPhysicalAddress(addr);
				RQD(stored, (void*)nullptr);
				char *storedc = (char*)stored;
				RQE(*storedc, initial[addr]);
				mtx.unlock();
			}
			{
				mtx.lock();
				VirtualAddress addr = w[rand() % workset] * 1024 + rand() % 1024;
				auto result = sys->access(proc->getProcessId(), addr, WRITE);
				RQD(result, TRAP);
				if (result == PAGE_FAULT)
				{
					fc++;
					result = proc->pageFault(addr);
					RQE(result, OK);
				}
				result = sys->access(proc->getProcessId(), addr, WRITE);
				if (result != OK) std::cout << i << '\n';
				RQE(result, OK);
				void *stored = proc->getPhysicalAddress(addr);
				RQD(stored, (void*)nullptr);
				char *storedc = (char*)stored;
				char gen = rand() % 128;
				initial[addr] = gen;
				*storedc = gen;
				mtx.unlock();
			}
		}
	}
	std::cout << "\nPROCDONE "<<workset<<"\n";
	delete[] w;
}

int main()
{
	//cout << sizeof(PageDescriptor);
	//return 0;
	char **initial = new char*[PROC_COUNT];
	char *phymem = new char[MEMORY_CAPACITY];
	char *pmtmem = new char[102400000];
	initial[0] = new char[MEMORY_CAPACITY];
	for (int i = 0; i < MEMORY_CAPACITY; i++)
	{
		initial[0][i] = rand() % 128;
	}
	for (int j = 1; j < PROC_COUNT; j++)
	{
		initial[j] = new char[MEMORY_CAPACITY];
		memcpy(initial[j], initial[0], MEMORY_CAPACITY);
	}
	auto part = new Partition("p1.ini");
	System *sys = new System(phymem, MEMORY_CAPACITY_P, pmtmem, 100000, part);
	auto pThrad = std::thread(periodical, sys);
	std::list<Process*> processes;
	for (int z = 0; z < LOOPS; z++)
	{
		auto proc0 = sys->createProcess();
		proc0->loadSegment(0, MEMORY_CAPACITY_P, READ_WRITE, initial[0]);
		processes.push_back(proc0);
		for (int i = 1; i < PROC_COUNT; i++)
		{
			/*auto proc = sys->createProcess();
			auto status = proc->loadSegment(0, MEMORY_CAPACITY_P, READ_WRITE, initial[i]);*/
			auto proc = sys->clone(proc0->getProcessId());
			assert(proc != nullptr);
			processes.push_back(proc);
		}
		std::list<std::thread*> threads;
		auto iter = processes.begin();
		for (int i = 0; i < PROC_COUNT; i++, iter++)
		{
			threads.push_back(new std::thread(processBody, sys, *iter, initial[i], 20 - rand() % 10));
		}
		for (auto proc : threads)
		{
			proc->join();
		}
		threads.clear();
		for (auto proc : processes)
		{
			proc->deleteSegment(0);
		}
		processes.clear();
	}
	running = false;
	pThrad.join();
	std::cout << "\n\nFAULTS: " << fc << "(" << ((double)fc) / ITER1 / ITER2 *100.0 / LOOPS/ PROC_COUNT << "%)";
}