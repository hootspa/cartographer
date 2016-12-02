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
#include "Globals.h"

extern UINT g_port;
extern bool isHost;
extern HANDLE H2MOD_Network;
extern bool NetworkActive;
extern bool Connected;
extern bool ThreadCreated;
extern XNADDR join_game_xn;
extern CUserManagement User;
SOCKET comm_socket = INVALID_SOCKET;

char* NetworkData = new char[255];

void authorizeClient(wchar_t* PlayerName, ::google::protobuf::uint32 secureaddr, sockaddr_in SenderAddr) {
	H2ModPacket h2pak;
	h2pak.set_type(H2ModPacket_Type_authorize_client);

	h2mod_auth *authpak = h2pak.mutable_h2auth();
	authpak->set_name(PlayerName, 34);
	authpak->set_secureaddr(secureaddr);

	char* SendBuf = new char[h2pak.ByteSize()];
	memset(SendBuf, 0x00, h2pak.ByteSize());
	h2pak.SerializeToArray(SendBuf, h2pak.ByteSize());

	sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

	delete[] SendBuf;
}

void onAuthorizeClient(H2ModPacket recvpak, sockaddr_in SenderAddr) {
	bool already_authed = false;
	if (recvpak.has_h2auth() && recvpak.h2auth().has_name())
	{
		wchar_t* PlayerName = new wchar_t[17];
		memset(PlayerName, 0x00, 17);
		wcscpy(PlayerName, (wchar_t*)recvpak.h2auth().name().c_str());


		for (auto it = h2mod->NetworkPlayers.begin(); it != h2mod->NetworkPlayers.end(); ++it) {
			if (wcscmp(it->first->PlayerName, PlayerName) == 0) {
				TRACE_GAME("[h2mod-network] This player ( %ws ) was already connected, sending them another packet letting them know they're authed already.", PlayerName);

				authorizeClient(PlayerName, recvpak.h2auth().secureaddr(), SenderAddr);
				already_authed = true;
			}
		}

		if (already_authed == false) {
			//TODO: should be in its own method
			NetworkPlayer *nPlayer = new NetworkPlayer();
			TRACE_GAME("[h2mod-network] PlayerName: %ws", PlayerName);
			TRACE_GAME("[h2mod-network] IP:PORT: %08X:%i", SenderAddr.sin_addr.s_addr, ntohs(SenderAddr.sin_port));

			nPlayer->addr = SenderAddr.sin_addr.s_addr;
			nPlayer->port = SenderAddr.sin_port;
			nPlayer->PlayerName = PlayerName;
			nPlayer->secure = recvpak.h2auth().secureaddr();
			h2mod->NetworkPlayers[nPlayer] = 1;

			authorizeClient(PlayerName, recvpak.h2auth().secureaddr(), SenderAddr);
		}
	}
}

