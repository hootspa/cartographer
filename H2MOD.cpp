#include <stdafx.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include "H2MOD.h"
#include "H2MOD_GunGame.h"
#include "Network.h"
#include "Globals.h"
#include "CUser.h"

H2MOD *h2mod = new H2MOD();
GunGame *gg = new GunGame();
extern bool b_GunGame;
extern CUserManagement User;

HMODULE base;

extern int MasterState;


#pragma region engine calls

int __cdecl call_get_object(signed int object_datum_index, int object_type)
{
	TRACE("call_get_object( object_datum_index: %08X, object_type: %08X )", object_datum_index, object_type);

	typedef int(__cdecl *get_object)(signed int object_datum_index, int object_type);
	get_object pget_object = (get_object)((char*)h2mod->GetBase() + 0x1304E3);

	return pget_object(object_datum_index, object_type);
}


int __cdecl call_unit_get_team_index(int unit_datum_index) {

	TRACE("unit_get_team_index(unit_datum_index: %08X)", unit_datum_index);

	typedef int(__cdecl *unit_get_team_index)(int unit_datum_index);
	//30002BD8
	//old one = 0x13A531
	unit_get_team_index punit_get_team_index = (unit_get_team_index)(((char*)h2mod->GetBase()) + 0x13A531);

	if (unit_datum_index != -1 && unit_datum_index != 0)
	{
		return punit_get_team_index(unit_datum_index);
	}

	return 0;
}

int __cdecl call_unit_reset_equipment(int unit_datum_index)
{
	TRACE("unit_reset_equipment(unit_datum_index: %08X)", unit_datum_index);

	typedef int(__cdecl *unit_reset_equipment)(int unit_datum_index);
	unit_reset_equipment punit_reset_equipment = (unit_reset_equipment)(((char*)h2mod->GetBase()) + 0x1441E0);

	if (unit_datum_index != -1 && unit_datum_index != 0)
	{
		return punit_reset_equipment(unit_datum_index);
	}

	return 0;
}

int __cdecl call_hs_object_destroy(int object_datum_index)
{
	TRACE("hs_object_destory(object_datum_index: %08X)", object_datum_index);
	typedef int(__cdecl *hs_object_destroy)(int object_datum_index);
	hs_object_destroy phs_object_destroy = (hs_object_destroy)(((char*)h2mod->GetBase()) + 0x136005);

	return phs_object_destroy(object_datum_index);
}

signed int __cdecl call_unit_inventory_next_weapon(unsigned short unit)
{
	TRACE("unit_inventory_next_weapon(unit: %08X)", unit);
	typedef signed int(__cdecl *unit_inventory_next_weapon)(unsigned short unit);
	unit_inventory_next_weapon punit_inventory_next_weapon = (unit_inventory_next_weapon)(((char*)h2mod->GetBase()) + 0x139E04);

	return punit_inventory_next_weapon(unit);
}

bool __cdecl call_assign_equipment_to_unit(int unit, int object_index, short unk)
{
	TRACE("assign_equipment_to_unit(unit: %08X,object_index: %08X,unk: %08X)", unit, object_index, unk);
	typedef bool(__cdecl *assign_equipment_to_unit)(int unit, int object_index, short unk);
	assign_equipment_to_unit passign_equipment_to_unit = (assign_equipment_to_unit)(((char*)h2mod->GetBase()) + 0x1442AA);


	return passign_equipment_to_unit(unit, object_index, unk);
}

int __cdecl call_object_placement_data_new(void* s_object_placement_data, int object_definition_index, int object_owner, int unk)
{

	TRACE("object_placement_data_new(s_object_placement_data: %08X,",s_object_placement_data);
	TRACE("object_definition_index: %08X, object_owner: %08X, unk: %08X)", object_definition_index, object_owner, unk);

	typedef int(__cdecl *object_placement_data_new)(void*, int, int, int);
	object_placement_data_new pobject_placement_data_new = (object_placement_data_new)(((char*)h2mod->GetBase()) + 0x132163);


	return pobject_placement_data_new(s_object_placement_data, object_definition_index, object_owner, unk);
}

signed int __cdecl call_object_new(void* pObject)
{
	TRACE("object_new(pObject: %08X)", pObject);
	typedef int(__cdecl *object_new)(void*);
	object_new pobject_new = (object_new)(((char*)h2mod->GetBase()) + 0x136CA7);
	
	return pobject_new(pObject);
}

#pragma endregion

