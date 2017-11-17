#include "Swap.h"

Swap::Swap(Partition * partition)
	:partition(partition)
{
}

ClusterNo Swap::get()
{
	if (!clustersAvailable()) throw std::exception("Attempted get on a full partition");
	if (next == NULL_FRAME)
	{
		allocateAnother();
	}
	auto cluster = next;
	std::shared_ptr<char> buffer = std::shared_ptr<char>(new char[ClusterSize]);
	partition->readCluster(next, buffer.get());
	FreePage *pg = (FreePage*)buffer.get();
	next = pg->next;
	used++;
	return cluster;
}

void Swap::release(ClusterNo cluster)
{
	claim(cluster);
	used--;
}

std::shared_ptr<char> Swap::read(ClusterNo cluster)
{
	std::shared_ptr<char> buffer = std::shared_ptr<char>(new char[ClusterSize]);
	int response = partition->readCluster(cluster, buffer.get());
	if (response != 1) throw std::exception("Read failure");
	return buffer;
}

void Swap::read(ClusterNo cluster, char * buffer)
{
	int response = partition->readCluster(cluster, buffer);
	if (response != 1) throw std::exception("Read failure");
}

void Swap::write(ClusterNo cluster, void * buffer)
{
	int response = partition->writeCluster(cluster, (char*)buffer);
	if (response != 1) throw std::exception("Write failure");
}

bool Swap::clustersAvailable()
{
	return used < partition->getNumOfClusters();
}

Swap::~Swap()
{
}

void Swap::claim(ClusterNo cluster)
{
	std::shared_ptr<char> buffer = std::shared_ptr<char>(new char[ClusterSize]);
	FreePage *fp = (FreePage*)buffer.get();
	fp->next = next;
	next = cluster;
	write(cluster, buffer.get());
}

void Swap::allocateAnother()
{
	claim(allocated++);
}
