#include "Globals.h"
#include <h2mod.pb.h>
#include <fstream>
#include <Urlmon.h>

#include <miniz.c>

std::wstring DOWNLOADING_MAP(L"Downloading Map");
std::wstring WAITING_FOR_MAP_DOWNLOAD_URL(L"Waiting for map url from server");
std::wstring FOUND_MAP_DOWNLOAD_URL(L"Found map download url");
std::wstring DOWNLOADING_COMPLETE(L"Downloading complete");
std::wstring RELOADING_MAPS(L"Reloading maps in memory");
std::wstring UNZIPPING_MAP_DOWNLOAD(L"Unzipping map download");
std::wstring FAILED_TO_OPEN_ZIP_FILE(L"Failed to open the zip file");
std::wstring STILL_SEARCHING_FOR_MAP(L"Could not find maps from server, still searching");

MapManager::MapManager() {}

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

void MapManager::unzipArchive(std::wstring localPath, std::wstring mapsDir) {
	mz_bool status;
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));

	status = mz_zip_reader_init_file(&zip_archive, std::string(localPath.begin(), localPath.end()).c_str(), 0);
	if (!status) {
		TRACE("Failed to open zip file, status=%d", status);
		this->setCustomLobbyMessage(FAILED_TO_OPEN_ZIP_FILE);
		return;
	}
	this->setCustomLobbyMessage(UNZIPPING_MAP_DOWNLOAD);

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

	this->setCustomLobbyMessage(DOWNLOADING_MAP);
	//TODO: make async
	HRESULT res = URLDownloadToFile(NULL, unicodeUrl.c_str(), localPath.c_str(), 0, NULL);

	if (res == S_OK) {
		this->setCustomLobbyMessage(DOWNLOADING_COMPLETE);
		TRACE("Map downloaded");
		//if we downloaded a zip file, unzip it
		if (type == "zip") {
			unzipArchive(localPath, mapsDir);
			//delete any zip files in map folder
			_wremove(localPath.c_str());
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

	return false;
}

std::wstring MapManager::getCustomLobbyMessage() {
	return this->customLobbyMessage;
}

void MapManager::setMapDownloadUrl(std::string url) {
	this->mapDownloadUrl = url;
}

void MapManager::setMapDownloadType(std::string type) {
	this->mapDownloadType = type;
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
					if (downloadMap(mapName)) {
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
			this->setCustomLobbyMessage(STILL_SEARCHING_FOR_MAP);
			//TODO: use special search to try and find map by name

		}
	}
}

void MapManager::startMapDownload() {
	std::thread t1(&MapManager::searchForMap, this);
	t1.detach();
}