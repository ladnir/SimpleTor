#include "Router.h"
#include "GlobalRouter.h"
#include <sstream>
#include <iostream>
#include <random>

TorRouter::TorRouter(Address& directory, bool malicious)
	:randomNumberEngine((uint64_t)this)
{
	mDirectory = directory;
    mAddress.mIPv4 = GlobalRouter::Register(*this);
	mAddress.port = 444;
	mMalicious = malicious;
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
		std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl;

		std::stringstream responseStream;

		std::string token;
		std::getline(dataStream, token, ':');
		uint64_t id = std::stoull(token);
		
		
		std::getline(dataStream, token, ':');
		bool RunMAC = (token.compare("\x2") == 0);

		// make a new routing rule for it
		auto rr = MakeNewRoutingRule(a, id, RunMAC);

		std::uniform_int_distribution<uint64_t> uniform_dist;
		uint64_t keyY = uniform_dist(randomNumberEngine);


		std::getline(dataStream, token, ';');
		rr->Key = std::stoull(token) * keyY;
		std::cout << GlobalRouter::PrintDepth() << "key " << rr->Key << std::endl << std::endl;
		
		// tell them that it was created, echo back their id and that this router will use.
		responseStream << "Created" << ":" << id << ":" << 3 * keyY << ";";

		//std::cout << GlobalRouter::PrintDepth() << responseStream.str() << std::endl;
		GlobalRouter::Send(responseStream.str(), a, mAddress);
	}
	else if (command.compare("Extend") == 0)
	{
		std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

		std::string token;
		std::getline(dataStream, token, ':');
		uint64_t id = std::stoull(token);
		std::stringstream responseStream;

		std::shared_ptr<TorRouter::RoutingRule> rr = GetRoutingRule(id);

		if (rr->nextHopId == -1)
		{	// this means we are extending this routing rule

			std::getline(dataStream, token, ':');
			char RunMAC = token[0];


			int start = dataStream.tellg();
			int length = dataStream.str().length() - start - 1;

			std::string cyphertext = dataStream.str().substr(start, length);
			std::string plaintext( Decrypt(cyphertext, rr));

			std::stringstream request;
			std::stringstream plaintextStream(plaintext);


			// parse the next ip and port values
			std::getline(plaintextStream, rr->address3.mIPv4, ':');
			std::getline(plaintextStream, token, ':');
			rr->address3.port = std::stoi(token);

			token = "";
			plaintextStream >> token;
			uint64_t keyGX = std::stoull(token);

			if (rr->address3.mIPv4.compare(mAddress.mIPv4) == 0)
				throw new std::exception;


			std::uniform_int_distribution<uint64_t> uniform_dist;
			rr->nextHopId = uniform_dist(randomNumberEngine);

			// Create the next contact
			request << "Create" << ":" << rr->nextHopId << ":" << RunMAC << ":" << keyGX << ";";

			std::cout << GlobalRouter::PrintDepth() << request.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
			GlobalRouter::depth++;
			GlobalRouter::Send(request.str(), rr->address3, mAddress);
			GlobalRouter::depth--;

			// wait for the router to respond
			std::string response = WaitForResponse(rr->nextHopId);
			std::cout << GlobalRouter::PrintDepth() << "<--------" << std::endl << GlobalRouter::PrintDepth() << response << std::endl;
			std::stringstream dataStream2(response);

			// check that it was created successfully
			std::getline(dataStream2, token, ':');
			if (token.compare("Created") != 0) throw new std::exception;

			// double check that it echoed back our id for the link
			std::getline(dataStream2, token, ':');
			if (token.compare("") == 0) throw new std::exception;
			if (rr->nextHopId != std::stoull(token)) throw new std::exception;

			start = dataStream2.tellg();
			length = dataStream2.str().length() - start - 1;
			token = dataStream2.str().substr(start, length);
			token = Encrypt(token, rr, true);

			// make sure that was all that data that was sent
			responseStream << "Extended:" << rr->prevHopId << ":" << token << ";";
		}
		else
		{	// this mean the extention is somewhere down the circuit
			std::getline(dataStream, token, ':');
			char RunMAC = token[0];

			int start = dataStream.tellg();
			int length = dataStream.str().length() - start - 1;
			std::string ipPortKeyGX = dataStream.str().substr(start, length);
			//std::getline(dataStream, ipPortKeyGX, ';');

			ipPortKeyGX = Decrypt(ipPortKeyGX, rr);

			std::stringstream request;
			request << "Extend:" << rr->nextHopId << ":" << RunMAC << ":" << ipPortKeyGX << ";";

			// forward to the next contact
			std::cout << GlobalRouter::PrintDepth() << request.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
			GlobalRouter::depth++;
			GlobalRouter::Send(request.str(), rr->address3, mAddress);
			GlobalRouter::depth--;

			// wait for the router to respond
			std::string response = WaitForResponse(rr->nextHopId);
			std::cout << GlobalRouter::PrintDepth() << "<--------" << std::endl << GlobalRouter::PrintDepth() << response << std::endl;

			std::stringstream dataStream2(response);

			// check that it was created successfully
			std::getline(dataStream2, token, ':');
			if (token.compare("Extended") != 0) 
				throw new std::exception;


			std::getline(dataStream2, token, ':');
			if (rr->nextHopId != std::stoull(token))
				throw new std::exception;




			start = dataStream2.tellg();
			length = dataStream2.str().length() - start - 1;
			token = dataStream2.str().substr(start, length);
			std::string cyphertext = Encrypt(token, rr);

			responseStream << "Extended:" << rr->prevHopId << ":" << cyphertext << ";";

		}


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
		
		std::stringstream response;
		uint64_t id;
		char c;

		dataStream >> id;
		dataStream >> c;
		std::shared_ptr<TorRouter::RoutingRule> rr = GetRoutingRule(id);



		int start = dataStream.tellg();
		int length = dataStream.str().length() - start - 1;
		std::string cyphertext = dataStream.str().substr(start, length);

		if (rr->prevHopId == id)
		{
			std::string plaintext(Decrypt(cyphertext, rr));

			if (rr->nextHopId == -1){
				std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;

				if (plaintext.substr(0, 5).compare("Begin") == 0)
				{
					std::string token;
					std::stringstream plaintextStream(plaintext);
					std::getline(plaintextStream, token, ':');


					std::getline(plaintextStream, rr->address3.mIPv4, ':');


					std::getline(plaintextStream, token, ';');
					rr->address3.port = std::stoi(token);


					// add TCP/socket handshake heres...


					std::stringstream response;
					response << "Relay:" << rr->prevHopId << ":" << Encrypt(std::string("Connected"), rr, true) << ";";

					GlobalRouter::Send(response.str(), rr->address1, mAddress);


				}
				else if (plaintext.substr(0, 5).compare("Data:") == 0)
				{

					if (rr->address3.mIPv4.compare("") == 0)
						throw new std::exception;

					std::string message = plaintext.substr(5);

					std::cout << GlobalRouter::PrintDepth() << message << std::endl;

					GlobalRouter::depth++;
					GlobalRouter::Send(message, rr->address3, mAddress);
					GlobalRouter::depth--;

					std::string httpResponse = WaitForHttp(rr->address3);
					std::stringstream clientResponse;

					clientResponse << "Relay:" << rr->prevHopId << ":" << Encrypt(httpResponse, rr, true) << ";";
					GlobalRouter::Send(clientResponse.str(), rr->address1, mAddress);
				}
			}
			else
			{
				std::cout << std::endl << GlobalRouter::PrintDepth() << "@ " << mAddress.mIPv4 << std::endl << std::endl;
			
				if (mMalicious)
				{
					plaintext[plaintext.length() / 2]++;
				}

				response << "Relay:" << rr->nextHopId << ":" << plaintext << ";";

				std::cout << GlobalRouter::PrintDepth() << response.str() << std::endl << GlobalRouter::PrintDepth() << "------->" << std::endl;
				GlobalRouter::depth++;
				GlobalRouter::Send(response.str(), rr->address3, mAddress);
			}
		}
		else
		{
			response << "Relay:" << rr->prevHopId << ":" << Encrypt(cyphertext, rr) << ";";

			GlobalRouter::depth--;
			std::cout << GlobalRouter::PrintDepth() << "<-------" << std::endl << GlobalRouter::PrintDepth() << response.str() << std::endl;
			GlobalRouter::Send(response.str(), rr->address1, mAddress);
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

std::shared_ptr<TorRouter::RoutingRule> TorRouter::MakeNewRoutingRule(Address& source, uint64_t sourceID, bool RunMAC)
{

	

	for (int i = 0; i < mRoutingRules.size(); i++)
	{
		if (mRoutingRules[i]->prevHopId == sourceID ||
			mRoutingRules[i]->nextHopId == sourceID)
			throw new std::exception;
	}

	std::shared_ptr<RoutingRule> fr = std::shared_ptr<RoutingRule>(new RoutingRule);
	fr->address1 = source;
	fr->messageCount = 0;
	fr->prevHopId = sourceID;
	fr->nextHopId = -1;// uniform_dist(randomNumberEngine);
	fr->RunMAC = RunMAC;
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


std::string TorRouter::Encrypt(std::string& plaintext, std::shared_ptr<TorRouter::RoutingRule>& rr, bool pad)
{

#ifdef bracketEncryption
	std::stringstream cyphertext;
	cyphertext << "{" << plaintext << "}" << rr->messageCount++;

	return cyphertext.str();
#else


	int length = plaintext.length() / 8;
	//int length = plaintext.length() + (8 - plaintext.length() % 8);

	int mod = plaintext.length() % 8;
	int extra = (mod > 0) ? 1 : 0;

	length = length + extra;
	length = length * 8;

	if (!pad && length != plaintext.length())
		throw new std::exception;

	std::vector<char> chars(length);

	for (int i = 0; i < plaintext.length(); i++)
	{
		chars[i] = plaintext[i];
	}


	if (rr->RunMAC)
	{
		//for (int j = 0; j < 8; j++)
		//{
		//	chars.push_back('e');
		//}
		uint64_t MAC = 0;
		for (int j = 0; j < chars.size(); j += 8)
		{
			uint64_t* ptr = (uint64_t*)& chars[j];
			MAC = MAC ^ *ptr;
		}

		for (int j = 0; j < 8; j++)
		{
			chars.push_back(((char*)&MAC)[j]);
		}
		
	}
	//if (chars.size() < 32)
	//	std::cout << "oooo no" << std::endl;

	std::cout << GlobalRouter::PrintDepth() << "encrypting with mc =" << rr->messageCount << " and key =" << rr->Key << "  len=" << chars.size() << std::endl;
	uint64_t sessionKey = (rr->messageCount++ + 1) * rr->Key;
	for (int j = 0; j < chars.size(); j += 8)
	{
		uint64_t* ptr = (uint64_t*)& chars[j];

		*ptr = *ptr ^ sessionKey;
	}


	std::stringstream cyphertext;


	for (int i = 0; i < chars.size(); i++)
	{
		cyphertext << chars[i];
	}

	return cyphertext.str();

#endif
}

std::string TorRouter::Decrypt(std::string& cyphertext, std::shared_ptr<TorRouter::RoutingRule>& rr)
{
#ifdef bracketEncryption

	int start = 1, end = cyphertext.length();

	if (cyphertext[0] != '{')
		throw new std::exception;

	//if (cyphertext[cyphertext.length() - 1] != '}')
	//	throw new std::exception;
	
	std::stringstream sufixstream;
	sufixstream << "}" << rr->messageCount++;
	std::string sufix(sufixstream.str());
	end -= sufix.length();

	std::string s = cyphertext.substr(cyphertext.length() - sufix.length(), sufix.length());
	if (s.compare(sufix) != 0)
		throw new std::exception;

	auto plaintext = cyphertext.substr(start, end - start);
	return plaintext;
#else
	if (cyphertext.length() % 8 > 0)
		throw new std::exception;

	std::vector<char> chars(cyphertext.length());

	for (int i = 0; i < cyphertext.length(); i++)
	{
		chars[i] = cyphertext[i];
	}


	uint64_t sessionKey = (rr->messageCount++ + 1) * rr->Key;

	for (int j = 0; j < chars.size(); j += 8)
	{
		uint64_t* ptr = (uint64_t*)& chars[j];

		*ptr = *ptr ^ sessionKey;
	}		
	
	if (rr->RunMAC)
	{
		uint64_t MAC = *(uint64_t*)& chars[chars.size() - 8];

		for (int j = 0; j < chars.size() - 8; j += 8)
		{
			uint64_t* ptr = (uint64_t*)& chars[j];
			MAC = MAC ^ *ptr;
		}

		if (MAC != 0)
		{
			std::cout << "ABORT:   Bad MAC found!!!!!!!!!!!!!!!! " << std::endl;
			throw new std::exception;
		}

		for (int k = 0; k < 8; k++)
		{
			chars.pop_back();
		}

		//for (int j = 0; j < 8; j++)
		//{
		//	if (chars.back() != 'e')
		//	{
		//		throw new std::exception;
		//	}

		//	chars.pop_back();
		//}
	}
	

	std::stringstream plaintext;
	for (int i = 0; i < chars.size(); i++)
	{
		plaintext << chars[i];
	}

	return plaintext.str();


#endif
}