void GivePlayerWeapon(int PlayerIndex, int WeaponId)
{
	TRACE("GivePlayerWeapon(PlayerIndex: %08X, WeaponId: %08X)", PlayerIndex, WeaponId);

	int unit_datum = h2mod->get_unit_datum_from_player_index(PlayerIndex);

	if (unit_datum != -1 && unit_datum != 0)
	{
		char* nObject = new char[0xC4];
		DWORD dwBack;
		VirtualProtect(nObject, 0xC4, PAGE_EXECUTE_READWRITE, &dwBack);

		call_object_placement_data_new(nObject, WeaponId, unit_datum, 0);

		int object_index = call_object_new(nObject);

		call_unit_reset_equipment(unit_datum);

		call_assign_equipment_to_unit(unit_datum, object_index, 1);
	}

}

//only works in-game
void H2MOD::get_player_name(int playerIndex, char* buffer, int size) {
	//30002B5C (player1 gt)
	//0x204 difference from gt to each gt
	wchar_t _buffer[64];
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)((0x30002B5C + (playerIndex * 0x204))), &_buffer, sizeof(_buffer) / 2, NULL);
	wcstombs(buffer, _buffer, size);
}

void H2MOD::get_player_name2(int playerIndex, char* buffer, int size) {
	wchar_t* _buffer;

	//halo2.exe+51A638
	DWORD* playerNamePtr = (DWORD*)(this->GetBase() + 0x51A638);
	//difference between players
	playerNamePtr += (playerIndex * 0xB8);
	_buffer = (wchar_t*)playerNamePtr;
	wcstombs(buffer, _buffer, size);
}

IN_ADDR H2MOD::get_player_ip(int playerIndex) {
	//only server should ask for ip's
	int offset = 0x5057A8;

	BYTE* ipPtr = (BYTE*)(this->GetBase() + offset + (0x10c * playerIndex));
	IN_ADDR playerAddr = {};
	playerAddr.S_un.S_un_b.s_b1 = ipPtr[0];
	playerAddr.S_un.S_un_b.s_b2 = ipPtr[1];
	playerAddr.S_un.S_un_b.s_b3 = ipPtr[2];
	playerAddr.S_un.S_un_b.s_b4 = ipPtr[3];
	return playerAddr;
}

DWORD H2MOD::get_generated_id(int playerIndex) {
	//get the starting address for the first player xuid
	//case address to a unsigned long* (DWORD), then dereference it, so we get the current address
	//move to the given player index
	//TODO: need offset otherwise it won't work on every server
	return *(((DWORD*)(0x30002B44) + (playerIndex * 0x204)));
	//return (*(DWORD*)player_table_ptr);
}

int H2MOD::get_dynamic_player_base(int playerIndex) {
	int tempSight = get_unit_datum_from_player_index(playerIndex);
	int dynamicBase = playerIndexToDynamicBase[playerIndex];
	if (tempSight != -1 && tempSight != 0 && dynamicBase <= 0) {
		//TODo: this is based on some weird implementation in HaloObjects.cs, need just to find real offsets to dynamic player pointer
		//instead of this garbage
		for (int i = 0; i < 2048; i++) {
			int dynamicBase2 = *(DWORD*)(0x3003CF3C + (i * 12) + 8);
			DWORD* possiblyDynamicBasePtr = (DWORD*)(dynamicBase2 + 0x3F8);
			if (possiblyDynamicBasePtr) {
				//TODo: this sometimes fails, cause this whole impl is garbage
				try {
					int dynamicS = *possiblyDynamicBasePtr;
					if (tempSight == dynamicS) {
						return dynamicBase2;
						//playerIndexToDynamicBase[playerIndex] = dynamicBase;
						break;
					}
				}
				catch (...) {
					TRACE_GAME_N("Exception occured trying to read player dynamic base at index %d", i);
					continue;
				}
			}
		}
	}
	return -1;
}

int H2MOD::get_player_count() {
	return *((int*)(0x30004B60));
}

float H2MOD::get_player_x(int playerIndex) {
	int base = get_dynamic_player_base(playerIndex);
	if (base != -1) {
		return *(float*)(base + 0x64);
	}
	return 0.0f;
}

float H2MOD::get_player_y(int playerIndex) {
	int base = get_dynamic_player_base(playerIndex);
	if (base != -1) {
		return *(float*)(base + 0x68);
	}
	return 0.0f;
}

