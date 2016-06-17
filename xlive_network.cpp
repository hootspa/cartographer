#include "stdafx.h"
#include "CUser.h"
#include "resource.h"
#include <iostream>
#include <sstream>
#include <time.h>
#include <thread>
#include "h2mod.h"
#include <mutex>
#include "h2mod.h"
#include "Globals.h"

extern ULONG broadcast_server;

SOCKET boundsock = INVALID_SOCKET;
SOCKET game_sock = INVALID_SOCKET;
SOCKET game_sock_1000 = INVALID_SOCKET;


CUserManagement User;
clock_t begin, end;
double time_spent;
char* ServerStatus;
int MasterState = 0;

static LARGE_INTEGER timerFreq;
static LARGE_INTEGER counterAtStart;

float getElapsedNetworkTime(void) {
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	return (float)((c.QuadPart - counterAtStart.QuadPart) * 1000.0f / (float)timerFreq.QuadPart);
}

// #5310: XOnlineStartup
int WINAPI XOnlineStartup()
{
	//TRACE("XOnlineStartup");
	ServerStatus = new char[250];
	QueryPerformanceFrequency(&timerFreq);
	QueryPerformanceCounter(&counterAtStart);

	sprintf(ServerStatus, "Master Connection: Initializing....");
	
	boundsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (boundsock == INVALID_SOCKET)
	{
		TRACE("XOnlineStartup - We couldn't intialize our socket");
	}

	return 0;
}



// #11: XSocketBind
SOCKET WINAPI XSocketBind(SOCKET s, const struct sockaddr *name, int namelen)
{
	int port = (((struct sockaddr_in*)name)->sin_port);

	
	if (ntohs(port) == 1001)
	{
		game_sock = s;
		TRACE("game_sock set for port %i", ntohs(port));
	}

	if (ntohs(port) == 1000)
	{
		game_sock_1000 = s;
		TRACE("game_sock set for port %i", ntohs(port));
	}

	User.sockmap[s] = ntohs(port);


	SOCKET ret = bind(s, name, namelen);
	
	if (ret == SOCKET_ERROR)
	{
		TRACE("XSocketBind - SOCKET_ERROR");
	}
	return ret;
}

// #53: XNetRandom
INT WINAPI XNetRandom(BYTE * pb, UINT cb)
{
	if (cb)
		for (DWORD i = 0; i < cb; i++)
			pb[i] = static_cast<BYTE>(rand());


	return 0;
}

// #54: XNetCreateKey
INT WINAPI XNetCreateKey(XNKID * pxnkid, XNKEY * pxnkey)
{
	TRACE("XNetCreateKey");
	if (pxnkid && pxnkey)
	{

		memset(pxnkid, 0xAB, sizeof(XNKID));
		memset(pxnkey, 0XAA, sizeof(XNKEY));

		pxnkid->ab[0] &= ~XNET_XNKID_MASK;
		pxnkid->ab[0] |= XNET_XNKID_SYSTEM_LINK;

		//only the server ever creates the session key
		isServer = true;

	}
	return 0;
}

// #57: XNetXnAddrToInAddr
INT WINAPI XNetXnAddrToInAddr(XNADDR *pxna, XNKID *pnkid, IN_ADDR *pina)
{
	TRACE("XNetXNAddrToInAddr - ina.s_addr: %08X", pxna->ina.s_addr);
	TRACE("XNetXNAddrToInAddr - secure: %08X", pxna->inaOnline.s_addr);
	ULONG secure = pxna->inaOnline.s_addr;


	if (secure !=0)
    {
		pina->s_addr = secure;

	/*	User.cusers_mutex.lock();
			CUser* user = User.cusers[secure];
		User.cusers_mutex.unlock();

		if (user == 0) 
		{
			CUser *nUser = new CUser;
			memset(&nUser->pxna, 0x00, sizeof(XNADDR));
			memcpy(&nUser->pxna, pxna, sizeof(XNADDR));
			std::string ab(reinterpret_cast<const char*>(pxna->abEnet), 6);

			User.cusers_mutex.lock();
			User.xnmap_mutex.lock();
			User.xntosecure_mutex.lock();

			User.cusers[secure] = nUser;
			User.xnmap[secure] = pxna->ina.s_addr;
			User.xntosecure[ab] = secure;

			User.cusers_mutex.unlock();
			User.xnmap_mutex.unlock();
			User.xntosecure_mutex.unlock();
		}*/
	}
	else
	{
		TRACE("XNetXnAddrToInAddr() - Could not find user with the AbEnet Specified calling User.GetSecureFromXN()");

		TRACE("XNetXNAddrToInAddr: pina->s_addr: %08X - User.GetSecureFromXN(pxna)", pina->s_addr);
		pina->s_addr = User.GetSecureFromXN(pxna);
	}

	return 0;
}

