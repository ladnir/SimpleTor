#include "HTTPServer.h"
#include "GlobalRouter.h"

HTTPServer::HTTPServer()
{
	mAddress.mIPv4 = GlobalRouter::Register(*this);
	mAddress.port = 80;
}


HTTPServer::~HTTPServer()
{
}


void HTTPServer::ReceiveHand(std::string p, Address a)
{

	p = "HTTP/1.0 200 OK\n\n<html><body>hello there</body></html>";
	GlobalRouter::Send(p, a, mAddress);
}

Address HTTPServer::GetAddress()
{
	return mAddress;
}