float H2MOD::get_player_z(int playerIndex) {
	int base = get_dynamic_player_base(playerIndex);
	if (base != -1) {
		return *(float*)(base + 0x6C);
	}
	return 0.0f;
}

XUID H2MOD::get_xuid(int playerIndex) {
	//0x01BD8F68
	return *(((XUID*)(this->GetBase() + 0x968F68 + (playerIndex * 8))));
}

BYTE H2MOD::get_team_id(int playerIndex) {
	//halo2.exe+2fc22bd8+288
	int address = (0x30002BD8 + (playerIndex * 0x204));
	//int address = (this->GetBase() + 0x2fc22bd8 + (playerIndex * 0x204));
	TRACE_GAME_N("playerindex-%d, baseAddress=%d, baseAddressHex=0x%08x, address=%d, addressHex=0x%08x", playerIndex, this->GetBase(), this->GetBase(), address, address);
	return *(((BYTE*)address));
}

/*
 * Checks if the given players are on the same team
 */
BOOL H2MOD::is_same_team(int p1, int p2) {
	BYTE p1Team = h2mod->get_team_id(p1);
	BYTE p2Team = h2mod->get_team_id(p2);
	return p1Team == p2Team;
}

int H2MOD::get_unit_team_index(int pIndex) {
	return call_unit_get_team_index(get_unit_datum_from_player_index(pIndex));
}

int H2MOD::get_unit_datum_from_player_index(int pIndex)
{
	int unit = 0;
	DWORD player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	player_table_ptr += 0x44;

	unit = (int)*(int*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28);
	
	return unit;
}

int H2MOD::get_unit_from_player_index(int pIndex)
{
	int unit = 0;
	DWORD player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	player_table_ptr += 0x44;

	unit = (int)*(short*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28);

	return unit;
}


typedef bool(__cdecl *spawn_player)(int a1);
spawn_player pspawn_player;

typedef bool(__cdecl *membership_update_network_decode)(int a1, int a2, int a3);
membership_update_network_decode pmembership_update_network_decode;

//sub_1cce9b
typedef int(__stdcall *calls_session_boot)(void*, int, char);
calls_session_boot calls_session_boot_method;

int __stdcall calls_session_boot_sub_1cce9b(void* thisx, int a2, char a3) {
	//a3 is 1 or 0
	//when i got booted it was 1
	//a2 mighttt be player index
	TRACE_GAME_N("session boot - this=%d,a2=%d,a3=%d", thisx, a2, a3);
	return calls_session_boot_method(thisx, a2, a3);
}

void H2MOD::kick_player(int playerIndex) {
	//enable boot menu
	//*(((BYTE*)(0x30002B1C) + (playerIndex * 0x204))) = 1;

	//only works on my machine
	//DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x00505720);
	//don't work
	//DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x008578E0);
	//DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x5178E0);
	//halo2.exe+420FE8 points to the pointer needed here
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x420FE8);
	TRACE_GAME_N("about to kick player index=%d", playerIndex);
	calls_session_boot_method((DWORD*)(*ptr), playerIndex, (char)0x01);
}

//0x8F524
typedef int(__cdecl *free_halo_string)(LPVOID lpMem);
free_halo_string free_halo_string_method = (free_halo_string)(h2mod->GetBase() + 0x8F524);

typedef int(__cdecl *write_inner_chat_text)(int a1, unsigned int a2, int a3);
write_inner_chat_text write_inner_chat_text_method;

int __cdecl write_inner_chat_hook(int a1, unsigned int a2, int a3) {
	return write_inner_chat_text_method(a1, a2, a3);
}

//can write literal and dynamic wchar_t's
void H2MOD::write_inner_chat_dynamic(const wchar_t* data) {
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x00973ac8);

	int a3 = (int)&(*data);
	void* v5 = ptr;
	const unsigned __int16* v3 = (const unsigned __int16*)(a3 - 20);
	int v8 = *(DWORD*)v5;
	*((BYTE*)v5 + 7684) = 1;
	*((DWORD*)v5 + 2) = v8;
	int v10 = *(DWORD*)v5;
	int v11 = (*(DWORD*)v5)++ % 30;
	int v12 = v10 % 30;

	if (*((DWORD *)v5 + v10 % 30 + 4)) {
		free_halo_string_method(*((LPVOID *)v5 + v11 + 4));
	}
	//size in bytes
	unsigned int v13 = wcslen(data) + 256;
	LPVOID v14 = HeapAlloc(GetProcessHeap(), 0, 2 * v13);
	*((DWORD *)v5 + v12 + 4) = (DWORD)v14;

	//where the string is located
	int result = write_inner_chat_text_method(*((DWORD *)v5 + v12 + 4), v13, a3);
}

