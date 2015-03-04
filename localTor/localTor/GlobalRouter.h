#pragma once
#include "Address.h"
#include "TorCell.h"
#include "Device.h"
#include <vector>
#include <string>

class GlobalRouter
{
public:

	static Address::IPv4 Register(Device&);

	static void Send(std::string p, Address destination, Address source);


	static std::vector < Device* > mDevices;
	static int mNextIP;

	static std::string PrintDepth();
	static int depth;
};