void onPing(H2ModPacket recvpak, sockaddr_in SenderAddr) {
	H2ModPacket pongpak;
	pongpak.set_type(H2ModPacket_Type_h2mod_pong);

	char* pongdata = new char[pongpak.ByteSize()];
	pongpak.SerializeToArray(pongdata, pongpak.ByteSize());
	sendto(comm_socket, pongdata, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

	delete[] pongdata;
}

void receiveGameUpdates() {
	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);

	memset(NetworkData, 0x00, 255);
	int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

	if (recvresult > 0) {
		TRACE_GAME("[H2MOD-Network] recvresult: %i", recvresult);

		H2ModPacket recvpak;
		recvpak.ParseFromArray(NetworkData, recvresult);
		TRACE_GAME("[H2Mod-Network] recvpak.ByteSize(): %i", recvpak.ByteSize());

		if (recvpak.has_type()) {
			switch (recvpak.type()) {
			case H2ModPacket_Type_authorize_client:
				TRACE_GAME("[h2mod-network] Player Connected!");
				onAuthorizeClient(recvpak, SenderAddr);
				break;

			case H2ModPacket_Type_h2mod_ping:
				TRACE_GAME("[h2mod-network] ping packet from client sending pong...");
				TRACE_GAME("[h2mod-network] IP:PORT: %08X:%i", SenderAddr.sin_addr.s_addr, ntohs(SenderAddr.sin_port));
				onPing(recvpak, SenderAddr);
				break;

			case H2ModPacket_Type_get_map_download_url:
				//TODO: move into method
				//TODO: if the download link isn't set, we should be able to tell the client this
				//request from some client to get the map download url, send it
				H2ModPacket pack;
				pack.set_type(H2ModPacket_Type_map_download_url);
				h2mod_map_download_url* mapDownloadUrl = pack.mutable_map_url();
				if (strlen(customMapDownloadLink) != 0) {
					mapDownloadUrl->set_url(customMapDownloadLink);
					mapDownloadUrl->set_type("map");
				} else if (strlen(customMapZipDownloadLink) != 0) {
					mapDownloadUrl->set_url(customMapZipDownloadLink);
					mapDownloadUrl->set_type("zip");
				}	else {
					TRACE_GAME_N("[h2mod-network] no custom map downloading urls set");
				}

				char* SendBuf = new char[pack.ByteSize()];
				memset(SendBuf, 0x00, pack.ByteSize());
				pack.SerializeToArray(SendBuf, pack.ByteSize());

				sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));
				TRACE_GAME_N("[h2mod-network] Sending map download url=%s", mapDownloadUrl->mutable_url()->c_str());

				delete[] SendBuf;
				break;
			}
		}

		Sleep(1);
	}
}

void deletePlayer(NetworkPlayer* networkPlayer) {
	TRACE_GAME("[h2mod-network] Deleting player %ws as their value was set to 0", networkPlayer->PlayerName);

	if (networkPlayer->PacketsAvailable == true) {
		//TODO: this should be moved into the deconstructor for networkplayer
		delete[] networkPlayer->PacketData; // Delete packet data if there is any.
	}

	delete[] networkPlayer; // Clear NetworkPlayer object.
}

void sendPlayerData(NetworkPlayer* player) {
	SOCKADDR_IN QueueSock;
	QueueSock.sin_port = player->port; // We can grab the port they connected from.
	QueueSock.sin_addr.s_addr = player->addr; // Address they connected from.
	QueueSock.sin_family = AF_INET;

	sendto(comm_socket, player->PacketData, player->PacketSize, 0, (sockaddr*)&QueueSock, sizeof(QueueSock)); // Just send the already serialized data over the socket.

																																																						//TODO: should be its own method
	player->PacketsAvailable = false;

	delete[] player->PacketData; // Delete packet data we've sent it already.
}

void updateNetworkPlayers() {
	if (h2mod->NetworkPlayers.size() > 0) {
		auto iterator = h2mod->NetworkPlayers.begin();
		while (iterator != h2mod->NetworkPlayers.end()) {
			NetworkPlayer* player = iterator->first;
			if (iterator->second == 0) {
				TRACE_GAME("[h2mod-network] Deleting player %ws as their value was set to 0", player->PlayerName);
				deletePlayer(player);
				iterator = h2mod->NetworkPlayers.erase(iterator);
			}
			else {
				if (player->PacketsAvailable == true) {
					TRACE_GAME("[h2mod-network] Sending player %ws data", player->PlayerName);
					sendPlayerData(player);
				}
				iterator++;
			}
		}
	}
}

void reset() {
	isHost = false;
	Connected = false;
	ThreadCreated = false;
	H2MOD_Network = 0;
}

void runServer() {
	TRACE_GAME("[h2mod-network] We're host waiting for a authorization packet from joining clients...");

	while (true) {
		//TODO: duplicated code between server and client
		if (NetworkActive == false) {
			reset();
			TRACE_GAME("[h2mod-network] Killing host thread NetworkActive == false");
			break;
		}

		//update player information
		updateNetworkPlayers();

		//receive data from players (non blocking)
		receiveGameUpdates();

		Sleep(500);
	}
}

