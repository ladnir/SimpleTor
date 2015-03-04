#pragma once
#include <string>
#include "Device.h"

class HTTPServer : public Device
{
public:
	HTTPServer();
	~HTTPServer();

	void ReceiveHand(std::string p, Address a);
	Address GetAddress();

	Address mAddress;
};

