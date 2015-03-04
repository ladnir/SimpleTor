#include "Router.h"
#include "GlobalRouter.h"
#include <sstream>
#include <iostream>
#include <random>

TorRouter::TorRouter(Address& directory)
	:randomNumberEngine((uint64_t)this)
{
	mDirectory = directory;
    mAddress.mIPv4 = GlobalRouter::Register(*this);
	mAddress.port = 444;

	std::stringstream registerStream;
	registerStream << "Register" << ":" << mAddress.port << ";";


	GlobalRouter::Send(registerStream.str(), mDirectory, mAddress);


}


TorRouter::~TorRouter()
{
}

void TorRouter::ReceiveHand(std::string p, Address a)
{
	
	std::stringstream dataStream(p);
	std::string command;		
	//Address forwardAddr;



	std::getline(dataStream, command, ':');

	if (command.compare("Create") == 0)
	{
		std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

		std::stringstream responseStream;

		std::string sourceIDstr;
		std::getline(dataStream, sourceIDstr, ';');
		uint64_t id = std::stoull(sourceIDstr);


		// make a new routing rule for it
		auto routingID = MakeNewRoutingRule(a, id);

		// tell them that it was created, echo back their id and that this router will use.
		responseStream  << "Created" <<":" << id <<  ";";

		//std::cout << GlobalRouter::PrintDepth() << responseStream.str() << std::endl;
		GlobalRouter::Send(responseStream.str(), a, mAddress);
	}
	else if (command.compare("Extend") == 0)
	{
		std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

		std::string token;
		std::getline(dataStream, token, ':');
		uint64_t id = std::stoull(token);

		std::shared_ptr<TorRouter::RoutingRule> rr = GetRoutingRule(id);

		if (rr->nextHopId == -1)
		{	// this means we are extending this routing rule
			std::stringstream request;

			// parse the next ip and port values
			std::getline(dataStream, rr->address3.mIPv4, ':');
			std::getline(dataStream, token, ';');
			rr->address3.port = std::stoi(token);

			if (rr->address3.mIPv4.compare(mAddress.mIPv4) == 0)
				throw new std::exception;


			std::uniform_int_distribution<uint64_t> uniform_dist;
			rr->nextHopId = uniform_dist(randomNumberEngine);

			// Create the next contact
			request << "Create" << ":" << rr->nextHopId << ";";

			std::cout << GlobalRouter::PrintDepth() << request.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
			GlobalRouter::depth++;
			GlobalRouter::Send(request.str(), rr->address3, mAddress);
			GlobalRouter::depth--;

			// wait for the router to respond
			std::string response = WaitForResponse(rr->nextHopId);
			std::cout << GlobalRouter::PrintDepth() << "<--------" << std::endl << GlobalRouter::PrintDepth() << response << std::endl;
			std::stringstream dataStream(response);

			// check that it was created successfully
			std::getline(dataStream, token, ':');
			if (token.compare("Created") != 0) throw new std::exception;

			// double check that it echoed back our id for the link
			std::getline(dataStream, token, ':');
			if (token.compare("") == 0) throw new std::exception;
			rr->nextHopId = std::stoull(token);

			// see that id it assigned it
			std::getline(dataStream, token, ';');

			// make sure that was all that data that was sent
			token = "";
			dataStream >> token;
			if (token.length() != 0) throw new std::exception;
		}
		else
		{	// this mean the extention is somewhere down the circuit

			std::string ipPort;
			std::getline(dataStream, ipPort, ';');

			std::stringstream request;
			request << "Extend:" << rr->nextHopId << ":" << ipPort << ";";

			// forward to the next contact
			std::cout << GlobalRouter::PrintDepth() << request.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
			GlobalRouter::depth++;
			GlobalRouter::Send(request.str(), rr->address3, mAddress);
			GlobalRouter::depth--;

			// wait for the router to respond
			std::string response = WaitForResponse(rr->nextHopId);
			std::cout << GlobalRouter::PrintDepth() << "<--------" << std::endl << GlobalRouter::PrintDepth() << response << std::endl;

			std::stringstream dataStream(response);

			// check that it was created successfully
			std::getline(dataStream, token, ':');
			if (token.compare("Extended") != 0) 
				throw new std::exception;
			

		}

		std::stringstream responseStream;
		responseStream << "Extended:" << rr->prevHopId << ";";

		//std::cout << GlobalRouter::PrintDepth() << responseStream.str() << std::endl;
		GlobalRouter::Send(responseStream.str(), a, mAddress);
	}
	else if (command.compare("Created") == 0 || command.compare("Extended") == 0)
	{
		ResponseQueueItem response;

		response.cell = p;
		response.sender = a;

		mReceiveQueue.push_back(response);
	}
	else if (command.compare("Relay") == 0)
	{
		
		uint64_t id;
		dataStream >> id;

		std::shared_ptr<TorRouter::RoutingRule> rr = GetRoutingRule(id);

		std::stringstream response;
		char c;
		dataStream >> c;
		std::string token = dataStream.str().substr(dataStream.tellg(), 5);

		if (rr->nextHopId == -1){
			std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

			if (token.compare("Begin") == 0)
			{

				std::getline(dataStream, token, ':');

				char c;
				std::string token;

				std::getline(dataStream, rr->address3.mIPv4, ':');


				std::getline(dataStream, token, ';');
				rr->address3.port = std::stoi(token);


				// add TCP/socket handshake heres...

				//std::cout << GlobalRouter::PrintDepth() << "Connected" << std::endl;

				std::stringstream response;
				response << "Relay:" << rr->prevHopId << ":Data:Connected;";
				//GlobalRouter::depth--;
				//std::cout << GlobalRouter::PrintDepth() << response.str() << std::endl << GlobalRouter::PrintDepth() << "------->"  << std::endl;
				
				GlobalRouter::Send(response.str(), rr->address1, mAddress);


			}
			else if (token.compare("Data:") == 0)
			{
				std::getline(dataStream, token, ':');


				if (rr->address3.mIPv4.compare("") == 0)
					throw new std::exception;

				std::string message = dataStream.str().substr(dataStream.tellg());

				std::cout << GlobalRouter::PrintDepth() << message << std::endl;

				GlobalRouter::depth++;
				GlobalRouter::Send(message, rr->address3, mAddress);
				GlobalRouter::depth--;

				std::string httpResponse = WaitForHttp(rr->address3);
				std::stringstream clientResponse;

				clientResponse << "Relay:" << rr->prevHopId << ":" << httpResponse;
				GlobalRouter::Send(clientResponse.str(), rr->address1, mAddress);
			}
		}
		else
		{
			if (rr->prevHopId == id)
			{
				std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

				response << "Relay:" << rr->nextHopId << ":" << dataStream.str().substr(dataStream.tellg());

				std::cout << GlobalRouter::PrintDepth() << response.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
				GlobalRouter::depth++;
				GlobalRouter::Send(response.str(), rr->address3, mAddress);
			}
			else
			{
				response << "Relay:" << rr->prevHopId << ":" << dataStream.str().substr(dataStream.tellg());

				GlobalRouter::depth--;
				std::cout << GlobalRouter::PrintDepth() << "<-------" << std::endl << GlobalRouter::PrintDepth() << dataStream.str() << std::endl;
				GlobalRouter::Send(response.str(), rr->address1, mAddress);
			}
		}


	}
	else if (p.substr(0, 4).compare("HTTP") == 0)
	{
		ResponseQueueItem response;

		response.cell = p;
		response.sender = a;

		mReceiveQueue.push_back(response);
	}
	else 
		throw new std::exception;
}