//sub_1458759
typedef int(__stdcall *write_chat_text)(void*, int);
write_chat_text write_chat_text_method;

int __stdcall write_chat_hook(void* pObject, int a2) {
	/*
	sub_14A7567((int)&v14, 0x79u, a2);            // this function def populates v14 with whatevers in the input component
  sub_13D1855((int)&v7);
  v5 = sub_142B37B(); //can use this to get the this* object we need to even utilize write_chat_text_method
  return sub_1458759(v5, (int)&v7);             // this method that gets invoked here has logic in that that appends the GamerTag : or Server : to the chatbox line
	*/

	//to get the current clients name, call sub_11B57CA(*(_DWORD *)(v2 + 8)); replace v2 with a2
	//which is at method offset 0x2157CA

	wchar_t* chatStringWChar = (wchar_t*)(a2 + 20);
	char chatStringChar[119];
	wcstombs(chatStringChar, chatStringWChar, 119);
	TRACE_GAME_N("chat_string=%s", chatStringChar);
	std::string str(chatStringChar);
	char c = str.c_str()[0];
	if (c == '$') {
		//if this is a command, treat it differently
		h2mod->handle_command(str);
		return 0;
	}	else {
		return write_chat_text_method(pObject, a2);
	}
}

void H2MOD::handle_command(std::string command) {
	commands->handle_command(command);
}

int H2MOD::write_chat_dynamic(const wchar_t* data) {
	//		0x01b93ac8-h2mod->GetBase()	0x00973ac8	unsigned long
	//		0x0018f734-h2mod->GetBase()	0xfef6f734	unsigned long
	//		(DWORD*)(((char*)h2mod->GetBase()) + 0xfef6f734)	0x0018f734	unsigned long *
	//		(void*)(DWORD*)(((char*)h2mod->GetBase()) + 0xfef6f734)	0x0018f734	void *

	//seems to work for in game lobby and pre game lobby
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x00973ac8);
	//function at halo2.exe+0x002876Cf, adds 2 onto the data address at some point
	//in the detour, we need to add the original 20 that gets added as well (see write_chat_literal)

	//TODO: draws a white square unfortunately...
	return write_chat_text_method(ptr, (int)&(*data) - 22);
}

int H2MOD::write_chat_literal(const wchar_t* data) {
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x00973ac8);
	return write_chat_text_method(ptr, (int)&(*data) - 20);
}

typedef int(__cdecl *game_difficulty_get_real_evaluate)(int a1, int a2);
game_difficulty_get_real_evaluate pgame_difficulty_get_real_evaluate;

typedef int(__cdecl *map_intialize)(int a1);
map_intialize pmap_initialize;

typedef char(__cdecl *player_death)(int unit_datum_index, int a2, char a3, char a4);
player_death pplayer_death;


char __cdecl OnPlayerDeath(int unit_datum_index, int a2, char a3, char a4)
{
	TRACE("OnPlayerDeath(unit_datum_index: %08X)", unit_datum_index);

#pragma region GunGame Handler
	if (b_GunGame == 1)
		gg->PlayerDied(unit_datum_index);
#pragma endregion

	return pplayer_death(unit_datum_index, a2, a3, a4);
}

typedef void(__stdcall *update_player_score)(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6);
update_player_score pupdate_player_score;

void __stdcall OnPlayerScore(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6)
{
	TRACE("update_player_score_hook ( thisptr: %08X, a2: %i, a3: %i, a4: %i, a5: %i, a6: %i )", thisptr, a2, a3, a4, a5, a6);


#pragma region GunGame Handler
	if (a5 == 7) //player got a kill?
	{
		int PlayerIndex = a2;
		if (b_GunGame == 1)
			gg->LevelUp(PlayerIndex);
	}

	if (a5 == -1 && a4 == -1)
	{
		int PlayerIndex = a2;
		if (b_GunGame == 1)
			gg->LevelDown(PlayerIndex);
	}
#pragma endregion

	return pupdate_player_score(thisptr, a2, a3, a4, a5, a6);
}

bool first_load = true;
bool bcoop = false;

// This whole hook is called every single time a map loads,
// I've written a PHP script to compile the byte arrays due to the fact comparing unicode is a bitch.
// Basically if we have a single player map we set bcoop = true so that the coop variables are setup.

