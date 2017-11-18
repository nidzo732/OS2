#include "part.h"
#include<cstring>
#include<exception>
#define PART_SIZE 10000

class PartitionImpl
{
public:
	char** clusters;
};
Partition::Partition(const char *)
{
	myImpl = new PartitionImpl();
	myImpl->clusters = new char*[PART_SIZE];
	for (int i = 0; i < PART_SIZE; i++)
	{
		myImpl->clusters[i] = new char[ClusterSize];
	}
}

ClusterNo Partition::getNumOfClusters() const
{
	return PART_SIZE;
}

int Partition::readCluster(ClusterNo cluster, char * buffer)
{
	if (cluster > PART_SIZE) throw std::exception("Accessing nonexistent cluster");
	std::memcpy(buffer, myImpl->clusters[cluster], ClusterSize);
	return 1;
}

int Partition::writeCluster(ClusterNo cluster, const char * buffer)
{
	std::memcpy(myImpl->clusters[cluster], buffer, ClusterSize);
	return 1;
}

Partition::~Partition()
{
	for (int i = 0; i < PART_SIZE; i++)
	{
		delete[] myImpl->clusters[i];
	}
	delete[] myImpl;
}
