#pragma once
#include <vector>
#include <string>
#include "Router.h"
#include "Device.h"

class Directory : public Device
{
public:
	Directory();
	~Directory();

	Address GetAddress();
	void ReceiveHand(std::string p, Address a);

private:
	std::vector<Address> mRouters;

	Address mAddress;
};