int __cdecl OnMapLoad(int a1)
{

#pragma region GunGame Handler
 if(b_GunGame == 1)
	gg->Initialize();
#pragma endregion

#pragma region COOP FIXES
	bcoop = false;
	
	DWORD game_globals = *(DWORD*)(((char*)h2mod->GetBase()) + 0x482D3C);
	BYTE* engine_mode = (BYTE*)(game_globals + 8);

	BYTE quarntine_zone[86] = { 0x73, 0x00, 0x63, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x72, 
								0x00, 0x69, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x5C, 0x00, 0x6D, 0x00, 
								0x75, 0x00, 0x6C, 0x00, 0x74, 0x00, 0x69, 0x00, 0x5C, 0x00, 0x30, 
								0x00, 0x36, 0x00, 0x62, 0x00, 0x5F, 0x00, 0x66, 0x00, 0x6C, 0x00, 
								0x6F, 0x00, 0x6F, 0x00, 0x64, 0x00, 0x7A, 0x00, 0x6F, 0x00, 0x6E, 
								0x00, 0x65, 0x00, 0x5C, 0x00, 0x30, 0x00, 0x36, 0x00, 0x62, 0x00, 
								0x5F, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x6F, 0x00, 0x64, 
								0x00, 0x7A, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x65, 0x00 };

	BYTE main_menu[60] = { 0x73, 0x00, 0x63, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x72, 
						   0x00, 0x69, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x5C, 0x00, 0x75, 0x00, 0x69, 0x00, 
						   0x5C, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x6D, 0x00, 0x65, 
						   0x00, 0x6E, 0x00, 0x75, 0x00, 0x5C, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x69, 0x00, 
						   0x6E, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x75, 0x00 };

	if (!memcmp(main_menu, (BYTE*)0x300017E0, 60))
	{
		DWORD game_globals = *(DWORD*)(((char*)h2mod->GetBase()) + 0x482D3C);
		BYTE* garbage_collect = (BYTE*)(game_globals + 0xC);
		*(garbage_collect) = 1;
		MasterState = 5;
		isLobby = true;

	}
	else 
	{
		MasterState = 4;
		isLobby = false;
	}

	if (!memcmp(quarntine_zone, (BYTE*)0x300017E0, 86) && *(engine_mode) == 2 ) // check the map and if we're loading a multiplayer game (We don't want to fuck up normal campaign)
	{
		bcoop = true; // set coop mode to true because we're loading an SP map and we're trying to do so with the multiplayer engine mode set.
	}

	if (bcoop == true)
	{
	

		DWORD game_globals = *(DWORD*)(((char*)h2mod->GetBase()) + 0x482D3C);
		BYTE* coop_mode = (BYTE*)(game_globals + 0x2a4);
		BYTE* engine_mode = (BYTE*)(game_globals + 8);
		BYTE* garbage_collect = (BYTE*)(game_globals + 0xC);
		*(engine_mode) = 1;
		bcoop = true;

		if (first_load == true)
		{
			*(garbage_collect) = 4; // This is utterly broken and causes weird issues when other players start joining.
		}
		else
		{
			*(garbage_collect) = 1; // This has to be left at 5 for it to work, for some reason after the first time the host loads it seems to resolve some issues with weapons creation.
			first_load = false;
		}

	}
#pragma endregion

	//TRACE("OnMapLoad(): %s", (wchar_t*)0x300017E0); // for sanity I'd love to use an offset here assigned to a variable this will break servers...


	return pmap_initialize(a1);
}

