#pragma once
#include <vector>
#include "Router.h"
#include "Address.h"

class Circuit
{
public:
	Circuit();
	~Circuit();

	std::vector<Address> mRouters;
	std::vector<uint64_t> mKeys;
	std::vector<int> mMessageCounts;

	uint64_t mID;

	Address mDestination;
};

