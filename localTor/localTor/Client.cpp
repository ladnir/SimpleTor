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
	stringstream plaintextStream;
	plaintextStream << "Data:GET " << a.mIPv4 << " : " << a.port << "HTTP / 1.1";


	request << "Relay:" << cir.mID << ":" << Encrypt(plaintextStream.str(), cir) << ";";


	
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

	int start = responseStream.tellg();
	int length = responseStream.str().length() - start - 1;
	token = responseStream.str().substr(start, length);

	std::string plaintext = Decrypt(token, cir);

	return plaintext;
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
	uint64_t X =  uniform_dist(randomNumberEngine);

	// Create the first contact
	request << create << ":" << cir.mID << ":" << (char) 1 << ":" << 3 * X << ";";


	
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
	std::getline(responseStream, word, ':');
	if (word.compare("") == 0) throw new std::exception;
	if (cir.mID != std::stoull(word)) throw new std::exception;


	std::getline(responseStream, word, ';');
	if (word.compare("") == 0) throw new std::exception;
	cir.mKeys.push_back(X *  std::stoull(word));
	cir.mMessageCounts.push_back(0);


	cout << "key " << cir.mKeys.back() << endl << endl;
	

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
		X = uniform_dist(randomNumberEngine);

		char RunMAC = (i + 2 == mCurcuitLength) ? 2 : 1;

		std::stringstream plaintextstream;
		plaintextstream << cir.mRouters[i].mIPv4 << ":" << cir.mRouters[i].port << ":" << (3 * X);
		std::string cyphertext = Encrypt(plaintextstream.str(), cir);

		request << "Extend:" << cir.mID << ":" << RunMAC << ":" << cyphertext << ";";

		
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

		// double check that it echoed back our id for the link
		std::getline(responseStream2, word, ':');
		if (word.compare("") == 0) throw new std::exception;
		if (cir.mID != std::stoull(word)) throw new std::exception;


		// get the key
		int start = responseStream2.tellg();
		int length = responseStream2.str().length() - start - 1;
		word = responseStream2.str().substr(start, length);
		if (word.compare("") == 0) throw new std::exception;
		word = Decrypt(word, cir);


		cir.mKeys.push_back(X *  std::stoull(word));
		cir.mMessageCounts.push_back(0);
	

		cout << "key " << cir.mKeys.back() << endl << endl;
		//if (responseStream2.str().length() != 0) throw new std::exception;
	}

	request.clear();
	request.str("");
	std::stringstream plaintextstream;
	plaintextstream << "Begin:" << destination.mIPv4 << ":" << destination.port;
	std::string cyphertext = Encrypt(plaintextstream.str(), cir);

	request << "Relay:" << cir.mID << ":" << cyphertext  << ";";


	
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
	if (word.compare("Relay") != 0) throw new std::exception;

	// double check that it echoed back our id for the link
	std::getline(responseStream2, word, ':');
	if (word.compare("") == 0) throw new std::exception;
	if (cir.mID != std::stoull(word)) throw new std::exception;


	// get the key
	int start = responseStream2.tellg();
	int length = responseStream2.str().length() - start - 1;
	word = responseStream2.str().substr(start, length);
	if (word.compare("") == 0) throw new std::exception;
	word = Decrypt(word, cir);

	if (word.substr(0,9).compare("Connected") != 0 )
		throw new std::exception;

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
//#define bracketEncryption

