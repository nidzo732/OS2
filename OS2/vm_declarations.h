#pragma once
typedef unsigned long PageNum;
typedef unsigned long VirtualAddress;
typedef void* PhysicalAddress;
typedef unsigned long Time;
typedef unsigned long AccessRight;
enum Status { OK, PAGE_FAULT, TRAP };
enum AccessType { READ, WRITE, READ_WRITE, EXECUTE };
typedef unsigned ProcessId;
typedef unsigned long Frame;
#define PAGE_SIZE 1024 

#define DIRECTORY_SIZE 256
#define TABLE_SIZE 64
#define VA_SIZE 24
#define PAGE_BITS 14
#define OFFSET_BITS 10
#define NO_OF_PAGES (1<<PAGE_BITS)
#define DESCRIPTOR_SIZE 16
