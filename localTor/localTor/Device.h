#pragma once

#include "Address.h"
#include <string>

class Device
{
public:

	virtual void ReceiveHand(std::string p, Address a) = 0;
	virtual Address GetAddress() = 0;
};

