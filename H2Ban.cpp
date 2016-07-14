#include "Globals.h"

bool BanUtility::isBannedAddress(IN_ADDR possibleBannedAddress) {
	return false;
}

//TODO: add a char* version
bool BanUtility::isBannedGamerTag(wchar_t* possibleBannedGamertag) {
	return false;
}

bool BanUtility::isBannedXuid(XUID possibleBannedXuid) {
	return false;
}

void BanUtility::banPlayer(char* gamertag, IN_ADDR ipAddress, XUID xboxId) {

}