int ConnectionAttempts = 0;

// #73: XNetGetTitleXnAddr
DWORD WINAPI XNetGetTitleXnAddr(XNADDR * pAddr)
{
	static int print = 0;

	if (pAddr)
	{
		if (User.GetLocalXNAddr(pAddr))
		{
			MasterState = 2;
			sprintf(ServerStatus, "Master Connection: Connected!");
			return XNET_GET_XNADDR_STATIC | XNET_GET_XNADDR_ETHERNET;
		}
	}
	
	if( ConnectionAttempts > 5)
	{ 
		MasterState = 3;
		sprintf(ServerStatus, "Master Connection: This is taking awhile... - Either your login is invalid or your firewall is blocking us.");

		return XNET_GET_XNADDR_STATIC | XNET_GET_XNADDR_ETHERNET;
	}
	else 
	{
		MasterState = 1;
		sprintf(ServerStatus, "Master Connection: Connecting... - You will notice an FPS decrease during this.");
		ConnectionAttempts++;
	}

	
	
	

	return XNET_GET_XNADDR_PENDING;
}

// #24: XSocketSendTo
int WINAPI XSocketSendTo(SOCKET s, const char *buf, int len, int flags, sockaddr *to, int tolen)
{
	short port = (((struct sockaddr_in*)to)->sin_port);
	u_long iplong = (((struct sockaddr_in*)to)->sin_addr.s_addr);
	ADDRESS_FAMILY af = (((struct sockaddr_in*)to)->sin_family);


	if (iplong == INADDR_BROADCAST || iplong == 0x00)
	{
		(((struct sockaddr_in*)to)->sin_addr.s_addr) = broadcast_server;
		TRACE("XSocketSendTo - Broadcast");

		return sendto(s, buf, len, flags, to, tolen);
	}

		


		/*
			Create new SOCKADDR_IN structure,
			If we overwrite the original the game's security functions know it's not a secure address any longer.
			Worst case if this is found to cause performance issues we can handle the send and re-update to secure before return.
		*/
		
	    SOCKADDR_IN SendStruct;
		SendStruct.sin_port = port;
		SendStruct.sin_addr.s_addr = iplong;
		SendStruct.sin_family = AF_INET;

		/* 
		 Handle NAT map socket to port
		 Switch on port to determine which port we're intending to send to.
		 1000-> User.pmap_a[secureaddress]
		 1001-> User.pmap_b[secureaddress]
		*/
		int nPort = 0;

		switch (htons(port))
		{
			case 1000:
				nPort = User.pmap_a[iplong];


				if (nPort != 0)
				{
					SendStruct.sin_port = nPort;
				}
			break;

			case 1001:
				nPort = User.pmap_b[iplong];



				if (nPort != 0)
				{
					SendStruct.sin_port = nPort;
				}

			break;

			default:
				TRACE("XSocketSendTo() port: %i not matched!", htons(port));
			break;
		}

		std::pair <ULONG, SHORT> hostpair = std::make_pair(iplong, SendStruct.sin_port);

		u_long xn = User.xnmap[iplong];


		if (xn != 0)
		{
			SendStruct.sin_addr.s_addr = xn;
		}
		else
		{
			SendStruct.sin_addr.s_addr = User.GetXNFromSecure(iplong);
		}
    
	int ret = sendto(s, buf, len, flags, (SOCKADDR *) &SendStruct, sizeof(SendStruct));

	if (ret == SOCKET_ERROR)
	{
		TRACE("XSocketSendTo - Socket Error True");
		TRACE("XSocketSendTo - WSAGetLastError(): %08X", WSAGetLastError());


	}

	return ret;
}



