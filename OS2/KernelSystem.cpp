#include "KernelSystem.h"



KernelSystem::KernelSystem()
{
}

void KernelSystem::createProcess()
{
	std::lock_guard<std::mutex> _guard(guard);
}


KernelSystem::~KernelSystem()
{
}

std::mutex & KernelSystem::getMutex()
{
	return guard;
}
