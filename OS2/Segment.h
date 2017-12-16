#pragma once
#include "vm_declarations.h"
#include<list>
#include<string>
class KernelSystem;
class KernelProcess;
class Segment
{
public:
	struct SegmentUser
	{
		KernelProcess *user;
		VirtualAddress start;
		explicit SegmentUser(KernelProcess *user, VirtualAddress start = 0)
			:start(start), user(user)
		{

		}
		bool operator==(const SegmentUser& other) const
		{
			return other.user == user;
		}
	};
	Segment(PageNum size, const char *name = nullptr);
	Segment(const Segment &s) = delete;
	Segment(Segment &&s) = delete;
	bool overlap(VirtualAddress start, PageNum size, VirtualAddress myStart);
	bool contains(PageNum page, VirtualAddress myStart);
	bool shared();
	~Segment();
	bool operator==(const char* other) const;
	friend class KernelSystem;
	friend class KernelProcess;
private:
	std::string name;
	std::list<SegmentUser> users;
	KernelProcess *dummyUser;
	//VirtualAddress start;
	PageNum size;
	int references = 0;
};

