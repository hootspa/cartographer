#include <stdafx.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include "H2MOD.h"
#include "H2MOD_GunGame.h"
#include "H2MOD_Infection.h"
#include "Network.h"
#include "xliveless.h"
#include <CUser.h>
#include <h2mod.pb.h>


extern UINT g_port;
extern bool isHost;
extern HANDLE H2MOD_Network;
extern bool NetworkActive;
extern bool Connected;
extern bool ThreadCreated;
extern XNADDR join_game_xn;
extern CUserManagement User;
SOCKET comm_socket = INVALID_SOCKET;
char* NetworkData = new char[500];


DWORD WINAPI NetworkThread(LPVOID lParam)
{
	TRACE_GAME("[h2mod-network] NetworkThread Initializing");

	if (comm_socket == INVALID_SOCKET)
	{
		comm_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (comm_socket == INVALID_SOCKET)
		{
			TRACE_GAME("[h2mod-network] Socket is invalid even after socket()");
		}

		SOCKADDR_IN RecvStruct;
		RecvStruct.sin_port = htons(((short)g_port) + 7);
		RecvStruct.sin_addr.s_addr = htonl(INADDR_ANY);

		RecvStruct.sin_family = AF_INET;

		if (bind(comm_socket, (const sockaddr*)&RecvStruct, sizeof RecvStruct) == -1)
		{
			TRACE_GAME("[h2mod-network] Would not bind socket!");
		}

		DWORD dwTime = 20;

		if (setsockopt(comm_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime)) < 0)
		{
			TRACE_GAME("[h2mod-network] Socket Error on register request");
		}

		TRACE_GAME("[h2mod-network] Listening on port: %i", ntohs(RecvStruct.sin_port));
	}
	else
	{
		TRACE_GAME("[h2mod-network] socket already existed continuing without attempting bind...");
	}






	TRACE_GAME("[h2mod-network] Are we host? %i", isHost);

	NetworkActive = true;

	if (isHost)
	{
		TRACE_GAME("[h2mod-network] We're host waiting for a authorization packet from joining clients...");

		while (1)
		{

			if (NetworkActive == false)
			{
				isHost = false;
				Connected = false;
				ThreadCreated = false;
				H2MOD_Network = 0;
				TRACE_GAME("[h2mod-network] Killing host thread NetworkActive == false");
				return 0;
			}
			sockaddr_in SenderAddr;
			int SenderAddrSize = sizeof(SenderAddr);

			if (h2mod->NetworkPlayers.size() > 0)
			{
				auto it = h2mod->NetworkPlayers.begin();
				while (it != h2mod->NetworkPlayers.end())
				{
					if (it->second == 0)
					{
						TRACE_GAME("[h2mod-network] Deleting player %ws as their value was set to 0", it->first->PlayerName);

						if (it->first->PacketsAvailable == true)
							delete[] it->first->PacketData; // Delete packet data if there is any.


						delete[] it->first; // Clear NetworkPlayer object.

						it = h2mod->NetworkPlayers.erase(it);


					}
					else
					{
						if (it->first->PacketsAvailable == true) // If there's a packet available we set this to true already.
						{
							TRACE_GAME("[h2mod-network] Sending player %ws data", it->first->PlayerName);

							SOCKADDR_IN QueueSock;
							QueueSock.sin_port = it->first->port; // We can grab the port they connected from.
							QueueSock.sin_addr.s_addr = it->first->addr; // Address they connected from.
							QueueSock.sin_family = AF_INET;

							sendto(comm_socket, it->first->PacketData, it->first->PacketSize, 0, (sockaddr*)&QueueSock, sizeof(QueueSock)); // Just send the already serialized data over the socket.

							it->first->PacketsAvailable = false;
							delete[] it->first->PacketData; // Delete packet data we've sent it already.
						}
						it++;
					}
				}
			}

			memset(NetworkData, 0x00, 500);
			int recvresult = recvfrom(comm_socket, NetworkData, 500, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

			if (recvresult > 0)
			{
				TRACE_GAME("[H2MOD-Network] recvresult: %i", recvresult);
				int data_handled = 0;

				while (data_handled != recvresult)
				{


					bool already_authed = false;
					H2ModPacket recvpak;
					recvpak.ParseFromArray(NetworkData, (recvresult - data_handled));
					TRACE_GAME("[H2Mod-Network] data_handled: %i, recvpak.ByteSize(): %i", data_handled,recvpak.ByteSize());

					memcpy(NetworkData, &NetworkData[recvpak.ByteSize()], (recvresult - recvpak.ByteSize() - data_handled));

					if (recvpak.has_type())
					{
						switch (recvpak.type())
						{

						case H2ModPacket_Type_authorize_client:
							TRACE_GAME("[h2mod-network] Player Connected!");
							if (recvpak.has_h2auth())
							{
								if (recvpak.h2auth().has_name())
								{
									wchar_t* PlayerName = new wchar_t[17];
									memset(PlayerName, 0x00, 17);
									wcscpy(PlayerName, (wchar_t*)recvpak.h2auth().name().c_str());


									for (auto it = h2mod->NetworkPlayers.begin(); it != h2mod->NetworkPlayers.end(); ++it)
									{
										if (wcscmp(it->first->PlayerName, PlayerName) == 0)
										{

											TRACE_GAME("[h2mod-network] This player ( %ws ) was already connected, sending them another packet letting them know they're authed already.", PlayerName);

											H2ModPacket h2pak;
											h2pak.set_type(H2ModPacket_Type_authorize_client);

											h2mod_auth *authpak = h2pak.mutable_h2auth();
											authpak->set_name(PlayerName, 34);
											authpak->set_secureaddr(recvpak.h2auth().secureaddr());

											char* SendBuf = new char[h2pak.ByteSize()];
											memset(SendBuf, 0x00, h2pak.ByteSize());
											h2pak.SerializeToArray(SendBuf, h2pak.ByteSize());

											sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));


											already_authed = true;

											delete[] SendBuf;
										}
									}

									if (already_authed == false)
									{
										NetworkPlayer *nPlayer = new NetworkPlayer;


										TRACE_GAME("[h2mod-network] PlayerName: %ws", PlayerName);
										TRACE_GAME("[h2mod-network] IP:PORT: %08X:%i", SenderAddr.sin_addr.s_addr, ntohs(SenderAddr.sin_port));

										nPlayer->addr = SenderAddr.sin_addr.s_addr;
										nPlayer->port = SenderAddr.sin_port;
										nPlayer->PlayerName = PlayerName;
										nPlayer->secure = recvpak.h2auth().secureaddr();
										h2mod->NetworkPlayers[nPlayer] = 1;

										H2ModPacket h2pak;
										h2pak.set_type(H2ModPacket_Type_authorize_client);

										h2mod_auth *authpak = h2pak.mutable_h2auth();
										authpak->set_name(PlayerName, 34);
										authpak->set_secureaddr(recvpak.h2auth().secureaddr());

										char* SendBuf = new char[h2pak.ByteSize()];
										memset(SendBuf, 0x00, h2pak.ByteSize());
										h2pak.SerializeToArray(SendBuf, h2pak.ByteSize());

										sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

										delete[] SendBuf;
									}
								}
							}
							break;

						case H2ModPacket_Type_h2mod_ping:
							H2ModPacket pongpak;
							pongpak.set_type(H2ModPacket_Type_h2mod_pong);

							TRACE_GAME("[h2mod-network] ping packet from client sending pong...");
							TRACE_GAME("[h2mod-network] IP:PORT: %08X:%i", SenderAddr.sin_addr.s_addr, ntohs(SenderAddr.sin_port));

							char* pongdata = new char[pongpak.ByteSize()];
							pongpak.SerializeToArray(pongdata, pongpak.ByteSize());
							sendto(comm_socket, pongdata, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

							delete[] pongdata;
							break;

						}
					}
					data_handled = data_handled + recvpak.ByteSize();
				}
				Sleep(500);
			}
		}
	}
	else
	{
		Connected = false;
		TRACE_GAME("[h2mod-network] We're a client connecting to server...");


		SOCKADDR_IN SendStruct;
		SendStruct.sin_port = htons(ntohs(join_game_xn.wPortOnline) + 7);
		SendStruct.sin_addr.s_addr = join_game_xn.ina.s_addr;
		SendStruct.sin_family = AF_INET;

		TRACE_GAME("[h2mod-network] Connecting to server on %08X:%i", SendStruct.sin_addr.s_addr, ntohs(SendStruct.sin_port));

		H2ModPacket h2pak;
		h2pak.set_type(H2ModPacket_Type_authorize_client);

		h2mod_auth *authpak = h2pak.mutable_h2auth();
		authpak->set_name((char*)h2mod->get_local_player_name(), 34);
		authpak->set_secureaddr(User.LocalSec);

		char* SendBuf = new char[h2pak.ByteSize()];
		memset(SendBuf, 0x00, h2pak.ByteSize());
		h2pak.SerializeToArray(SendBuf, h2pak.ByteSize());

		sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

		while (1)
		{
			if (NetworkActive == false)
			{
				isHost = false;
				Connected = false;
				ThreadCreated = false;
				H2MOD_Network = 0;
				TRACE_GAME("[h2mod-network] Networkactive == false ending client thread.");
				return 0;
			}

			if (Connected == false)
			{
				TRACE_GAME("[h2mod-network] Client - we're not connected re-sending our auth..");
				sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));
			}

			sockaddr_in SenderAddr;
			int SenderAddrSize = sizeof(SenderAddr);

			memset(NetworkData, 0x00, 500);

			int recvresult = recvfrom(comm_socket, NetworkData, 500, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

			if (recvresult > 0)
			{
				TRACE_GAME("[H2MOD-Network] recvresult: %i", recvresult);
				int data_handled = 0;

				while(data_handled != recvresult)
				{
				
					H2ModPacket recvpak;
					recvpak.ParseFromArray(NetworkData, (recvresult-data_handled) );
					TRACE_GAME("[H2Mod-Network] data_handled: %i, recvpak.ByteSize(): %i", data_handled,recvpak.ByteSize());

					memcpy(NetworkData, &NetworkData[recvpak.ByteSize()], ( recvresult - recvpak.ByteSize() - data_handled ));

					if (recvpak.has_type())
					{
						switch (recvpak.type())
						{
							case H2ModPacket_Type_h2mod_pong:
								TRACE("[h2mod-network] Got a pong packet back!");
								break;

							case H2ModPacket_Type_authorize_client:

								if (Connected == false)
								{
									TRACE("[h2mod-network] Got the auth packet back!, We're connected!");
									Connected = true;
								}

								break;

								case H2ModPacket_Type_set_player_team:

									if (recvpak.has_h2_set_player_team())
									{

										BYTE TeamIndex = recvpak.h2_set_player_team().team();
										TRACE_GAME("[h2mod-network] Got a set team request from server! TeamIndex: %i", TeamIndex);
										h2mod->set_local_team_index(TeamIndex);
									}

								break;

							case H2ModPacket_Type_set_unit_grenades:
								TRACE_GAME("[h2mod-network] Got a set unit grenades request, updating our grenades...");
								if (recvpak.has_set_grenade())
								{
									BYTE type = recvpak.set_grenade().type();
									BYTE count = recvpak.set_grenade().count();
									BYTE pIndex = recvpak.set_grenade().pindex();

									h2mod->set_local_grenades(type, count, pIndex);
								}
							break;
						}
					}

					data_handled = data_handled + recvpak.ByteSize();
				}

			}

			if (Connected == true)
			{
				TRACE_GAME("[H2MOD-Network] We're connected so sending ping packets...");

				H2ModPacket pack;
				pack.set_type(H2ModPacket_Type_h2mod_ping);

				char* SendBuf = new char[pack.ByteSize()];
				memset(SendBuf, 0x00, pack.ByteSize());
				pack.SerializeToArray(SendBuf, pack.ByteSize());

				sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

				delete[] SendBuf;
			}


			Sleep(500);
		}

	}

	return 0;
}