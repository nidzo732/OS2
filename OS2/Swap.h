#pragma once
#include<memory>
#include "part.h"
#include "page_tables.h"
class Swap
{
public:
	Swap(Partition *partition);
	ClusterNo get();
	void release(ClusterNo cluster);
	void read(ClusterNo cluster, char *buffer);
	void write(ClusterNo cluster, void* buffer);
	bool clustersAvailable();
	~Swap();
private:
	void claim(ClusterNo cluster);
	void allocateAnother();
	Partition *partition;
	ClusterNo allocated = 0;
	ClusterNo next = NULL_FRAME;
	ClusterNo used = 0;
};

