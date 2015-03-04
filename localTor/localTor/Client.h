#pragma once
#include <string>
#include <vector>
#include <queue>
#include "Circuit.h"
#include "Address.h"
#include "Router.h"
#include "Device.h"

class TorClient : public Device
{
public:
	TorClient(const Address& directory);
	~TorClient();

	std::string GetRequest(Address a, std::string uri);

	void ReceiveHand(std::string p, Address a);
	Address GetAddress();


private:

	void GetRoutersFromDirectory();
	std::string WaitForResponse(Address&);

	Circuit CreateCurcuit(Address destination);


	unsigned int mCurcuitLength;
	Address mDirectory;
	Address mAddress;


	std::mt19937_64 randomNumberEngine;
	std::vector<Circuit> mCurcuits;
	std::vector<Address> mRouters;

	struct ResponseQueueItem
	{
		std::string cell;
		Address sender;
	};

	std::queue<ResponseQueueItem> mReceiveQueue;

};