void runClient() {
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

	auto last_ping = std::chrono::high_resolution_clock::now();
	auto last_connect_attempt = std::chrono::high_resolution_clock::now();
	auto ping_sent = std::chrono::high_resolution_clock::now();
	bool waiting_for_pong = false;
	auto pong_received = ping_sent;
	__int64 ping_ms = 0;

	while (true)
	{
		auto time_now = std::chrono::high_resolution_clock::now();

		//TODO: duplicated code between server and client
		if (NetworkActive == false)
		{
			reset();
			TRACE_GAME("[h2mod-network] Networkactive == false ending client thread.");
			break;
		}


		auto seconds_passed_connect = std::chrono::duration_cast<std::chrono::seconds>(time_now - last_connect_attempt).count();

		if (Connected == false && seconds_passed_connect > 5)
		{
			TRACE_GAME("[h2mod-network] Client - we're not connected re-sending our auth.. seconds_passed_connect: %i", seconds_passed_connect);
			sendto(comm_socket, SendBuf, h2pak.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));
			last_connect_attempt = std::chrono::high_resolution_clock::now();
		}

		auto seconds_passed_ping = std::chrono::duration_cast<std::chrono::seconds>(time_now - last_ping).count();

		if (Connected == true && seconds_passed_ping > 5)
		{
			TRACE_GAME("[H2MOD-Network] We're connected so sending ping packets... seconds_passed_ping: %i", seconds_passed_ping);

			H2ModPacket pack;
			pack.set_type(H2ModPacket_Type_h2mod_ping);

			char* SendBuf = new char[pack.ByteSize()];
			memset(SendBuf, 0x00, pack.ByteSize());
			pack.SerializeToArray(SendBuf, pack.ByteSize());

			sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

			delete[] SendBuf;

			last_ping = std::chrono::high_resolution_clock::now();

			if (!waiting_for_pong)
			{
				ping_sent = std::chrono::high_resolution_clock::now();
				waiting_for_pong = true;
			}
		}

		//request map download url if necessary from the server
		mapManager->requestMapDownloadUrl(comm_socket, SendStruct);

		sockaddr_in SenderAddr;
		int SenderAddrSize = sizeof(SenderAddr);

		memset(NetworkData, 0x00, 255);

		int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

		if (recvresult > 0)
		{
			TRACE_GAME("[H2MOD-Network] recvresult: %i", recvresult);

			H2ModPacket recvpak;
			recvpak.ParseFromArray(NetworkData, recvresult);
			TRACE_GAME("[H2Mod-Network] recvpak.ByteSize(): %i", recvpak.ByteSize());

			if (recvpak.has_type())
			{
				switch (recvpak.type())
				{
				case H2ModPacket_Type_h2mod_pong:
					pong_received = std::chrono::high_resolution_clock::now();
					ping_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pong_received - ping_sent).count();
					waiting_for_pong = false;



					TRACE_GAME("[h2mod-network] Got a pong packet back!");
					TRACE_GAME("[h2mod-network] ping ms: %I64d", ping_ms);

					break;

				case H2ModPacket_Type_authorize_client:

					if (Connected == false)
					{
						TRACE_GAME("[h2mod-network] Got the auth packet back!, We're connected!");
						Connected = true;
					}

					break;

				case H2ModPacket_Type_map_download_url:
				{
					h2mod_map_download_url mapUrlPack = recvpak.map_url();
					if (mapUrlPack.has_url()) {
						std::string url = mapUrlPack.url();
						TRACE_GAME_N("[h2mod-network] Got the map download url from from server! url = %s", url.c_str());

						mapManager->setMapDownloadUrl(url);
						mapManager->setMapDownloadType(mapUrlPack.type());
					}
					else {
						TRACE_GAME_N("[h2mod-network] Got a map download packet but no url specified");
					}
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
		}

		Sleep(500);
	}
}

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

	if (isHost) {
		runServer();
	}
	else {
		runClient();
	}

	return 0;
}