bool __cdecl OnPlayerSpawn(int a1)
{
	


#pragma region COOP Fixes
	/* hacky coop fixes*/
	
	DWORD game_globals = *(DWORD*)(((char*)h2mod->GetBase()) + 0x482D3C);
	BYTE* garbage_collect = (BYTE*)(game_globals + 0xC);
	BYTE* coop_mode = (BYTE*)(game_globals + 0x2a4);
	BYTE* engine_mode = (BYTE*)(game_globals + 8);

	if (bcoop == true)
	{
		*(coop_mode) = 1; // Turn coop mode on before spawning the player, maybe this fixes their weapon and biped or something idk?
						  // Going to have to reverse the engine simulation function for weapon creation further.
	}
	
	int ret =  pspawn_player(a1); // This handles player spawning for both multiplayer and sinlgeplayer/coop careful with it.

	/* More hacky coop fixes*/
	if (bcoop == true)
	{
		*(coop_mode) = 0; // Turn it back off, sometimes it causes crashes if it's self on we only need it when we're spawning players.
	}
#pragma endregion
	TRACE("OnPlayerSpawn(a1: %08X)", a1);
	
	int playerIndex = a1 & 0x000FFFF;



#pragma region GunGame Handler
	if (b_GunGame == 1)
		gg->SpawnPlayer(playerIndex);
#pragma endregion

	char gamertag[32];
	h2mod->get_player_name(playerIndex, gamertag, 32);
	XUID xuid = h2mod->get_xuid(playerIndex);
	
	//TODO: just create player struct
	xuidPlayerIndexMap[xuid] = playerIndex;
	//TODO: not working right?
	nameToXuidIndexMap[gamertag] = xuid;
	nameToPlayerIndexMap[gamertag] = playerIndex;

	for (auto it = nameToPlayerIndexMap.begin(); it != nameToPlayerIndexMap.end(); ++it) {
		char* name = it->first;
		int index = it->second;

		TRACE_GAME_N("PlayerMap-PlayerName=%s, PlayerIndex=%d", name, index);
	}

	BYTE teamIndex = h2mod->get_team_id(playerIndex);
	int teamIndex2 = h2mod->get_unit_team_index(playerIndex);
	TRACE_GAME_N("player-%d gamertag:%s, teamIndex:%u, teamIndex2:%d, xuid:%llu", playerIndex, gamertag, teamIndex, teamIndex2, xuid);


	return ret;
}

/*
	We'll play sounds and perform other actions from the server here.
	We need to find a way to determine if the local client is host or not and either establish a connection to the server or..
	Start listening for packets from clients.
*/
void H2MOD::EstablishNetwork()
{

}

/* Really need some hooks here,
	??_7c_simulation_game_engine_player_entity_definition@@6B@ dd offset sub_11D0018
	
	This area will give us tons of info on the game_engine player, including object index and such linked to secure-address, xnaddr, and wether or not they're host.


	; class c_simulation_game_statborg_entity_definition: c_simulation_entity_definition;   (#classinformer)
	We'll want to hook this for infection, it handles updating teams it seems we could possibly call it after updating a team to push it to all people.

	; class c_simulation_damage_section_response_event_definition: c_simulation_event_definition;   (#classinformer)
	Damage handlers... could help for quake sounds and other things like detecting assisnations (We'll probably find this when we track medals in-game as well)

	; class c_simulation_unit_entity_definition: c_simulation_object_entity_definition, c_simulation_entity_definition;   (#classinformer)
	Ofc we want unit handling.

	; class c_simulation_slayer_engine_globals_definition: c_simulation_game_engine_globals_definition, c_simulation_entity_definition;   (#classinformer)
	Should take a look here for extended functions on scoring chances are we're already hooking one of these.

*/

//0xD114C
typedef bool(__thiscall *can_join)(int thisx);

//0xD1F47
typedef bool(__thiscall *something_with_player_reservation)(int thisx, int a2);

//0xD1FA7
typedef void(__thiscall *data_decode_string)(void* thisx, int a2, int a3, int a4);

//0xD1FFD
typedef int(__thiscall *data_decode_address)(int thisx, int a1, int a2);
data_decode_address data_decode_address_method;

//0xD1F95
typedef int(__thiscall *data_decode_id)(int thisx, int a1, int a2, int a3);
data_decode_id data_decode_id_method;

//0xD1EE5
typedef unsigned int(__thiscall *data_decode_integer)(int thisx, int a1, int a2);
data_decode_integer data_decode_integer_method;

//0x1F0CFC
typedef bool(__cdecl *tjoin_request_read)(int a1, int a2, int a3);
tjoin_request_read tjoin_request_read_method;

