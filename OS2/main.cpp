#include<iostream>
#include<cassert>
#include "page_tables.h"
#include<mutex>
#include "vm_declarations.h"
using namespace std;

int main()
{
	assert(sizeof(PageDescriptor) == DESCRIPTOR_SIZE);
	assert(sizeof(PageTable) == PAGE_SIZE);
	assert(sizeof(PageDirectory) == PAGE_SIZE);
	assert(sizeof(FreePage) == PAGE_SIZE);
	assert(NO_OF_PAGES == DIRECTORY_SIZE*TABLE_SIZE);
	PageDescriptor d;
	d.read = 0;
	d.write = 0;
	d.execute = 0;
	cout << (int)(d.read) << (int)(d.write) << (int)(d.execute) << '\n';
	d.execute=1;
	cout << (int)(d.read) << (int)(d.write) << (int)(d.execute) << '\n';
	cout << "ALL OK\n";
}