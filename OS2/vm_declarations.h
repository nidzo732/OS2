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
#define PAGEMAP_CHUNK 100
#define NULL_FRAME 0xffffffff
#define PAGE_TABLE_BITS 6
#define TABLE_BITS 8

#define OFFSET_MASK ((1<<OFFSET_BITS)-1)
#define PAGE_MASK (((1<<PAGE_TABLE_BITS)-1)<<OFFSET_BITS)
#define TABLE_MASK (((1<<TABLE_BITS)-1)<<(OFFSET_BITS+PAGE_TABLE_BITS))

#define MIN_WORKINGSET 5
#define MAX_WORKINGSET ((1<<PAGE_BITS)-1)
#define INITIAL_WORKINGSET 6
#define TOLERABLE_FAULTRATE (0.05f)
#define VERY_SMALL_FAULTRATE (0.005f)
#define WORKING_SET_INCREMENT (1.6f) 
#define WORKING_SET_DECREMENT (1.2f)
#define SWAPPINES (0.8f)