bool __cdecl joinRequestRead(int a1, int a2, int a3) {
	data_decode_integer dataDecodeInteger = (data_decode_integer)(h2mod->GetBase() + 0xD1EE5);
	data_decode_id dataDecodeId = (data_decode_id)(h2mod->GetBase() + 0xD1F95);
	data_decode_address dataDecodeAddress = (data_decode_address)(h2mod->GetBase() + 0xD1FFD);
	data_decode_string dataDecodeString = (data_decode_string)(h2mod->GetBase() + 0xD1F95);
	something_with_player_reservation sub_451F47_method = (something_with_player_reservation)(h2mod->GetBase() + 0xD1F47);
	can_join sub_45114C_method = (can_join)(h2mod->GetBase() + 0xD114C);

	int v3;
	unsigned int v4;
	int v5;
	int v6;
	char v7;
	int v9;
	int v10;

	v3 = a3;
	*(WORD*)a3 = dataDecodeInteger(a1, (int)"protocol", 16);
	dataDecodeId(a1, (int)"session-id", a3 + 4, 64);
	dataDecodeId(a1, (int)"join-nonce", a3 + 1312, 64);
	dataDecodeId(a1, (int)"party-nonce", a3 + 1296, 64);
	//TODO: check if machine address is banned
	dataDecodeAddress(a1, (int)"machine-address", a3 + 1320);
	v4 = dataDecodeInteger(a1, (int)"joining-player-count", 5);
	v5 = 0;

	*(DWORD*)(a3 + 12) = v4;
	if ((signed int)v4 > 0) {
		v6 = a3 + 1232;
		v9 = a3 + 144;
		v10 = a3 + 16;

		do {
			//TODO: assuming this is xuid or some other id, check if this id is banned
			dataDecodeId(a1, (int)"joining-player-id", v10, 64);
			//TODO: check if joining player name is a banned player
			dataDecodeString((void*)a1, (int)"joining-player-name", v9, 32);

			*(DWORD*)(v6 - 64) = dataDecodeInteger(a1, (int)"joining-player-skill-level", 8) - 1;
			v10 += 8;
			v9 += 64;
			*(DWORD*)v6 = dataDecodeInteger(a1, (int)"joining-player-experience", 31) - 1;
			++v5;
			v6 += 4;
		} while (v5 < *(DWORD*)(v3 + 12));
	}
	v7 = sub_451F47_method(a1, (int)"create-player-reservations");
	*(BYTE*)(v3 + 1305) = v7;
	if (v7)
		dataDecodeId(a1, (int)"player-reservation-timeout-msec", v3 + 1308, 32);
	return !(unsigned __int8)sub_45114C_method(a1) && *(DWORD*)(v3 + 12) >= 0;
}

typedef void(__stdcall *tjoin_game)(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14);
tjoin_game pjoin_game;

extern SOCKET game_sock;
extern SOCKET game_sock_1000;
extern CUserManagement User;
XNADDR join_game_xn;

IN_ADDR H2MOD::get_server_address() {
	return join_game_xn.ina;
}

void __stdcall join_game(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14)
{
	
	XNADDR* host_xn = (XNADDR*)a6;
	memcpy(&join_game_xn, host_xn, sizeof(XNADDR));

	trace(L"join_game host_xn->ina.s_addr: %08X ",host_xn->ina.s_addr);
	
	sockaddr_in SendStruct;
	SendStruct.sin_addr.s_addr = host_xn->ina.s_addr;
	SendStruct.sin_port = htons(1001); // These kinds of things need to be fixed too cause we would have the port in the XNADDR struct...
	SendStruct.sin_family = AF_INET;

	int securitysend_1001 = sendto(game_sock, (char*)User.SecurityPacket, 8, 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));

	User.CreateUser(host_xn);

	if (securitysend_1001 == SOCKET_ERROR )
	{
		TRACE("join_game Security Packet - Socket Error True");
		TRACE("join_game Security Packet - WSAGetLastError(): %08X", WSAGetLastError());
	}

	return pjoin_game(thisptr, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

typedef int(__cdecl *tconnect_establish_write)(void* a1, int a2, int a3);
tconnect_establish_write pconnect_establish_write;

int __cdecl connect_establish_write(void* a1, int a2, int a3)
{
	sockaddr_in SendStruct;
	SendStruct.sin_addr.s_addr = join_game_xn.ina.s_addr;
	SendStruct.sin_port = htons(1000); // Like said about these kinds of things need to be fixed... I have the XNADDR struct we just need to pull teh port.
	SendStruct.sin_family = AF_INET;

	int securitysend_1000 = sendto(game_sock_1000, (char*)User.SecurityPacket, 8, 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));

	return pconnect_establish_write(a1, a2, a3);
}