int TimesWrittenTo=0;

// #20
int WINAPI XSocketRecvFrom(SOCKET s, char *buf, int len, int flags, sockaddr *from, int *fromlen)
{
	int ret = recvfrom(s, buf, len, flags, from, fromlen);

	
	
	if (ret > 0)
	{

		u_long iplong = (((struct sockaddr_in*)from)->sin_addr.s_addr);
		short port    =	(((struct sockaddr_in*)from)->sin_port);
		std::pair <ULONG, SHORT> hostpair = std::make_pair(iplong,port); 

		if (iplong != broadcast_server)
		{

			if (*(ULONG*)buf == 0x11223344)
			{
				TRACE("XSocketRecvFrom() Got security packet form iplong: %08X:%i", iplong,htons(port));
					User.smap[hostpair] = *(ULONG*)(buf+4);
				TRACE("XSocketRecvFrom() Security packet address: %08X", *(ULONG*)(buf + 4));

				ret = 0;
			}


			ULONG secure = User.smap[hostpair];


			(((struct sockaddr_in*)from)->sin_addr.s_addr) = secure;

			if (secure == 0)
			{
				TRACE("This is probably the issue.... now the fucking recv address is 0 you numb nuts. iplong: %08X, port: %08X",iplong,htons(port));
			}
			/* Store NAT data 
			   First we look at our socket's intended port.
			   port 1000 is mapped to the receiving port pmap_a via the secure address.
			   port 1001 is mapped to the receiving port pmap_b via the secure address.
			  */
			switch (User.sockmap[s])
			{
				case 1000:
						User.pmap_a[secure] = port;
				break;

				case 1001:
						User.pmap_b[secure] = port;
				break;

				default:
					TRACE("XSocketRecvFrom() User.sockmap[s] didn't match any ports!");
				break;

			}
		}

	}

	

	return ret;
}

// #55: XNetRegisterKey //need #51
int WINAPI XNetRegisterKey(DWORD, DWORD)
{
	TRACE("XNetRegisterKey");
	return 0;
}


// #56: XNetUnregisterKey // need #51
int WINAPI XNetUnregisterKey(DWORD)
{
	TRACE("XNetUnregisterKey");
	return 0;
}

// #60: XNetInAddrToXnAddr
INT   WINAPI XNetInAddrToXnAddr(const IN_ADDR ina, XNADDR * pxna, XNKID * pxnkid)
{
	CUser* user = User.cusers[ina.s_addr];
	
	if (user != 0)
	{
		memset(pxna, 0x00, sizeof(XNADDR)); // Zero memory of the current buffer passed to us by the game.
		memcpy(pxna, &user->pxna, sizeof(XNADDR));

	}
	else
	{
		TRACE("XnetInAddrToXnAddr() - Data did not exist calling User.GetXNFromSecure()");

		ULONG xnaddr = User.GetXNFromSecure(ina.s_addr);
		user = User.cusers[ina.s_addr];
	

		memset(pxna, 0x00, sizeof(XNADDR)); // Zero the memory of the current buffer before doing the copy.
		memcpy(pxna, &user->pxna, sizeof(XNADDR));

	}

	return 0;
}

// #63: XNetUnregisterInAddr
int WINAPI XNetUnregisterInAddr(const IN_ADDR ina)
{

	/*

	  I'm thinking about ditching the unregister... 
	  Due to the fact it won't consume too much memory not doing it and it will basically just cache everyone's shit.
	  
	  So it's a matter of if we want to reduce resource usage and reduce performance at the same time, or increase performance and increase resource usage.
	  
	  Most people have enough RAM these days we can get away with leaving this as a hacky ass implementation

	  */
	//User.UnregisterSecureAddr(ina);
	TRACE("XNetUnregisterInAddr: %08X",ina.s_addr);
	return 0;
}