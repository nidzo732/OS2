#include "part.h"

Partition::Partition(const char *)
{
}

ClusterNo Partition::getNumOfClusters() const
{
	return ClusterNo();
}

int Partition::readCluster(ClusterNo, char * buffer)
{
	return 0;
}

int Partition::writeCluster(ClusterNo, const char * buffer)
{
	return 0;
}

Partition::~Partition()
{
}
