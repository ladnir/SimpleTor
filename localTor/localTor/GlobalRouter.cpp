#include "GlobalRouter.h"
#include <sstream>
#include <iostream>

int GlobalRouter::depth(0);

std::vector < Device* > GlobalRouter::mDevices;
int GlobalRouter::mNextIP = 0;

Address::IPv4 GlobalRouter::Register(Device& device)
{
	mDevices.push_back(&device);

	std::stringstream ip;
	ip << "192.168.1." << ++mNextIP;

	return Address::IPv4(ip.str());
}

void GlobalRouter::Send(std::string p, Address destination, Address source)
{
	for each (Device* device in mDevices)
	{
		if (device->GetAddress().mIPv4.compare(destination.mIPv4) == 0)
		{
			device->ReceiveHand(p, source);
			return;
		}
	}
	throw new std::exception;
}
std::string GlobalRouter::PrintDepth()
{
	std::string s;
	for (int i = 0; i < depth; i++)
	{
		s+= "\t";
	}
	return s;
}
