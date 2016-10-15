#include "Globals.h"
#include <h2mod.pb.h>
#include <fstream>
#include <Urlmon.h>

std::wstring DOWNLOADING_MAP(L"Downloading Map");
std::wstring WAITING_FOR_MAP_DOWNLOAD_URL(L"Waiting for map url from server");
std::wstring FOUND_MAP_DOWNLOAD_URL(L"Found map download url");
std::wstring DOWNLOADING_COMPLETE(L"Downloading complete");
std::wstring RELOADING_MAPS(L"Reloading maps in memory");

MapManager::MapManager() {
}

bool MapManager::hasMap(std::wstring mapName) {
	DWORD dwBack;
	wchar_t* mapsDirectory = (wchar_t*)(h2mod->GetBase() + 0x482D70 + 0x2423C);
	VirtualProtect(mapsDirectory, 4, PAGE_EXECUTE_READ, &dwBack);

	std::wstring mapFileName(mapsDirectory);
	VirtualProtect(mapsDirectory, 4, dwBack, NULL);

	mapFileName += mapName + L".map";
	TRACE("Checking if the following map exists, %s", mapFileName.c_str());
	std::ifstream file(mapFileName.c_str());
	return (bool)file;
}

int maxDownloads = 5;

//TODO: make this only try a few times and give up completely till a rejoin occurs
void MapManager::requestMapDownloadUrl(SOCKET comm_socket, SOCKADDR_IN SenderAddr) {
	if (mapDownloadUrl.empty() && maxDownloads > 0) {
		//only ask the server for the map url if we don't have one
		H2ModPacket pack;
		pack.set_type(H2ModPacket_Type_get_map_download_url);

		char* SendBuf = new char[pack.ByteSize()];
		memset(SendBuf, 0x00, pack.ByteSize());
		pack.SerializeToArray(SendBuf, pack.ByteSize());

		sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));
		maxDownloads--;

		delete[] SendBuf;
	}
}

BOOL MapManager::hasCheckedMapAlready(std::wstring mapName) {
	return this->checkedMaps.find(mapName) != checkedMaps.end();
}

void MapManager::reloadMaps() {
	typedef char(__thiscall *possible_map_reload)(int thisx);
	possible_map_reload map_reload_method = (possible_map_reload)(h2mod->GetBase() + 0x4D021);
	DWORD* unk = (DWORD*)(h2mod->GetBase() + 0x482D70);

	typedef struct RTL_CRITICAL_SECTION**(__thiscall *init_critical_section)(int thisx);
	init_critical_section init_critical_section_method = (init_critical_section)(h2mod->GetBase() + 0xC18BD);

	DWORD dwBack;
	BOOL canprotect = VirtualProtect((WORD*)((int)unk + 148016), sizeof(WORD), PAGE_EXECUTE_READWRITE, &dwBack);
	if (!canprotect && GetLastError()) {
		TRACE("canprotect=%s, error=%d", canprotect, GetLastError());
	}
	init_critical_section_method((int)unk);
	map_reload_method((int)unk);
}

void MapManager::resetMapDownloadUrl() {
	this->mapDownloadUrl = "";
	maxDownloads = 5;
	this->checkedMaps.clear();
	this->customLobbyMessage = L"";
}

void MapManager::setCustomLobbyMessage(std::wstring newStatus) {
	this->customLobbyMessage = newStatus;
	Sleep(750);
}

bool MapManager::isZipFile(std::string path) {
	return path.substr(path.find_last_of(".") + 1) == "zip";
}

std::string MapManager::getFileName(const std::string& s) {
	char sep = '/';

#ifdef _WIN32
	sep = '\\';
#endif

	size_t i = s.rfind(sep, s.length());
	if (i != std::string::npos) {
		return(s.substr(i + 1, s.length() - i));
	}

	return("");
}

BOOL MapManager::downloadMap(std::string url, std::wstring mapName) {
	std::wstring unicodeUrl(url.begin(), url.end());

	DWORD dwBack;
	wchar_t* mapsDirectory = (wchar_t*)(h2mod->GetBase() + 0x482D70 + 0x2423C);

	VirtualProtect(mapsDirectory, 4, PAGE_EXECUTE_READ, &dwBack);
	std::wstring localPath(mapsDirectory);
	VirtualProtect(mapsDirectory, 4, dwBack, NULL);

	//TODO: since url's don't always contain the filename, this won't help
	//std::string filename = this->getFileName(url);
	localPath += mapName + L".map";

	this->setCustomLobbyMessage(DOWNLOADING_MAP);
	//TODO: make async
	//TODO: need to handle zips
	HRESULT res = URLDownloadToFile(NULL, unicodeUrl.c_str(), localPath.c_str(), 0, NULL);

	if (res == S_OK) {
		this->setCustomLobbyMessage(DOWNLOADING_COMPLETE);
		TRACE("Map downloaded");
		if (isZipFile(url)) {
			//TODo: unzip the file
		}

		this->setCustomLobbyMessage(RELOADING_MAPS);
		this->reloadMaps();
		//TODO: add counter reset method
		maxDownloads = 5;
		return true;
	}	else if (res == E_OUTOFMEMORY) {
		TRACE("Buffer length invalid, or insufficient memory");
	}	else if (res == INET_E_DOWNLOAD_FAILURE) {
		TRACE("URL is invalid");
	}	else {
		TRACE("Other error: %d\n", res);
	}

	this->setCustomLobbyMessage(ERROR);
	return false;
}

std::wstring MapManager::getCustomLobbyMessage() {
	return this->customLobbyMessage;
}

void MapManager::setMapDownloadUrl(std::string url) {
	this->mapDownloadUrl = url;
}

void MapManager::searchForMap() {
	wchar_t* currentMapName = (wchar_t*)(h2mod->GetBase() + 0x97737C);
	checkedMaps.insert(std::wstring(currentMapName));
	if (isLobby) {
		//only run in the lobby
		//TODO: set read access on currentMapName
		BOOL currentMapNameNotNull = currentMapName[0] != L' ';
		BOOL currentGameMapDifferent = wcscmp(this->currentMap.c_str(), currentMapName) != 0; 
		std::wstring mapName(currentMapName);
		BOOL hasCurrentMap = hasMap(mapName);
		if (currentMapNameNotNull, currentGameMapDifferent, !hasCurrentMap) {
			int seconds = 10;
			while (seconds > 0) {
				this->setCustomLobbyMessage(WAITING_FOR_MAP_DOWNLOAD_URL);
				if (!this->mapDownloadUrl.empty()) {
					this->setCustomLobbyMessage(FOUND_MAP_DOWNLOAD_URL);
					if (downloadMap(this->mapDownloadUrl, mapName)) {
						//all done, complete
						return;
					}	else {
						//try other downloading options
						break;
					}
				}
				Sleep(1000);
				seconds--;
			}

			//map still isn't found try another way to find the map
			//TODO: use special search to try and find map by name

			this->setCustomLobbyMessage(FOUND_MAP_DOWNLOAD_URL);
		}
	}
}

void MapManager::startMapDownload() {
	std::thread t1(&MapManager::searchForMap, this);
	t1.detach();
}