std::string TorClient::Encrypt(std::string& plaintext, Circuit& cir)
{
#ifdef bracketEncryption
	stringstream prefix, sufix;
	sufix << plaintext;

	for (int i = cir.mKeys.size() - 1; i >= 0; i--)
	{
		prefix << "{";

		sufix << "}" << cir.mMessageCounts[i]++;
	}

	return prefix.str() + sufix.str();
#else

	int length = plaintext.length() / 8 ;
	//int length = plaintext.length() + (8 - plaintext.length() % 8);

	int mod = plaintext.length() % 8;
	int extra = (mod > 0) ? 1 : 0;
	length = length + extra;
	length = length * 8;

	vector<char> chars(length);

	for (int i = 0; i < plaintext.length(); i++)
	{
		chars[i] = plaintext[i];
	}

	

	for (int i = cir.mKeys.size() - 1; i >= 0; i--)
	{
		uint64_t sessionKey = (cir.mMessageCounts[i]++ + 1) * cir.mKeys[i];
		if (i + 2 == cir.mRouters.size())
		{
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

		for (int j = 0; j < chars.size(); j += 8)
		{
			uint64_t* ptr = (uint64_t*)& chars[j];

			*ptr = *ptr ^ sessionKey;
		}

	}
	stringstream cyphertext;


	for (int i = 0; i < chars.size(); i++)
	{
		cyphertext << chars[i];
	}

	return cyphertext.str();
#endif
}

std::string TorClient::Decrypt(std::string& cyphertext, Circuit& cir)
{

#ifdef bracketEncryption
	int start = 0, end = cyphertext.length();
	for (int i = 0; i < cir.mKeys.size(); i++)
	{
		if (cyphertext[i] != '{')
			throw new std::exception;
		start++;

		std::stringstream sufixstream;
		sufixstream << "}" << cir.mMessageCounts[i]++;
		std::string sufix(sufixstream.str());

		std::string s = cyphertext.substr(end - sufix.length(), sufix.length());
		if (s.compare(sufix) != 0)
			throw new std::exception;

		end -= sufix.length();
	}
	auto plaintext = cyphertext.substr(start, end - start);
	return plaintext;
#else

	if (cyphertext.length() % 8 > 0)
		throw new std::exception;

	vector<char> chars(cyphertext.length());

	for (int i = 0; i < cyphertext.length(); i++)
	{
		chars[i] = cyphertext[i];
	}

	for (int i = 0; i < cir.mKeys.size() ; i++)
	{
		cout << "decrypting with   mc =" << cir.mMessageCounts[i] << " and key =" << cir.mKeys[i] << "  len=" << chars.size() << endl;

		uint64_t sessionKey = (cir.mMessageCounts[i]++ + 1 ) * cir.mKeys[i];

		for (int j = 0; j < chars.size(); j += 8)
		{
			uint64_t* ptr = (uint64_t*)& chars[j];

			*ptr = *ptr ^ sessionKey;
		}

		if (i + 2 == cir.mRouters.size())
		{
			uint64_t MAC = *(uint64_t*)& chars[chars.size() - 8];

			for (int j = 0; j < chars.size() - 8; j += 8)
			{
				uint64_t* ptr = (uint64_t*)& chars[j];
				MAC = MAC ^ *ptr;
			}

			if (MAC != 0)
				throw new std::exception;

			for (int k = 0; k < 8; k++)
			{

				//if (chars.back() != 'e')
				//{
				//	throw new std::exception;
				//}

				chars.pop_back();
			}
		}
	}

	stringstream plaintext;
	for (int i = 0; i < chars.size(); i++)
	{
		plaintext << chars[i];
	}

	return plaintext.str();


#endif
}

void TorClient::Test()
{
	

	Circuit cir;
	TorRouter tr(mDirectory);

	vector<shared_ptr<TorRouter::RoutingRule>> routers;

	for (int i = 0; i < 10; i++)
	{
		routers.emplace_back(new TorRouter::RoutingRule);

		cir.mKeys.push_back((uint64_t)9341435233413 * (i + 1));
		routers[i]->Key = cir.mKeys[i];

		routers.back()->messageCount = 5 + 2 * i;
		cir.mMessageCounts.push_back( 5 + 2 * i);
	}

	string plain("hello workd :)      ee ff");

	string cyphertext = Encrypt(plain, cir);

	for (int i = 0; i < 10; i++)
	{
		cyphertext = tr.Decrypt(cyphertext, routers[i]);

	}

	if (cyphertext.substr(0,plain.length()).compare(plain) != 0)
		throw new std::exception;

	string plain2 = Decrypt(cyphertext, cir);

}