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

/*void allocat(Process *proc, char* initial, System *sys)
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
}*/
unsigned long long fc = 0;
void processBody(System *sys, Process *proc, char *initial, int workset)
{
	int *w = new int[workset];
	for (int i = 0; i < 10000; i++)
	{
		for (int j = 0; j < workset; j++) w[j] = rand() % 100;
		for (int j = 0; j<10000; j++)
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
	}
	std::cout << "\nPROCDONE "<<workset<<"\n";
	delete[] w;
}
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
int main()
{
	char* pmtSpace = new char[1024*10000];
	char* vmSpace = new char[1024*10000];
	auto part = new Partition("pera");
	auto sys = new System(vmSpace, 100, pmtSpace, 10000, part);
	auto pTask = std::thread(periodical, sys);
	char *initial = new char[102400];
	for (int i = 0; i < 102400; i++)
	{
		initial[i] = rand() % 128;
	}
	initial[10239] = '\0';
	auto proc1 = sys->createProcess();
	auto proc2 = sys->createProcess();
	auto proc3 = sys->createProcess();
	auto proc4 = sys->createProcess();
	auto proc5 = sys->createProcess();
	auto result = proc1->loadSegment(0, 100, READ_WRITE, initial);
	RQE(result, OK);
	result = proc2->loadSegment(0, 100, READ_WRITE, initial);
	RQE(result, OK);
	result = proc3->loadSegment(0, 100, READ_WRITE, initial);
	RQE(result, OK);
	result = proc4->loadSegment(0, 100, READ_WRITE, initial);
	RQE(result, OK);
	result = proc5->loadSegment(0, 100, READ_WRITE, initial);
	RQE(result, OK);
	auto t1 = std::thread(processBody, sys, proc1, initial, 10);
	auto t2 = std::thread(processBody, sys, proc2, initial, 20);
	auto t3 = std::thread(processBody, sys, proc3, initial, 30);
	auto t4 = std::thread(processBody, sys, proc4, initial, 5);
	auto t5 = std::thread(processBody, sys, proc5, initial, 45);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	running = false;
	pTask.join();
	cout << part->ax<<'\n';
	cout << fc << '\n';
	cout << "ALL_OK";
}