void H2MOD::ApplyHooks()
{

	/* Should store all offsets in a central location and swap the variables based on h2server/halo2.exe*/
	/* We also need added checks to see if someone is the host or not, if they're not they don't need any of this handling. */
	if (this->Server == false)
	{
		TRACE_GAME("Applying client hooks...");
		/* These hooks are only built for the client, don't enable them on the server! */
		DWORD dwBack;

		pjoin_game = (tjoin_game)DetourClassFunc((BYTE*)this->GetBase() + 0x1CDADE, (BYTE*)join_game, 13);
		VirtualProtect(pjoin_game, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pspawn_player = (spawn_player)DetourFunc((BYTE*)this->GetBase() + 0x55952, (BYTE*)OnPlayerSpawn, 6);
		VirtualProtect(pspawn_player, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pmap_initialize = (map_intialize)DetourFunc((BYTE*)this->GetBase() + 0x5912D, (BYTE*)OnMapLoad, 10);
		VirtualProtect(pmap_initialize, 4, PAGE_EXECUTE_READWRITE, &dwBack);
		
		pupdate_player_score = (update_player_score)DetourClassFunc((BYTE*)this->GetBase() + 0xD03ED, (BYTE*)OnPlayerScore,12);
		VirtualProtect(pupdate_player_score, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pplayer_death = (player_death)DetourFunc((BYTE*)this->GetBase() + 0x17B674, (BYTE*)OnPlayerDeath, 9);
		VirtualProtect(pplayer_death, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		pconnect_establish_write = (tconnect_establish_write)DetourFunc((BYTE*)this->GetBase() + 0x1F1A2D, (BYTE*)connect_establish_write, 5);
		VirtualProtect(pconnect_establish_write, 4, PAGE_EXECUTE_READWRITE, &dwBack);

    //lobby chatbox
		write_chat_text_method = (write_chat_text)DetourClassFunc((BYTE*)this->GetBase() + 0x238759, (BYTE*)write_chat_hook, 8);
		VirtualProtect(write_chat_text_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//detect host/client
		pjoin_game = (tjoin_game)DetourClassFunc((BYTE*)this->GetBase() + 0x1CDADE, (BYTE*)join_game, 13);
		VirtualProtect(pjoin_game, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//join request
		tjoin_request_read_method = (tjoin_request_read)DetourFunc((BYTE*)this->GetBase() + 0x1F0CFC, (BYTE*)joinRequestRead, 7);
		VirtualProtect(tjoin_request_read_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//boot method
		calls_session_boot_method = (calls_session_boot)DetourClassFunc((BYTE*)this->GetBase() + 0x1CCE9B, (BYTE*)calls_session_boot_sub_1cce9b, 8);
		VirtualProtect(calls_session_boot_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//raw log line (without Server: or GAMER_TAG: prefix)
		write_inner_chat_text_method = (write_inner_chat_text)DetourFunc((BYTE*)this->GetBase() + 0x287669, (BYTE*)write_inner_chat_hook, 8);
		VirtualProtect(write_inner_chat_text_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
	}
	



}

DWORD WINAPI Thread1( LPVOID lParam )
{
	char *binarydata = new char[0xAA8 + 1];
	FILE* binarydump = fopen("binarydump.bin", "r");
	fread(binarydata, 0xAA8, 1, binarydump);

	while (1)
	{	

		
		DWORD Base = (DWORD)GetModuleHandleA("halo2.exe");
	
		DWORD *ServerList = (DWORD*)(*(DWORD*)(Base + 0x96743C));
		if (ServerList > 0)
		{
			memcpy(ServerList, binarydata, 0xAA8);
			memcpy(ServerList + 0xAA8, binarydata, 0xAA8);
		}
		
		//fread((ServerList + 0xAA8), 0xAA8, 1, BinaryDump);
		//TRACE("ServerList: %08X\n", ServerList);
		//fwrite(ServerList, 0xAA8, 1, BinaryDump);	
	}
}

void H2MOD::Initialize()
{
	if (GetModuleHandleA("H2Server.exe"))
	{
		this->Base = (DWORD)GetModuleHandleA("H2Server.exe");
		this->Server = TRUE;
	}
	else
	{
		this->Base = (DWORD)GetModuleHandleA("halo2.exe");
		this->Server = FALSE;
		//HANDLE Handle_Of_Thread_1 = 0;
		//int Data_Of_Thread_1 = 1;
		//Handle_Of_Thread_1 = CreateThread(NULL, 0,
		//	Thread1, &Data_Of_Thread_1, 0, NULL);
	}

	TRACE_GAME("H2MOD - Initialized v0.1a");
	TRACE_GAME("H2MOD - BASE ADDR %08X", this->Base);
	TRACE_GAME("H2MOD - Initializing H2MOD Network handlers");

	Network::Initialize();

	if ( b_GunGame == 1 )
		gg->Initialize();
	
	h2mod->ApplyHooks();
	
}

DWORD H2MOD::GetBase()
{
	return this->Base;
}