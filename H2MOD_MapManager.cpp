#include "Globals.h"
#include <h2mod.pb.h>
#include <fstream>
#include "H2MOD_ServerList.h"
#include <Urlmon.h>

#pragma comment (lib, "urlmon.lib")

#include <miniz.c>

std::wstring DOWNLOADING_MAP(L"Downloading Map");
std::wstring WAITING_FOR_MAP_DOWNLOAD_URL(L"Waiting for map url from server");
std::wstring FOUND_MAP_DOWNLOAD_URL(L"Found map download url");
std::wstring DOWNLOADING_COMPLETE(L"Downloading complete");
std::wstring RELOADING_MAPS(L"Reloading maps in memory");
std::wstring UNZIPPING_MAP_DOWNLOAD(L"Unzipping map download");
std::wstring FAILED_TO_OPEN_ZIP_FILE(L"Failed to open the zip file");
std::wstring STILL_SEARCHING_FOR_MAP(L"Could not find maps from server, still searching");
std::wstring COULD_NOT_FIND_MAPS(L"Couldn't find the map");

std::wstring CUSTOM_MAP = L"Custom Map";
wchar_t empty2 = '\0';

MapManager::MapManager() {
	this->checkedMaps.insert(CUSTOM_MAP);
}

std::wstring MapManager::getCurrentMapName() {
	wchar_t* currentMapName = (wchar_t*)(h2mod->GetBase() + 0x97737C);

	DWORD dwBack;
	VirtualProtect(currentMapName, 4, PAGE_EXECUTE_READWRITE, &dwBack);
	//set access for copy of the string
	std::wstring ucurrentMapName(currentMapName);
	VirtualProtect(currentMapName, 4, dwBack, NULL);

	return ucurrentMapName;
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

//this only try a few times and give up completely till a rejoin occurs
void MapManager::requestMapDownloadUrl(SOCKET comm_socket, SOCKADDR_IN SenderAddr) {
	if (mapDownloadUrl.empty() && maxDownloads > 0) {
		maxDownloads--;
		//only ask the server for the map url if we don't have one
		H2ModPacket pack;
		pack.set_type(H2ModPacket_Type_get_map_download_url);

		char* SendBuf = new char[pack.ByteSize()];
		memset(SendBuf, 0x00, pack.ByteSize());
		pack.SerializeToArray(SendBuf, pack.ByteSize());

		sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

		delete[] SendBuf;
	}
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
}

void MapManager::setCustomLobbyMessage(const wchar_t* newStatus) {
	this->customLobbyMessage = newStatus;
	Sleep(750);
}

const wchar_t* MapManager::getCustomLobbyMessage()
{
	return this->customLobbyMessage;
}

void MapManager::unzipArchive(std::wstring localPath, std::wstring mapsDir) {
	mz_bool status;
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));

	status = mz_zip_reader_init_file(&zip_archive, std::string(localPath.begin(), localPath.end()).c_str(), 0);
	if (!status) {
		TRACE("Failed to open zip file, status=%d", status);
		this->setCustomLobbyMessage(FAILED_TO_OPEN_ZIP_FILE.c_str());
		return;
	}
	this->setCustomLobbyMessage(UNZIPPING_MAP_DOWNLOAD.c_str());

	mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
	for (int i = 0; i < (int)numFiles; i++) {

		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
			TRACE("mz_zip_reader_file_stat() failed!");
			mz_zip_reader_end(&zip_archive);
			return;
		}

		if (!mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
			std::string zipDest(mapsDir.begin(), mapsDir.end());
			std::string zipEntryFileName(file_stat.m_filename);

			std::vector<std::string> v = split(zipEntryFileName, '.');

			//TODO: this might not be the maps actual name in memory, might need to get that somehow

			//each map unzipped gets counted as a checked map
			this->checkedMaps.insert(std::wstring(v[0].begin(), v[0].end()));

			zipDest += zipEntryFileName;
			if (!mz_zip_reader_extract_to_file(&zip_archive, i, zipDest.c_str(), 0)) {
				TRACE("Could not extract zip file");
			}
		}
		else {
			TRACE("nO Support to extract directories, cause I'm lazy");
		}
	}
	mz_zip_reader_end(&zip_archive);
}

