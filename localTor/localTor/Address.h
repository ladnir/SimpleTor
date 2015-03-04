#pragma once

#include <string>

class Address
{

public:
	typedef std::string IPv4;
	typedef int Port;

	IPv4 mIPv4;
	Port port;
};

