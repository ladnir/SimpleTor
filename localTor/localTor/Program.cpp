
#include "Directory.h"
#include "Client.h"
#include "HTTPServer.h"
#include "Router.h"
#include <iostream>

int main(int argc, char** argv)
{
	Directory dir;

	TorRouter r1(dir.GetAddress());
	TorRouter r2(dir.GetAddress(), true);
	TorRouter r3(dir.GetAddress());
	TorRouter r4(dir.GetAddress(), true);
	TorRouter r5(dir.GetAddress());
	//TorRouter rr1(dir.GetAddress());
	//TorRouter rr2(dir.GetAddress());
	//TorRouter rr3(dir.GetAddress());
	//TorRouter rr4(dir.GetAddress());
	//TorRouter rr5(dir.GetAddress());

	TorClient c1(dir.GetAddress());

	HTTPServer server;

	//c1.Test();

	std::cout << std::endl << "================================" << std::endl << "Response:\n\n" << c1.GetRequest(server.mAddress, "http://www.someFakeSite.com");

	
}
