#include "Directory.h"
#include "GlobalRouter.h"
#include <sstream>

Directory::Directory()
{

	mAddress.mIPv4 = GlobalRouter::Register(*this);
	mAddress.port = 222;
}


Directory::~Directory()
{
}

Address Directory::GetAddress()
{
	return mAddress;
}

void Directory::ReceiveHand(std::string p, Address a)
{
	std::string reg = "Register";
	std::string request = "Request";

	std::stringstream dataStream(p);
	std::string command;
	std::getline(dataStream, command, ':');

	if (command.compare(reg) == 0)
	{
		mRouters.push_back(a);
	}
	else if (command.compare(request) == 0)
	{
		std::stringstream responseString;

		for each (auto router in mRouters)
		{
			responseString << router.mIPv4 << ":" << router.port << ";";
		}
		
		GlobalRouter::Send(responseString.str(), a, mAddress);
	}
	else
	{
		throw new std::exception();
	}

}