BOOL MapManager::downloadMap(std::wstring mapName) {
	std::string url = this->mapDownloadUrl;
	std::string type = this->mapDownloadType;
	std::wstring unicodeUrl(url.begin(), url.end());

	DWORD dwBack;
	wchar_t* mapsDirectory = (wchar_t*)(h2mod->GetBase() + 0x482D70 + 0x2423C);

	VirtualProtect(mapsDirectory, 4, PAGE_EXECUTE_READ, &dwBack);
	std::wstring mapsDir(mapsDirectory);
	std::wstring localPath(mapsDir);
	VirtualProtect(mapsDirectory, 4, dwBack, NULL);

	localPath += mapName + L"." + std::wstring(type.begin(), type.end());

	this->setCustomLobbyMessage(DOWNLOADING_MAP.c_str());
	//TODO: make async
	//TODO: probably just use libcurl
	HRESULT res = URLDownloadToFile(NULL, unicodeUrl.c_str(), localPath.c_str(), 0, NULL);

	if (res == S_OK) {
		this->setCustomLobbyMessage(DOWNLOADING_COMPLETE.c_str());
		TRACE("Map downloaded");
		//if we downloaded a zip file, unzip it
		if (type == "zip") {
			unzipArchive(localPath, mapsDir);
			//delete any zip files in map folder
			_wremove(localPath.c_str());
		}

		this->setCustomLobbyMessage(RELOADING_MAPS.c_str());
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

	return false;
}

void MapManager::setMapDownloadUrl(std::string url) {
	this->mapDownloadUrl = url;
}

void MapManager::setMapDownloadType(std::string type) {
	this->mapDownloadType = type;
}

void MapManager::searchForMap() {
	//run this thread once
	overrideUnicodeMessage = true;

	std::wstring currentMapName = this->getCurrentMapName();
	//regardless what happens during the search, we only check for a map once per game session
	checkedMaps.insert(currentMapName);

	if (hasMap(currentMapName)) {
		//if somehow we have the map and made it here, something is wrong, exit
		TRACE_GAME_N("Map already exists, should not be searching for it");
		return;
	}

	if (isLobby) {
		//only run map searches if in lobby

		if (this->downloadFromExternal()) {
			return;
		} 

		this->setCustomLobbyMessage(STILL_SEARCHING_FOR_MAP.c_str());
		if (this->downloadFromUrl()) {
			return;
		} 

		this->setCustomLobbyMessage(STILL_SEARCHING_FOR_MAP.c_str());
		//TODO: control via xlive property
		if (this->downloadFromHost()) {
			return;
		}

		this->setCustomLobbyMessage(COULD_NOT_FIND_MAPS.c_str());
	}
}

bool MapManager::downloadFromUrl() {
	std::wstring ucurrentMapName = this->getCurrentMapName();
	int attempts = 10;
	//this attempt logic exists soley because the hook that this triggers from could be called before
	//the server has had a chance to send us a download url for the map, so we check if the url is set
	//over every second for 10 seconds till we find it
	while (attempts > 0) {
		this->setCustomLobbyMessage(WAITING_FOR_MAP_DOWNLOAD_URL.c_str());
		if (!this->mapDownloadUrl.empty()) {
			this->setCustomLobbyMessage(FOUND_MAP_DOWNLOAD_URL.c_str());
			if (downloadMap(ucurrentMapName)) {
				//TODO: use constants
				this->setCustomLobbyMessage(L"Finished");
				this->setCustomLobbyMessage(NULL);
				return true;
			}
			else {
				//try other downloading options
				break;
			}
		}
		Sleep(1000);
		attempts--;
	}
	return false;
}

class TwigzieHandler : public ResponseHandler {
	virtual void handleResponse(char* responseBody) {
		TRACE_GAME_N("Response body=%s", responseBody);
		//Soda Hil|http://dl.halomaps.org/dl.cfm?fid=2154&f=zbe_soda.zip&S=2&h=2B8C199DDBAFCB0CAF1A74DC0CB3E265
	}
};

bool MapManager::downloadFromExternal() {
	//TODO: use special search to try and find map by name

	char URL[1024];
	char* geturi = "api.aspx?action=1&sid=0&name=soda%20hil&json=false";

	wsprintfA(URL, geturi);

	TwigzieHandler* handler = new TwigzieHandler();
	Request(0, "fantalitystudios.somee.com", URL, NULL, NULL, handler);
	delete handler;
	return false;
}

bool MapManager::downloadFromHost() {
	//TODO: download directly from server
	return false;
}

void MapManager::startMapDownload() {
	std::thread t1(&MapManager::searchForMap, this);
	t1.detach();
}

bool MapManager::canDownload() {
	wchar_t* currentMapName = (wchar_t*)(h2mod->GetBase() + 0x97737C);

	DWORD dwBack;
	VirtualProtect(currentMapName, 4, PAGE_EXECUTE_READWRITE, &dwBack);

	
	bool isMapNameEmpty = currentMapName == NULL || currentMapName[0] == L'\0';
	if (isMapNameEmpty) {
		//if pointer to map name is null or there is no string data, then we don't try to download
		return false;
	}

	
	bool isExcludedMap = wcscmp(currentMapName, CUSTOM_MAP.c_str()) == 0;
	if (isExcludedMap) {
		//if the map is named "custom map" don't download
		return false;
	}

	for (auto it = checkedMaps.begin(); it != checkedMaps.end(); ++it)	{
		if (wcscmp(currentMapName, it->c_str()) == 0) {
			//we already downloaded the map, no need to check for it again
			return false;
		}
	}

	VirtualProtect(currentMapName, 4, dwBack, NULL);

	//if we made it here, start downloading
	return true;
}