std::shared_ptr<TorRouter::RoutingRule> TorRouter::MakeNewRoutingRule(Address& source, uint64_t sourceID)
{

	

	for (int i = 0; i < mRoutingRules.size(); i++)
	{
		if (mRoutingRules[i]->prevHopId == sourceID ||
			mRoutingRules[i]->nextHopId == sourceID)
			throw new std::exception;
	}

	std::shared_ptr<RoutingRule> fr = std::shared_ptr<RoutingRule>(new RoutingRule);
	fr->address1 = source;

	fr->prevHopId = sourceID;
	fr->nextHopId = -1;// uniform_dist(randomNumberEngine);
	//fr->nextHopRRID = -1;
	fr->address3.mIPv4 = "";
	fr->address3.port = 0;

	mRoutingRules.push_back(fr);

	return fr;
}


std::shared_ptr<TorRouter::RoutingRule> TorRouter::GetRoutingRule(uint64_t sourceID)
{

	for (int i = 0; i < mRoutingRules.size(); i++)
	{
		if (mRoutingRules[i]->prevHopId == sourceID ||
			mRoutingRules[i]->nextHopId == sourceID)
			return mRoutingRules[i];
	}

	throw new std::exception;
}

Address TorRouter::GetAddress()
{
	return mAddress;
}

std::string  TorRouter::WaitForHttp(Address& addr)
{
	//ResponseQueueItem response = mReceiveQueue.back();
	//mReceiveQueue.pop();

	for (int i = 0; i < mReceiveQueue.size(); i++)
	{
		if (mReceiveQueue[i].cell.substr(0, 4).compare("HTTP") == 0)
		{
			if (mReceiveQueue[i].sender.mIPv4.compare(addr.mIPv4) == 0)
			{
				std::string t = mReceiveQueue[i].cell;

				mReceiveQueue[i] = mReceiveQueue.back();
				mReceiveQueue.pop_back();

				return t;
			}
		}
	}

	throw new std::exception;
}
std::string  TorRouter::WaitForResponse(uint64_t responseId)
{
	//ResponseQueueItem response = mReceiveQueue.back();
	//mReceiveQueue.pop();

	for (int i = 0; i < mReceiveQueue.size(); i++)
	{
		if (mReceiveQueue[i].cell.substr(0, 4).compare("HTTP") == 0)
			continue;

		std::string s;
		std::stringstream  ss(mReceiveQueue[i].cell);
		std::getline(ss, s, ':');

		uint64_t id;
		ss >> id;

		if (id == responseId)
		{
			std::string t = mReceiveQueue[i].cell;

			mReceiveQueue[i] = mReceiveQueue.back();
			mReceiveQueue.pop_back();

			return t;
		}
	}

	//if (response.sender.mIPv4.compare(on.mIPv4) != 0) throw std::exception();
	throw new std::exception;
	//return response.cell;
}