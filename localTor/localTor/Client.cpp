#include "Client.h"
#include "GlobalRouter.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;


TorClient::TorClient(const Address& directory)
	: randomNumberEngine((uint64_t)this)
{
	mDirectory.mIPv4 = directory.mIPv4;
	mDirectory.port = directory.port;

	mAddress.mIPv4 = GlobalRouter::Register(*this); 

	mAddress.port = 333;
	mCurcuitLength = 10;

	GetRoutersFromDirectory();
}


TorClient::~TorClient()
{
}

std::string TorClient::GetRequest(Address a, string uri)
{

	Circuit cir = CreateCurcuit(a);

	stringstream request;

	request << "Relay:" << cir.mID << ":Data:GET " << a.mIPv4 << ":" << a.port << "HTTP/1.1";


	cout << request.str() << endl << "-------->" << endl;
	GlobalRouter::depth++;
	GlobalRouter::Send(request.str(), cir.mRouters[0], mAddress);
	GlobalRouter::depth++;

	// wait for the router to respond
	std::string response = WaitForResponse(cir.mRouters[0]);
	cout << "<-------" << endl;

	stringstream responseStream(response);
	std::string token;

	std::getline(responseStream, token, ':');
	if (token.compare("Relay") != 0)
		throw new std::exception;

	uint64_t id;
	char c;
	responseStream >> id;
	responseStream >> c;

	if (id != cir.mID)
		throw new std::exception;

	return responseStream.str();
}

Circuit TorClient::CreateCurcuit(Address destination)
{
	//if (mRouters.size() < mCurcuitLength) throw std::exception();
	Circuit cir;
	cir.mDestination = destination;

	int prev = -1;
	// TODO choose path randomly
	for (unsigned int i = 0; i < mCurcuitLength; i++)
	{
		int router = rand() % mRouters.size();

		if (prev == router)
		{
			i--;
			continue;
		}
		prev = router;
		cir.mRouters.push_back(mRouters[router]);
	}

	std::string create("Create");
	std::stringstream request;

	std::string word;

	std::uniform_int_distribution<uint64_t> uniform_dist;

	cir.mID = uniform_dist(randomNumberEngine);

	// Create the first contact
	request << create << ":" << cir.mID << ";";

	cout << request.str() << endl << "-------->" << endl;
	GlobalRouter::depth++;
	GlobalRouter::Send(request.str(), cir.mRouters[0], mAddress);
	GlobalRouter::depth--;

	// wait for the router to respond
	std::string response = WaitForResponse(cir.mRouters[0]);
	cout << "<---------" << endl << response << endl << endl << endl;
	std::stringstream responseStream(response);

	// check that it was created successfully
	std::getline(responseStream, word, ':');
	if (word.compare("Created") != 0) throw new std::exception;

	// double check that it echoed back our id for the link
	std::getline(responseStream, word, ';');
	if (word.compare("") == 0) throw new std::exception;
	if (cir.mID != std::stoull(word)) throw new std::exception;


	

	// make sure that was all that data that was sent
	word = "";
	responseStream >> word;
	if (word.length() != 0) throw new std::exception;

	// Cool, first connection made, now lets extend it

	for (int i = 1; i < mCurcuitLength; i++)
	{
		// create the extend command
		request.clear();
		request.str("");
		request << "Extend:" << cir.mID << ":" << cir.mRouters[i].mIPv4 << ":" << cir.mRouters[i].port << ";";

		cout << request.str() << endl << "-------->" << endl;
		GlobalRouter::depth++;
		GlobalRouter::Send(request.str(), cir.mRouters[0], mAddress);
		GlobalRouter::depth--;
		cout << "<-------" << endl;

		// wait for the router to respond
		response = WaitForResponse(cir.mRouters[0]);
		std::stringstream responseStream2(response);
		cout << response << endl;

		// check that it was created successfully
		std::getline(responseStream2, word, ':');
		if (word.compare("Extended") != 0) throw new std::exception;

		cout << endl << endl;
		//if (responseStream2.str().length() != 0) throw new std::exception;
	}

	request.clear();
	request.str("");
	request << "Relay:" << cir.mID << ":Begin:" << destination.mIPv4 << ":" << destination.port << ";";

	cout << request.str() << endl << "-------->" << endl;
	GlobalRouter::depth++;
	GlobalRouter::Send(request.str(), cir.mRouters[0], mAddress);
	GlobalRouter::depth--;
	cout << "<-------" << endl;

	// wait for the router to respond
	response = WaitForResponse(cir.mRouters[0]);
	cout << response << endl << endl;


	return cir;
}


void TorClient::GetRoutersFromDirectory()
{
	std::string request(std::string("Request"));
	GlobalRouter::Send(request, mDirectory, mAddress);

	std::string response = WaitForResponse(mDirectory);

	std::istringstream dataStream( response);
	std::string addr, portstr;

	while (std::getline(dataStream, addr, ':'))
	{
		std::getline(dataStream, portstr, ';');

		Address rt;
		rt.mIPv4 = addr;
		rt.port = std::stoi(portstr);

		mRouters.push_back(rt);
	}
}
Address TorClient::GetAddress()
{
	return mAddress;
}

void TorClient::ReceiveHand(std::string p, Address a)
{
	ResponseQueueItem response;
	response.cell = p;
	response.sender = a;

	mReceiveQueue.push(response);
}

std::string  TorClient::WaitForResponse(Address& on)
{
	ResponseQueueItem response = mReceiveQueue.back();
	mReceiveQueue.pop();

	if (response.sender.mIPv4.compare(on.mIPv4) != 0) throw std::exception();

	return response.cell;
}