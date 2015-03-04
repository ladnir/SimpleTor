#pragma once
#include "Address.h"
#include "Device.h"
#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <cstdint>
#include <random>

class TorRouter : public Device
{
public:
	TorRouter(Address& mDirectory);
	~TorRouter();

	void ReceiveHand(std::string p, Address a);
	Address GetAddress();

private:
	struct RoutingRule
	{
		Address address1, address3;
		uint64_t prevHopId, nextHopId;
		//int prevHopRRID, myRRID, nextHopRRID;
	}; 

	std::shared_ptr<RoutingRule> MakeNewRoutingRule(Address& source, uint64_t sourceID);
	std::shared_ptr<RoutingRule> GetRoutingRule(uint64_t id);
	std::string WaitForResponse(uint64_t);
	std::string WaitForHttp(Address&);

	std::vector<std::shared_ptr<RoutingRule>> mRoutingRules;
	int nextForwardId = 1;
	Address mDirectory;
	Address mAddress;
	std::mt19937_64 randomNumberEngine;

	struct ResponseQueueItem
	{
		std::string cell;
		Address sender;
	};

	std::vector<ResponseQueueItem> mReceiveQueue;
};

