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

	struct RoutingRule
	{
		Address address1, address3;
		uint64_t prevHopId, nextHopId;
		uint64_t Key;
		int messageCount;
		bool RunMAC;
		//int prevHopRRID, myRRID, nextHopRRID;
	};

	TorRouter(Address& mDirectory, bool malicious = false);
	~TorRouter();

	void ReceiveHand(std::string p, Address a);
	Address GetAddress();

	std::string Decrypt(std::string&, std::shared_ptr<RoutingRule>&);
	std::string Encrypt(std::string&, std::shared_ptr<RoutingRule>&, bool pad = false);

//private:


	std::shared_ptr<RoutingRule> MakeNewRoutingRule(Address& source, uint64_t sourceID, bool RunMAC);
	std::shared_ptr<RoutingRule> GetRoutingRule(uint64_t id);
	std::string WaitForResponse(uint64_t);
	std::string WaitForHttp(Address&);

	std::vector<std::shared_ptr<RoutingRule>> mRoutingRules;
	int nextForwardId = 1;
	Address mDirectory;
	Address mAddress;
	std::mt19937_64 randomNumberEngine;
	bool mMalicious;

	struct ResponseQueueItem
	{
		std::string cell;
		Address sender;
	};

	std::vector<ResponseQueueItem> mReceiveQueue;
};

