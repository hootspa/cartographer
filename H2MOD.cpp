#include <stdafx.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include "H2MOD.h"
#include "H2MOD_GunGame.h"
#include "H2MOD_Infection.h"
#include "Network.h"
#include "xliveless.h"
#include "Globals.h"
#include "CUser.h"
#include <h2mod.pb.h>
#include <Mmsystem.h>
#include <thread>

#include "H2Security.h"
#include "H2SecurityMod.h"


class H2SecureMod : public SecurityMod {
public:
	H2SecureMod::H2SecureMod() {
	}
	virtual DWORD getBase() {
		return h2mod->GetBase();
	}
	virtual bool isLobby() {
		return false;
	}
	virtual void trace(LPSTR message, ...) {
		va_list args;
		va_start(args, message);
		TRACE_GAME_N(message, args);
		va_end(args);
	}
};

H2MOD *h2mod = new H2MOD();
H2SecureMod* secureMod = new H2SecureMod();
SecurityUtil securityUtil(secureMod);
GunGame *gg = new GunGame();
Infection *inf = new Infection();

bool b_Infection = false;

extern bool b_GunGame;
extern CUserManagement User;
extern ULONG g_lLANIP;
extern ULONG g_lWANIP;
extern UINT g_port;
extern bool isHost;
extern HANDLE H2MOD_Network;
extern bool NetworkActive;
extern bool Connected;
extern bool ThreadCreated;
extern ULONG broadcast_server;


SOCKET comm_socket = INVALID_SOCKET;
char* NetworkData = new char[255];

HMODULE base;

extern int MasterState;


#pragma region engine calls

int __cdecl call_get_object(signed int object_datum_index, int object_type)
{
	//TRACE_GAME("call_get_object( object_datum_index: %08X, object_type: %08X )", object_datum_index, object_type);

	typedef int(__cdecl *get_object)(signed int object_datum_index, int object_type);
	get_object pget_object = (get_object)((char*)h2mod->GetBase() + 0x1304E3);

	return pget_object(object_datum_index, object_type);
}

int __cdecl call_unit_reset_equipment(int unit_datum_index)
{
	//TRACE_GAME("unit_reset_equipment(unit_datum_index: %08X)", unit_datum_index);

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
	//TRACE_GAME("hs_object_destory(object_datum_index: %08X)", object_datum_index);
	typedef int(__cdecl *hs_object_destroy)(int object_datum_index);
	hs_object_destroy phs_object_destroy = (hs_object_destroy)(((char*)h2mod->GetBase()) + 0x136005);

	return phs_object_destroy(object_datum_index);
}

signed int __cdecl call_unit_inventory_next_weapon(unsigned short unit)
{
	//TRACE_GAME("unit_inventory_next_weapon(unit: %08X)", unit);
	typedef signed int(__cdecl *unit_inventory_next_weapon)(unsigned short unit);
	unit_inventory_next_weapon punit_inventory_next_weapon = (unit_inventory_next_weapon)(((char*)h2mod->GetBase()) + 0x139E04);

	return punit_inventory_next_weapon(unit);
}

bool __cdecl call_assign_equipment_to_unit(int unit, int object_index, short unk)
{
	//TRACE_GAME("assign_equipment_to_unit(unit: %08X,object_index: %08X,unk: %08X)", unit, object_index, unk);
	typedef bool(__cdecl *assign_equipment_to_unit)(int unit, int object_index, short unk);
	assign_equipment_to_unit passign_equipment_to_unit = (assign_equipment_to_unit)(((char*)h2mod->GetBase()) + 0x1442AA);


	return passign_equipment_to_unit(unit, object_index, unk);
}

int __cdecl call_object_placement_data_new(void* s_object_placement_data, int object_definition_index, int object_owner, int unk)
{

	//TRACE_GAME("object_placement_data_new(s_object_placement_data: %08X,",s_object_placement_data);
	//TRACE_GAME("object_definition_index: %08X, object_owner: %08X, unk: %08X)", object_definition_index, object_owner, unk);

	typedef int(__cdecl *object_placement_data_new)(void*, int, int, int);
	object_placement_data_new pobject_placement_data_new = (object_placement_data_new)(((char*)h2mod->GetBase()) + 0x132163);


	return pobject_placement_data_new(s_object_placement_data, object_definition_index, object_owner, unk);
}

signed int __cdecl call_object_new(void* pObject)
{
	//TRACE_GAME("object_new(pObject: %08X)", pObject);
	typedef int(__cdecl *object_new)(void*);
	object_new pobject_new = (object_new)(((char*)h2mod->GetBase()) + 0x136CA7);
	
	return pobject_new(pObject);
}

#pragma endregion

void GivePlayerWeapon(int PlayerIndex, int WeaponId,bool bReset)
{
	//TRACE_GAME("GivePlayerWeapon(PlayerIndex: %08X, WeaponId: %08X)", PlayerIndex, WeaponId);

	int unit_datum = h2mod->get_unit_datum_from_player_index(PlayerIndex);

	if (unit_datum != -1 && unit_datum != 0)
	{
		char* nObject = new char[0xC4];
		DWORD dwBack;
		VirtualProtect(nObject, 0xC4, PAGE_EXECUTE_READWRITE, &dwBack);

		call_object_placement_data_new(nObject, WeaponId, unit_datum, 0);

		int object_index = call_object_new(nObject);
		
		if(bReset == true)
			call_unit_reset_equipment(unit_datum);

		call_assign_equipment_to_unit(unit_datum, object_index, 1);
	}

}
bool H2MOD::is_team_play() {
	//0x971A90 only works in lobby (not in game)
	//0x978CB4 works in both 
	DWORD ptr = *((DWORD*)(this->GetBase() + 0x978CB4));
	ptr += 0x1C68;

	return *(BYTE*)ptr;
}

IN_ADDR H2MOD::get_player_ip(int playerIndex) {
	/*
	int offset = 0x507C18;
	if (((DWORD)(this->GetBase() + 0x5057A8)) != 0) {
		//offset = 0x5057AC;
		offset = 0x5057A8;
	}*/

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

int H2MOD::get_dynamic_player_base(int playerIndex, bool resetDynamicBase) {
	int tempSight = get_unit_datum_from_player_index(playerIndex);
	int cachedDynamicBase = playerIndexToDynamicBase[playerIndex];
	if (tempSight != -1 && tempSight != 0 && (!cachedDynamicBase || resetDynamicBase)) {
		//TODo: this is based on some weird implementation in HaloObjects.cs, need just to find real offsets to dynamic player pointer
		//instead of this garbage
		for (int i = 0; i < 2048; i++) {
			int possiblyDynamicBase = *(DWORD*)(0x3003CF3C + (i * 12) + 8);
			DWORD* possiblyDynamicBasePtr = (DWORD*)(possiblyDynamicBase + 0x3F8);
			if (possiblyDynamicBasePtr) {
				BYTE buffer[4];
				//use readprocess memory since it will set the page memory and read/write
				ReadProcessMemory(GetCurrentProcess(), (LPVOID)(possiblyDynamicBase + 0x3F8), &buffer, sizeof(buffer), NULL);
				//little endian
				int dynamicS2 = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
				if (tempSight == dynamicS2) {
					playerIndexToDynamicBase[playerIndex] = possiblyDynamicBase;
					return possiblyDynamicBase;
				}
			}
		}
	}
	return cachedDynamicBase;
}

int H2MOD::get_player_count() {
	return *((int*)(0x30004B60));
}

float H2MOD::get_distance(int playerIndex1, int playerIndex2) {
	float player1X, player1Y, player1Z;
	float player2X, player2Y, player2Z;

	player1X = h2mod->get_player_x(playerIndex1, false);
	player1Y = h2mod->get_player_y(playerIndex1, false);
	player1Z = h2mod->get_player_z(playerIndex1, false);

	player2X = h2mod->get_player_x(playerIndex2, false);
	player2Y = h2mod->get_player_y(playerIndex2, false);
	player2Z = h2mod->get_player_z(playerIndex2, false);

	float dx = abs(player1X - player2X);
	float dy = abs(player1Y - player2Y);
	float dz = abs(player1Z - player2Z);

	return sqrt(pow(dx, 2) + pow(dy, 2) + pow(dz, 2));
}

float H2MOD::get_player_x(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x64), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

float H2MOD::get_player_y(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x68), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

float H2MOD::get_player_z(int playerIndex, bool resetDynamicBase) {
	int base = get_dynamic_player_base(playerIndex, resetDynamicBase);
	if (base != -1) {
		float buffer;
		ReadProcessMemory(GetCurrentProcess(), (LPVOID)(base + 0x6C), &buffer, sizeof(buffer), NULL);
		return buffer;
	}
	return 0.0f;
}

/*
 * Checks if the given players are on the same team
 */
BOOL H2MOD::is_same_team(int p1, int p2) {
	H2Player pp1 = players->getPlayer(p1);
	H2Player pp2 = players->getPlayer(p2);

	return pp1.team == pp2.team;
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


void H2MOD::handle_command(std::string command) {
	commands->handle_command(command);
}

//0x2096AE
typedef int(*sub_2196AE)();

//0x287BA9
typedef void(__cdecl *sub_297BA9)(void* a1, int a2, unsigned int a3);

//0xD114C
//this isn't can_join, its some other shit
typedef bool(__thiscall *can_join)(int thisx);
can_join getCanJoinMethod() {
	return (can_join)(h2mod->GetBase() + 0xD114C);
}

//0xD1F47
typedef bool(__thiscall *something_with_player_reservation)(int thisx, int a2);

//0xD1FA7
typedef void(__thiscall *data_decode_string)(void* thisx, int a2, int a3, int a4);
data_decode_string getDataDecodeStringMethod() {
	return (data_decode_string)(h2mod->GetBase() + 0xD1FA7);
}

//0xD18DF
typedef void(__thiscall *data_encode_string)(void* thisx, int a2, int a3, int a4);
data_encode_string getDataEncodeStringMethod() {
	return (data_encode_string)(h2mod->GetBase() + 0xD18DF);
}

//0xD1FFD
typedef int(__thiscall *data_decode_address)(int thisx, int a1, int a2);
data_decode_address getDataDecodeAddressMethod() {
	return (data_decode_address)(h2mod->GetBase() + 0xD1FFD);
}

//0xD1918
typedef int(__thiscall *data_encode_address)(int thisx, int a1, int a2);
data_encode_address getDataEncodeAddressMethod() {
	return (data_encode_address)(h2mod->GetBase() + 0xD1918);
}

//0xD1F95
typedef int(__thiscall *data_decode_id)(int thisx, int a1, int a2, int a3);
data_decode_id getDataDecodeId() {
	return (data_decode_id)(h2mod->GetBase() + 0xD1F95);
}

//0xD18CD
typedef int(__thiscall *data_encode_id)(int thisx, int a1, int a2, int a3);
data_encode_id getDataEncodeId() {
	return (data_encode_id)(h2mod->GetBase() + 0xD18CD);
}

//0xD1EE5
typedef unsigned int(__thiscall *data_decode_integer)(int thisx, int a1, int a2);
data_decode_integer getDataDecodeIntegerMethod() {
	return (data_decode_integer)(h2mod->GetBase() + 0xD1EE5);
}

//0xD17C6
typedef int(__thiscall *data_encode_integer)(void* thisx, int a2, unsigned int a3, int a4);
data_encode_integer getDataEncodeIntegerMethod() {
	return (data_encode_integer)(h2mod->GetBase() + 0xD17C6);
}

//0xD1F47
typedef bool(__thiscall *data_decode_bool)(int thisx, int a2);
data_decode_bool getDataDecodeBoolMethod() {
	return (data_decode_bool)(h2mod->GetBase() + 0xD1F47);
}

//0xD1886
typedef bool(__thiscall *data_encode_bool)(int thisx, int a2, char a3);
data_encode_bool getDataEncodeBoolMethod() {
	return (data_encode_bool)(h2mod->GetBase() + 0xD1886);
}

//0x1ECE28
typedef int(__cdecl *write_text_chat_packet)(int a1, int a2, int a3);

//0x1E440A
typedef bool(__cdecl *player_properties_detail_read_packet)(int a1, int a2);
player_properties_detail_read_packet getPlayerPropertiesDetailReadMethod() {
	return (player_properties_detail_read_packet)(h2mod->GetBase() + 0x1E440A);
}

//0x1E42BB
typedef bool(__cdecl *player_properties_detail_write_packet)(int a1, int a2);
player_properties_detail_write_packet getPlayerPropertiesDetailWriteMethod() {
	return (player_properties_detail_write_packet)(h2mod->GetBase() + 0x1E42BB);
}

//0x1E36E7
typedef int(__cdecl *player_properties_color_read_packet)(int a1, int a2);

//0x1BA418
typedef bool(*live_check)();
live_check live_check_method;

bool clientXboxLiveCheck() {
	return false;
}

//0x1B1643
typedef signed int(*live_check2)();
live_check2 live_check_method2;

signed int clientXboxLiveCheck2() {
	//1 = turns off live? 
	//2 = either not live or can't download maps
	return 1;
}

//0x1AD782
typedef char(__cdecl *live_check3)(DWORD);
live_check3 live_check_method3;

char __cdecl clientXboxLiveCheck3(DWORD a1) {
	*(DWORD*)a1 = *(DWORD*)(h2mod->GetBase() + 0x420FE8);
	return 1;
}

//0x1AD736
typedef char(__cdecl *live_check4)(int);
live_check4 live_check_method4;

char __cdecl clientXboxLiveCheck4(int a1) {
	/*
	char result; // al@1

	result = 0;
	BYTE condition1 = *(BYTE*)(h2mod->GetBase() + 0x420FC0);
	//*(int*)(h2mod->GetBase() + 0x420FE8 + 0x73A0)
	if (condition1 && *(int*)(h2mod->GetBase() + 0x420FE8 + 0x73A0)) {
		if (a1) {
			*(DWORD*)a1 = *(DWORD*)(h2mod->GetBase() + 0x420FE8);
		}
		result = 1;
	}
	return result;*/

	if (a1) {
		*(int*)a1 = *(int*)(h2mod->GetBase() + 0x420FE8);
	}
	return 1;
}


//0x197313
typedef signed __int16(*dedi_live_check)();
dedi_live_check dedi_live_check_method;

signed __int16 dedicatedServerXboxLiveCheck() {
	return 2;
}

//0x19736F
typedef int(*dedi_live_check2)();
dedi_live_check2 dedi_live_check_method2;

int dedicatedServerXboxLiveCheck2() {
	return 2;
}

//0x197313
typedef signed int(*dedi_live_check3)();
dedi_live_check3 dedi_live_check_method3;

signed int dedicatedServerXboxLiveCheck3() {
	return 2;
}

//0x1AC596
typedef bool(*dedi_live_check4)();
dedi_live_check4 dedi_live_check_method4;

bool dedicatedServerXboxLiveCheck4() {
	return true;
}

//0x1EF6B9
typedef int(__cdecl *membership_update_write_packet)(int a1, int a2, int a3);
membership_update_write_packet membership_update_write_packet_method;

int __cdecl writeMembershipUpdate(int a1, int a2, int a3) {
	//dataEncodeInteger((void*)a1, (int)"update-number", *(DWORD *)(a3 + 8), 32);
	TRACE("Server Membership Update: Update-number = %d", *(DWORD *)(a3 + 8));
	signed int v5; // ebx@21
	int v6; // edi@22
	v5 = 0;
	v6 = 0;
	if (*(WORD *)(a3 + 34) > 0)
	{
		v6 = a3 + 6294;
		do {
			TRACE("Server Membership Update: Player-index = %d", *(WORD *)(v6 - 2));
			if (*(WORD *)v6 == 1)
			{
				WCHAR strw[8192];
				wsprintf(strw, L"%I64x", *(XUID*)(v6 + 2));
				TRACE("Server Membership Update: Player-xuid = %s", strw);

			}
			if (*(BYTE *)(v6 + 16))	{
				wchar_t* playerName = (wchar_t*)(v6 + 154);
				char pplayerName[32];
				wcstombs(pplayerName, playerName, 32);
				TRACE_GAME_N("Server Membership Update: Player-name = %s", pplayerName);
				TRACE_GAME_N("Server Membership Update: Player-team = %d", *(BYTE *)(v6 + 154 + 124));

				//hack for xuid
				WCHAR strw[8192];
				char strw3[4096];
				wsprintf(strw, L"%I64x", *(XUID*)(v6 + 2));
				wcstombs(strw3, strw, 8192);
				XUID id = _atoi64(strw3);

				//update player cache
				players->update(std::string(pplayerName), *(WORD *)(v6 - 2), *(BYTE *)(v6 + 154 + 124), id);

			}
			++v5;
			v6 += 296;
		} while (v5 < *(WORD *)(a3 + 34));
	}
	v5 = 0;
	v6 = 0;
	return membership_update_write_packet_method(a1, a2, a3);
}

//0x1EFADD
typedef bool(__cdecl *membership_update_read_packet)(int a1, int a2, int a3);
membership_update_read_packet membership_update_read_packet_method;

bool __cdecl readMembershipUpdate(int a1, int a2, int a3) {
	data_decode_integer dataDecodeInteger = getDataDecodeIntegerMethod();
	data_decode_id dataDecodeId = getDataDecodeId();
	data_decode_address dataDecodeAddress = getDataDecodeAddressMethod();
	data_decode_string dataDecodeString = getDataDecodeStringMethod();
	data_decode_bool dataDecodeBool = getDataDecodeBoolMethod();
	can_join sub_45114C_method = getCanJoinMethod();
	player_properties_detail_read_packet readPlayerPropertiesDetail = getPlayerPropertiesDetailReadMethod();

	bool v3; // bl@1
	bool v4; // al@2
	__int16 v5; // ax@5
	__int16 v6; // cx@5
	int v7; // edi@10
	__int16 v8; // ax@12
	__int16 v9; // ax@19
	bool v10; // al@23
	bool v11; // al@24
	bool v12; // al@26
	unsigned int v13; // eax@27
	signed int v14; // ecx@28
	bool v15; // al@38
	unsigned int v16; // eax@39
	signed int v17; // ecx@40
	signed int v18; // ecx@42
	int v19; // edi@50
	__int16 v20; // ax@51
	__int16 v21; // cx@52
	__int16 v22; // ax@62
	bool v23; // al@66
	unsigned int v24; // eax@67
	bool v25; // bl@70
	bool v26; // al@72
	bool v27; // bl@74
	bool v28; // al@76
	bool v29; // al@84
	bool v30; // al@86
	int v31; // eax@90
	bool result; // al@92
	signed int v33; // [sp+10h] [bp-4h]@9
	signed int v34; // [sp+10h] [bp-4h]@49

	v3 = 1;
	dataDecodeId(a1, (int)"session-id", a3, 64);
	*(DWORD *)(a3 + 8) = dataDecodeInteger(a1, (int)"update-number", 32);
	TRACE("Client Membership Update: Update-number = %d", *(DWORD *)(a3 + 8));
	if (dataDecodeBool(a1, (int)"complete-update")) {
		*(DWORD *)(a3 + 12) = -1;
		v4 = dataDecodeBool(a1, (int)"server-xuid-valid");
		*(BYTE *)(a3 + 16) = v4;
		if (v4)
			dataDecodeId(a1, (int)"server-xuid", a3 + 24, 64);
	} else {
		*(DWORD *)(a3 + 12) = dataDecodeInteger(a1, (int)"incremental-update-number", 32);
	}
	*(WORD *)(a3 + 32) = dataDecodeInteger(a1, (int)"peer-count", 5);
	v5 = dataDecodeInteger(a1, (int)"player-count", 5);
	v6 = *(WORD *)(a3 + 32);
	*(WORD *)(a3 + 34) = v5;
	if (v6 < 0 || v6 > 34 || v5 < 0 || v5 > 32)	{
		v3 = 0;
	}	else {
		v33 = 0;
		if (v6 > 0)	{
			v7 = a3 + 38;
			do {
				if (dataDecodeBool(a1, (int)"old-peer-exists"))	{
					v8 = dataDecodeInteger(a1, (int)"old-peer-index", 5);
					*(WORD *)(v7 - 2) = v8;
					v3 = v3 && v8 >= 0 && v8 < 17;
				}	else {
					*(WORD *)(v7 - 2) = -1;
				}
				if (dataDecodeBool(a1, (int)"new-peer-exists")) {
					v9 = dataDecodeInteger(a1, (int)"new-peer-index", 5);
					*(WORD *)v7 = v9;
					if (!v3 || v9 < 0 || v9 >= 17)
						v3 = 0;
				} else {
					*(WORD *)v7 = -1;
					if (!v3)
						v3 = 0;
				}
				if (*(WORD *)(v7 - 2) != -1 || *(WORD *)v7 != -1)	{
					v3 = 1;
					dataDecodeAddress(a1, (int)"peer-address", v7 + 2);
					v10 = dataDecodeBool(a1, (int)"peer-properties-updated");
					*(BYTE *)(v7 + 38) = v10;
					if (v10)
					{
						*(BYTE *)(v7 + 39) = dataDecodeBool(a1, (int)"peer-in-session");
						v11 = dataDecodeBool(a1, (int)"peer-name-updated");
						*(BYTE *)(v7 + 42) = v11;
						if (v11)
						{
							dataDecodeString((void *)a1, (int)"peer-name", v7 + 44, 16);
							dataDecodeString((void *)a1, (int)"peer-session-name", v7 + 76, 32);
						}
						v12 = dataDecodeBool(a1, (int)"peer-map-updated");
						*(BYTE *)(v7 + 140) = v12;
						if (v12)
						{
							*(DWORD *)(v7 + 142) = dataDecodeInteger(a1, (int)"peer-map-status", 3);
							v13 = dataDecodeInteger(a1, (int)"peer-map-progress-percentage", 7);
							*(DWORD *)(v7 + 146) = v13;
							v3 = v3
								&& (v14 = *(DWORD *)(v7 + 142), v14 >= 0)
								&& v14 < 6
								&& (v13 & 0x80000000) == 0
								&& (signed int)v13 <= 100;
						}
						v15 = dataDecodeBool(a1, (int)"peer-connection-updated");
						*(BYTE *)(v7 + 150) = v15;
						if (v15)
						{
							dataDecodeId(a1, (int)"estimated-downstream-bandwidth-bps", v7 + 154, 32);
							dataDecodeId(a1, (int)"estimated-upstream-bandwidth-bps", v7 + 158, 32);
							*(DWORD *)(v7 + 162) = dataDecodeInteger(a1, (int)"nat-type", 2);
							*(DWORD *)(v7 + 166) = dataDecodeInteger(a1, (int)"peer-connectivity-mask", 17);
							*(DWORD *)(v7 + 170) = dataDecodeInteger(a1, (int)"peer-latency-min", 11);
							*(DWORD *)(v7 + 174) = dataDecodeInteger(a1, (int)"peer-latency-est", 11);
							v16 = dataDecodeInteger(a1, (int)"peer-latency-max", 11);
							*(DWORD *)(v7 + 178) = v16;
							v3 = v3
								&& (v17 = *(DWORD *)(v7 + 170), v17 >= 0)
								&& v17 <= 2000
								&& (v18 = *(DWORD *)(v7 + 174), v18 >= 0)
								&& v18 <= 2000
								&& (v16 & 0x80000000) == 0
								&& (signed int)v16 <= 2000;
						}
					}
					v7 += 184;
					++v33;
				}
			} while (v33 < *(WORD *)(a3 + 32));
		}
		v34 = 0;
		if (*(WORD *)(a3 + 34) > 0) {
			v19 = a3 + 6304;
			do {
				*(WORD *)(v19 - 12) = dataDecodeInteger(a1, (int)"player-index", 4);
				TRACE("Client Membership Update: Player-index = %d", *(WORD *)(v19 - 12));
				v20 = dataDecodeInteger(a1, (int)"update-type", 2);
				*(WORD *)(v19 - 10) = v20;
				v3 = v3 && (v21 = *(WORD *)(v19 - 12), v21 >= 0) && v21 < 16 && v20 >= 0 && v20 < 3;
				if (v20 == 1)	{
					dataDecodeId(a1, (int)"identifier", v19 - 8, 64);

					WCHAR strw[8192];
					wsprintf(strw, L"%I64x", *(XUID*)(v19 - 8));
					TRACE("Client Membership Update: Player-xuid = %s", strw);

					*(WORD *)v19 = dataDecodeInteger(a1, (int)"peer-index", 5);
					//TRACE("Membership Update: Peer-index = %d", *(WORD *)v19);
					*(WORD *)(v19 + 2) = dataDecodeInteger(a1, (int)"peer-user-index", 2);
					//this seems to always be 0
					//TRACE("Membership Update: Peer-user-index = %d", *(WORD *)(v19 + 2));
					*(WORD *)(v19 - 8 + 12) = dataDecodeInteger(a1, (int)"player-flags", 1);
					v3 = v3 && *(WORD *)v19 >= 0 && *(WORD *)v19 < 17 && (v22 = *(WORD *)(v19 + 2), v22 >= 0) && v22 < 4;
				}
				v23 = dataDecodeBool(a1, (int)"properties-valid");
				*(BYTE *)(v19 + 6) = v23;
				if (v23) {
					v24 = dataDecodeInteger(a1, (int)"controller-index", 2);
					*(DWORD *)(v19 + 8) = v24;
					v25 = v3 && (v24 & 0x80000000) == 0 && (signed int)v24 < 4;
					v26 = readPlayerPropertiesDetail(a1, v19 + 12);
					v27 = v25 && v26;
					v28 = readPlayerPropertiesDetail(a1, v19 + 144);

					//player name and team index are decoded at this point
					wchar_t* playerName = (wchar_t*)(v19 + 144);
					char pplayerName[32];
					wcstombs(pplayerName, playerName, 32);
					TRACE_GAME_N("Client Membership Update: Player-name = %s", pplayerName);
					TRACE_GAME_N("Client Membership Update: Player-team = %d", *(BYTE *)(v19 + 144 + 124));

					//hack for xuid
					WCHAR strw[8192];
					char strw3[4096];
					wsprintf(strw, L"%I64x", *(XUID*)(v19 - 8));
					wcstombs(strw3, strw, 8192);
					XUID id = _atoi64(strw3);

					//update player cache
					players->update(std::string(pplayerName), *(WORD *)(v19 - 12), *(BYTE *)(v19 + 144 + 124), id);

					v3 = v27 && v28;
					*(DWORD *)(v19 + 276) = dataDecodeInteger(a1, (int)"player-voice", 32);
					*(DWORD *)(v19 + 280) = dataDecodeInteger(a1, (int)"player-text-chat", 32);
				}
				v19 += 296;
				++v34;
			} while (v34 < *(WORD *)(a3 + 34));
		}
	}
	v29 = dataDecodeBool(a1, (int)"leader-updated");
	*(BYTE *)(a3 + 15764) = v29;
	if (v29)
		*(DWORD *)(a3 + 15768) = dataDecodeInteger(a1, (int)"leader-peer_index", 5);
	v30 = dataDecodeBool(a1, (int)"virtual-couch-host-updated");
	*(BYTE *)(a3 + 15772) = v30;
	if (v30)
		*(DWORD *)(a3 + 15776) = dataDecodeInteger(a1, (int)"virtual-couch-host-peer-index", 5);
	result = 0;
	if (v3 && !sub_45114C_method(a1))
	{
		v31 = *(DWORD *)(a3 + 12);
		if (v31 == -1 || v31 < *(DWORD *)(a3 + 8))
			result = 1;
	}
	return result;
}

//0x1ECEEB
typedef bool(__cdecl *read_text_chat_packet)(int a1, int a2, int a3);
read_text_chat_packet read_text_chat_packet_method;

bool __cdecl readTextChat(int a1, int a2, int a3) {
	//TODO: from this method you can determine if the server sent you the message
	//could open up scripting

	//TODO: any pieces of text with "$" in front, ignore, since this is the client trying to possibly
	//send a malicious command
	data_decode_integer dataDecodeInteger = getDataDecodeIntegerMethod();
	data_decode_id dataDecodeId = getDataDecodeId();
	data_decode_address dataDecodeAddress = getDataDecodeAddressMethod();
	data_decode_string dataDecodeString = getDataDecodeStringMethod();
	data_decode_bool dataDecodeBool = getDataDecodeBoolMethod();
	can_join sub_45114C_method = getCanJoinMethod();

	bool v3; // al@1
	unsigned int v4; // eax@3
	unsigned int v5; // ebx@4
	int v6; // ebp@5
	bool result; // al@7

	dataDecodeId(a1, (int)"session-id", a3, 64);
	*(DWORD*)(a3 + 8) = dataDecodeInteger(a1, (int)"routed-players", 32);
	*(DWORD*)(a3 + 12) = dataDecodeInteger(a1, (int)"metadata", 8);
	v3 = dataDecodeBool(a1, (int)"source-is-server");
	*(BYTE *)(a3 + 16) = v3;
	if (!v3)
		dataDecodeId(a1, (int)"source-player", a3 + 17, 64);
	v4 = dataDecodeInteger(a1, (int)"destination-player-count", 8);
	*(DWORD*)(a3 + 156) = v4;
	if (v4 > 0x10)
	{
		result = 0;
	}
	else
	{
		v5 = 0;
		if (v4)
		{
			v6 = a3 + 25;
			do
			{
				dataDecodeId(a1, (int)"destination-player", v6, 64);
				++v5;
				v6 += 8;
			} while (v5 < *(DWORD*)(a3 + 156));
		}
		dataDecodeString((void *)a1, (int)"text", a3 + 160, 121);

		char* text = (char*)(a3 + 160);
		char c = text[0];

		TRACE_GAME_N("text_chat_packet=%s", text);
		if (c == '$') {
			if (v3) {
				//TODO: if came from server, run thru server->client command handler
			} else {
				//TODO: run thru client->client command handler
			}

			text[0] = ' ';
		}
		//result = sub_45114C_method(a1) == 0;
	}
	return true;
}

//0x1F0CFC
typedef bool(__cdecl *tjoin_request_read)(int a1, int a2, int a3);
tjoin_request_read tjoin_request_read_method;

bool __cdecl joinRequestRead(int a1, int a2, int a3) {
	data_decode_integer dataDecodeInteger = getDataDecodeIntegerMethod();
	data_decode_id dataDecodeId = getDataDecodeId();
	data_decode_address dataDecodeAddress = getDataDecodeAddressMethod();
	data_decode_string dataDecodeString = getDataDecodeStringMethod();
	data_decode_bool dataDecodeBool = getDataDecodeBoolMethod();
	can_join sub_45114C_method = getCanJoinMethod();

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
	int playerAddress = dataDecodeAddress(a1, (int)"machine-address", a3 + 1320);

	BYTE* ipPtr = (BYTE*)(playerAddress);
	IN_ADDR joiningPlayerAddress = {};
	joiningPlayerAddress.S_un.S_un_b.s_b1 = ipPtr[0];
	joiningPlayerAddress.S_un.S_un_b.s_b2 = ipPtr[1];
	joiningPlayerAddress.S_un.S_un_b.s_b3 = ipPtr[2];
	joiningPlayerAddress.S_un.S_un_b.s_b4 = ipPtr[3];

	bool isAllowedAddress = BanUtility::getInstance().isValidAddressForAllowedCountries(joiningPlayerAddress);
	if (!isAllowedAddress) {
		const char* hostnameOrIP = inet_ntoa(joiningPlayerAddress);
		TRACE_GAME_N("Address is from a country that is not allowed on this server, %s", hostnameOrIP);
		return false;
	}

	bool isBanned = BanUtility::getInstance().isBannedAddress(joiningPlayerAddress);
	if (isBanned) {
		const char* hostnameOrIP = inet_ntoa(joiningPlayerAddress);
		TRACE_GAME_N("Address is banned, %s", hostnameOrIP);
		return false;
	}

	v4 = dataDecodeInteger(a1, (int)"joining-player-count", 5);
	v5 = 0;

	const char* hostnameOrIP = inet_ntoa(joiningPlayerAddress);
	*(DWORD*)(a3 + 12) = v4;
	if ((signed int)v4 > 0) {
		v6 = a3 + 1232;
		v9 = a3 + 144;
		v10 = a3 + 16;

		do {
			//TODO: assuming this is xuid or some other id, check if this id is banned
			int id = dataDecodeId(a1, (int)"joining-player-id", v10, 64);
			XUID joiningPlayerXUID = *(XUID*)id;
			if (BanUtility::getInstance().isBannedXuid(joiningPlayerXUID)) {
				TRACE_GAME_N("XUID is banned, %llu", joiningPlayerXUID);
				return false;
			}
			dataDecodeString((void*)a1, (int)"joining-player-name", v9, 1024);

			//TODO: probably not sending unicode data over internet
			wchar_t* joiningPlayerName = (wchar_t*)(v9);
			char joinPlayerName[32];
			wcstombs(joinPlayerName, joiningPlayerName, 32);

			if (BanUtility::getInstance().isBannedGamerTag(joinPlayerName)) {
				TRACE_GAME_N("Player %s is banned", joinPlayerName);
				return false;
			}
			*(DWORD*)(v6 - 64) = dataDecodeInteger(a1, (int)"joining-player-skill-level", 8) - 1;
			v10 += 8;
			v9 += 64;
			*(DWORD*)v6 = dataDecodeInteger(a1, (int)"joining-player-experience", 31) - 1;
			++v5;
			v6 += 4;
		} while (v5 < *(DWORD*)(v3 + 12));
	}
	v7 = dataDecodeBool(a1, (int)"create-player-reservations");
	*(BYTE*)(v3 + 1305) = v7;
	if (v7)
		dataDecodeId(a1, (int)"player-reservation-timeout-msec", v3 + 1308, 32);

	bool canJoin = !(unsigned __int8)sub_45114C_method(a1);
	bool areThereValidPlayers = *(DWORD*)(v3 + 12) >= 0;
	TRACE_GAME_N("sub_45114C_method=%d, validPlayersJoining=%d", canJoin, areThereValidPlayers);
	return areThereValidPlayers;
}

typedef void(__stdcall *tjoin_game)(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14);
tjoin_game pjoin_game;

//sub_1cce9b
typedef int(__stdcall *calls_session_boot)(void*, int, char);
calls_session_boot calls_session_boot_method;

int __stdcall calls_session_boot_sub_1cce9b(void* thisx, int a2, char a3) {
	TRACE_GAME_N("session boot - this=%d,a2=%d,a3=%d", thisx, a2, a3);
	return calls_session_boot_method(thisx, a2, a3);
}

void H2MOD::kick_player(int playerIndex) {
	DWORD* ptr = (DWORD*)(((char*)h2mod->GetBase()) + 0x420FE8);
	TRACE_GAME_N("about to kick player index=%d", playerIndex);
	calls_session_boot_method((DWORD*)(*ptr), playerIndex, (char)0x01);
}

typedef int(__cdecl *write_inner_chat_text)(int a1, unsigned int a2, int a3);
write_inner_chat_text write_inner_chat_text_method;

int __cdecl write_inner_chat_hook(int a1, unsigned int a2, int a3) {
	return write_inner_chat_text_method(a1, a2, a3);
}

//can write literal and dynamic wchar_t's
void H2MOD::write_inner_chat_dynamic(const wchar_t* data) {
	sub_297BA9 sub_297BA9_method = (sub_297BA9)(h2mod->GetBase() + 0x287BA9);
	sub_2196AE sub_2196AE_method = (sub_2196AE)(h2mod->GetBase() + 0x2096AE);
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
		HeapFree(GetProcessHeap(), 0, ((LPVOID *)v5 + v11 + 4));
	}
	//size in bytes
	unsigned int v13 = wcslen(data) + 256;
	LPVOID v14 = HeapAlloc(GetProcessHeap(), 0, 2 * v13);
	*((DWORD *)v5 + v12 + 4) = (DWORD)v14;
	sub_297BA9_method(v14, 0, v13);

	//write the string
	write_inner_chat_text_method(*((DWORD *)v5 + v12 + 4), v13, a3);

	*((DWORD*)v5 + 3) = sub_2196AE_method();
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
		//this be a command, treat it differently
		h2mod->handle_command(str);
		return 1;
	} else {
		return write_chat_text_method(pObject, a2);
	}
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

//0x24499F
typedef int(__stdcall *show_download_dialog)(BYTE* thisx);
show_download_dialog show_download_dialog_method;

int __stdcall showDownloadDialog(BYTE* thisx) {
	//do nothing instead, so people don't get kicked from the game
	return 0;
}

wchar_t* H2MOD::get_local_player_name()
{
	return (wchar_t*)(((char*)h2mod->GetBase()) + 0x51A638);
}

int H2MOD::get_player_index_from_name(wchar_t* playername)
{
	DWORD player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	player_table_ptr += 0x44;

	for (int i = 0; i<=16; i++)
	{
		wchar_t* comparename = (wchar_t*)(*(DWORD*)player_table_ptr + (i * 0x204) + 0x40);
		
		TRACE_GAME("[H2MOD]::get_player_index_from_name( %ws : %ws )", playername, comparename);

		if (wcscmp(comparename, playername))
		{
			return i;
		}
	}
}

wchar_t* H2MOD::get_player_name_from_index(int pIndex)
{
	DWORD player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	player_table_ptr += 0x44;

	return	(wchar_t*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x40);
}

int H2MOD::get_player_index_from_unit_datum(int unit_datum_index)
{
	int pIndex = 0;
	DWORD player_table_ptr = *(DWORD*)(this->GetBase() + 0x004A8260);
	player_table_ptr += 0x44;
	
	for (pIndex = 0; pIndex <= 16; pIndex++)
	{
		if (unit_datum_index == (int)*(int*)(*(DWORD*)player_table_ptr + (pIndex * 0x204) + 0x28))
		{
			return pIndex;
		}
	}

}

BYTE H2MOD::get_unit_team_index(int unit_datum_index)
{
	BYTE tIndex = 0;
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		tIndex = *(BYTE*)((BYTE*)unit_object + 0x13C);
	}
	return tIndex;
}

void H2MOD::set_unit_team_index(int unit_datum_index, BYTE team)
{
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{	
		*(BYTE*)((BYTE*)unit_object + 0x13C) = team;
	}
}

void H2MOD::set_unit_biped(BYTE biped,int pIndex)
{
	if (pIndex < 17)
		*(BYTE*)(((char*)0x30002BA0 + (pIndex * 0x204))) = biped;
}

void H2MOD::set_local_team_index(BYTE team)
{
	*(BYTE*)(((char*)h2mod->GetBase()) + 0x51A6B4) = team;
}

void H2MOD::set_local_grenades(BYTE type, BYTE count,int pIndex)
{
	int unit_datum_index = h2mod->get_unit_datum_from_player_index(pIndex);
	
	int unit_object = call_get_object(unit_datum_index, 3);

	if (unit_object)
	{
		if(type == GrenadeType::Frag)
			*(BYTE*)((BYTE*)unit_object + 0x252) = count;
		if(type == GrenadeType::Plasma)
			*(BYTE*)((BYTE*)unit_object + 0x253) = count;
	}

}

void H2MOD::set_unit_grenades(BYTE type, BYTE count,int pIndex, bool bReset)
{
	int unit_datum_index = h2mod->get_unit_datum_from_player_index(pIndex);
	wchar_t* pName = h2mod->get_player_name_from_index(pIndex);

	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{
		if (bReset)
			call_unit_reset_equipment(unit_datum_index);

		if (type == GrenadeType::Frag)
		{
			*(BYTE*)((BYTE*)unit_object + 0x252) = count;
			

		}
		if (type == GrenadeType::Plasma)
		{
			*(BYTE*)((BYTE*)unit_object + 0x253) = count;
		}

		H2ModPacket GrenadePak;
		GrenadePak.set_type(H2ModPacket_Type_set_unit_grenades);
		h2mod_set_grenade *gnade = GrenadePak.mutable_set_grenade();
		gnade->set_count(count);
		gnade->set_pindex(pIndex);
		gnade->set_type(type);

		char* GrenadeBuf = new char[GrenadePak.ByteSize()];
		GrenadePak.SerializeToArray(GrenadeBuf, GrenadePak.ByteSize());
		
		for (auto it = NetworkPlayers.begin(); it != NetworkPlayers.end(); ++it)
		{
			if (wcscmp(it->first->PlayerName, pName) == 0)
			{
				it->first->PacketData = GrenadeBuf;
				it->first->PacketSize = GrenadePak.ByteSize();
				it->first->PacketsAvailable = true;
			}
		}


	}
}

BYTE H2MOD::get_local_team_index()
{
	return *(BYTE*)(((char*)h2mod->GetBase() + 0x51A6B4));
}


void H2MOD::DisableSound(int sound)
{
	DWORD AddressOffset = *(DWORD*)((char*)this->GetBase() + 0x47CD54);

	TRACE_GAME("AddressOffset: %08X", AddressOffset);
	switch (sound)
	{
		case SoundType::Slayer:
			TRACE_GAME("AddressOffset+0xd7dfb4:", (DWORD*)(AddressOffset + 0xd7dfb4));
			*(DWORD*)(AddressOffset + 0xd7e05c) = -1;
			*(DWORD*)(AddressOffset + 0xd7dfb4) = -1;
		break;

		case SoundType::GainedTheLead:
			*(DWORD*)(AddressOffset + 0xd7ab34) = -1;
			*(DWORD*)(AddressOffset + 0xd7ac84) = -1;
		break;

		case SoundType::LostTheLead:
			*(DWORD*)(AddressOffset + 0xd7ad2c) = -1;
			*(DWORD*)(AddressOffset + 0xd7add4) = -1;
		break;

		case SoundType::TeamChange:
			*(DWORD*)(AddressOffset + 0xd7b9a4) = -1;
		break;
	}
}

void SoundThread(void)
{

	while (1)
	{	
		if (h2mod->SoundMap.size() > 0)
		{
			auto it = h2mod->SoundMap.begin();
			while (it != h2mod->SoundMap.end())
			{
				TRACE_GAME("[H2MOD-SoundQueue] - attempting to play sound %ws - delaying for %i miliseconds first", it->first, it->second);
				Sleep(it->second);
				PlaySound(it->first, NULL, SND_FILENAME);
				it = h2mod->SoundMap.erase(it);
			}
		}

		Sleep(100);
	}

}


typedef bool(__cdecl *membership_update_network_decode)(int a1, int a2, int a3);
membership_update_network_decode pmembership_update_network_decode;


typedef int(__cdecl *game_difficulty_get_real_evaluate)(int a1, int a2);
game_difficulty_get_real_evaluate pgame_difficulty_get_real_evaluate;

typedef int(__cdecl *map_intialize)(int a1);
map_intialize pmap_initialize;

typedef char(__cdecl *player_death)(int unit_datum_index, int a2, char a3, char a4);
player_death pplayer_death;


char __cdecl OnPlayerDeath(int unit_datum_index, int a2, char a3, char a4)
{
#pragma region GunGame Handler
	if (b_GunGame && isHost)
		gg->PlayerDied(unit_datum_index);
#pragma endregion

#pragma region Infection Handler
	if(b_Infection)
		inf->PlayerInfected(unit_datum_index);
#pragma endregion

	return pplayer_death(unit_datum_index, a2, a3, a4);
}

typedef void(__stdcall *update_player_score)(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6);
update_player_score pupdate_player_score;

void __stdcall OnPlayerScore(void* thisptr, unsigned short a2, int a3, int a4, int a5, char a6)
{
#pragma region GunGame Handler
	if (a5 == 7) //player got a kill?
	{
		int PlayerIndex = a2;
		if (b_GunGame && isHost)
			gg->LevelUp(PlayerIndex);
	}

#pragma endregion

	return pupdate_player_score(thisptr, a2, a3, a4, a5, a6);
}

bool first_load = true;
bool bcoop = false;

// This whole hook is called every single time a map loads,

int __cdecl OnMapLoad(int a1)
{
	b_Infection = false;
	b_GunGame = false;
	
	wchar_t* variant_name = (wchar_t*)(((char*)h2mod->GetBase())+0x97777C);

	TRACE_GAME("[h2mod] OnMapLoad variant name %ws", variant_name);

	if (wcsstr(variant_name, L"zombies") > 0 || wcsstr(variant_name, L"Zombies") > 0 || wcsstr(variant_name, L"Infection") > 0 || wcsstr(variant_name, L"infection") > 0)
	{
		TRACE_GAME("[h2mod] Zombies Turned on!");
		b_Infection = true;
	}

	if (wcsstr(variant_name, L"GunGame") > 0 || wcsstr(variant_name, L"gungame") > 0)
	{
		TRACE_GAME("[h2mod] GunGame Turned on!");
		b_GunGame = true;
	}

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

		TRACE("Loading mainmenu");
		isLobby = true;
		MasterState = 5;

	}
	else 
	{

		MasterState = 4;
	}
	//everytime we load a map, clear the player index dynamic base map
	h2mod->playerIndexToDynamicBase.clear();

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

	int ret = pmap_initialize(a1);


	if (MasterState == 4)
	{
		#pragma region Infection
		if(b_Infection)
			inf->Initialize();
		#pragma endregion

		#pragma region GunGame Handler
			if (b_GunGame && isHost)
				gg->Initialize();
		#pragma endregion
	}


	return ret;

}

bool __cdecl OnPlayerSpawn(int a1)
{
	//TRACE_GAME("OnPlayerSpawn(a1: %08X)", a1);

	int PlayerIndex = a1 & 0x000FFFF;

#pragma region Infection Prespawn Handler
	if(b_Infection)
		inf->PreSpawn(PlayerIndex);
#pragma endregion

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

#pragma region Infection Handler
	if(b_Infection)
		inf->SpawnPlayer(PlayerIndex);
#pragma endregion

#pragma region GunGame Handler
	if (b_GunGame && isHost)
		gg->SpawnPlayer(PlayerIndex);
#pragma endregion


	return ret;
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


extern SOCKET game_sock;
extern SOCKET game_sock_1000;
extern CUserManagement User;
XNADDR join_game_xn;

void __stdcall join_game(void* thisptr, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char a12, int a13, int a14)
{
	Connected = false;
	NetworkActive = false;
	isServer = false;

	XNADDR* host_xn = (XNADDR*)a6;
	memcpy(&join_game_xn, host_xn, sizeof(XNADDR));

	trace(L"join_game host_xn->ina.s_addr: %08X ",host_xn->ina.s_addr);
	
	sockaddr_in SendStruct;

	if (host_xn->ina.s_addr != g_lWANIP)
		SendStruct.sin_addr.s_addr = host_xn->ina.s_addr;
	else
		SendStruct.sin_addr.s_addr = g_lLANIP; 
	
	short nPort = (ntohs(host_xn->wPortOnline) + 1);

	TRACE("join_game nPort: %i", nPort);

	SendStruct.sin_port = htons(nPort);

	TRACE("join_game SendStruct.sin_port: %i", ntohs(SendStruct.sin_port));
	TRACE("join_game xn_port: %i", ntohs(host_xn->wPortOnline));

	//SendStruct.sin_port = htons(1001); // These kinds of things need to be fixed too cause we would have the port in the XNADDR struct...
	SendStruct.sin_family = AF_INET;

	int securitysend_1001 = sendto(game_sock, (char*)User.SecurityPacket, 8, 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));

	User.CreateUser(host_xn);

	if (securitysend_1001 == SOCKET_ERROR )
	{
		TRACE("join_game Security Packet - Socket Error True");
		TRACE("join_game Security Packet - WSAGetLastError(): %08X", WSAGetLastError());
	}

	int Data_of_network_Thread = 1;
	H2MOD_Network = CreateThread(NULL, 0, NetworkThread, &Data_of_network_Thread, 0, NULL);

	return pjoin_game(thisptr, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

IN_ADDR H2MOD::get_server_address() {
	return join_game_xn.ina;
}

typedef int(__cdecl *tconnect_establish_write)(void* a1, int a2, int a3);
tconnect_establish_write pconnect_establish_write;

int __cdecl connect_establish_write(void* a1, int a2, int a3)
{

	TRACE("connect_establish_write(a1: %08X,a2 %08X, a3: %08X)", a1, a2, a3);

	if (!isHost)
	{
		sockaddr_in SendStruct;

		if (join_game_xn.ina.s_addr != g_lWANIP)
			SendStruct.sin_addr.s_addr = join_game_xn.ina.s_addr;
		else
			SendStruct.sin_addr.s_addr = g_lLANIP;

		SendStruct.sin_port = join_game_xn.wPortOnline;
		SendStruct.sin_family = AF_INET;

		int securitysend_1000 = sendto(game_sock_1000, (char*)User.SecurityPacket, 8, 0, (SOCKADDR *)&SendStruct, sizeof(SendStruct));
	}
	

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
		//join request
		//TODO: move to security util
		tjoin_request_read_method = (tjoin_request_read)DetourFunc((BYTE*)this->GetBase() + 0x1F0CFC, (BYTE*)joinRequestRead, 7);
		VirtualProtect(tjoin_request_read_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
	
		//boot method
		calls_session_boot_method = (calls_session_boot)DetourClassFunc((BYTE*)this->GetBase() + 0x1CCE9B, (BYTE*)calls_session_boot_sub_1cce9b, 8);
		VirtualProtect(calls_session_boot_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//raw log line (without Server: or GAMER_TAG: prefix)
		write_inner_chat_text_method = (write_inner_chat_text)DetourFunc((BYTE*)this->GetBase() + 0x287669, (BYTE*)write_inner_chat_hook, 8);
		VirtualProtect(write_inner_chat_text_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//read text packet
		read_text_chat_packet_method = (read_text_chat_packet)DetourFunc((BYTE*)this->GetBase() + 0x1ECEEB, (BYTE*)readTextChat, 6);		
		VirtualProtect(read_text_chat_packet_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//read membership update
		membership_update_read_packet_method = (membership_update_read_packet)DetourFunc((BYTE*)this->GetBase() + 0x1EFADD, (BYTE*)readMembershipUpdate, 7);
		VirtualProtect(membership_update_read_packet_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//write membership update
		membership_update_write_packet_method = (membership_update_write_packet)DetourFunc((BYTE*)this->GetBase() + 0x1EF6B9, (BYTE*)writeMembershipUpdate, 6);
		VirtualProtect(membership_update_write_packet_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
		//
		live_check_method = (live_check)DetourFunc((BYTE*)this->GetBase() + 0x1BA418, (BYTE*)clientXboxLiveCheck, 9);
		VirtualProtect(live_check_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//
		live_check_method2 = (live_check2)DetourFunc((BYTE*)this->GetBase() + 0x1B1643, (BYTE*)clientXboxLiveCheck2, 9);
		VirtualProtect(live_check_method2, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//0x1AD782
		live_check_method3 = (live_check3)DetourFunc((BYTE*)this->GetBase() + 0x1AD782, (BYTE*)clientXboxLiveCheck3, 9);
		VirtualProtect(live_check_method3, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//0x1AD736
		live_check_method4 = (live_check4)DetourFunc((BYTE*)this->GetBase() + 0x1AD736, (BYTE*)clientXboxLiveCheck4, 9);
		VirtualProtect(live_check_method4, 4, PAGE_EXECUTE_READWRITE, &dwBack);

		//0x24499F
		show_download_dialog_method = (show_download_dialog)DetourClassFunc((BYTE*)h2mod->GetBase() + 0x24499F, (BYTE*)showDownloadDialog, 6);
		VirtualProtect(show_download_dialog_method, 4, PAGE_EXECUTE_READWRITE, &dwBack);
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

	if(isHost)
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

			memset(NetworkData, 0x00, 255);
			int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);

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
		

			if (recvresult > 0)
			{
				bool already_authed = false;
				H2ModPacket recvpak;
				recvpak.ParseFromArray(NetworkData, recvresult);


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
									wchar_t* PlayerName = new wchar_t[36];
									memset(PlayerName, 0x00, 36);
									wcscpy(PlayerName, (wchar_t*)recvpak.h2auth().name().c_str());
									

									for (auto it = h2mod->NetworkPlayers.begin(); it != h2mod->NetworkPlayers.end(); ++it)
									{
										if (wcscmp(it->first->PlayerName, PlayerName) == 0)
										{

											TRACE_GAME("[h2mod-network] This player ( %ws ) was already connected, sending them another packet letting them know they're authed already.",PlayerName);

											char* SendBuf = new char[recvpak.ByteSize()];
											recvpak.SerializeToArray(SendBuf, recvpak.ByteSize());

											sendto(comm_socket, SendBuf, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));


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

										char* SendBuf = new char[recvpak.ByteSize()];
										recvpak.SerializeToArray(SendBuf, recvpak.ByteSize());

										sendto(comm_socket, SendBuf, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

										delete[] SendBuf;
									}
								}
							}
						break;

						case H2ModPacket_Type_h2mod_ping:
							H2ModPacket pongpak;
							pongpak.set_type(H2ModPacket_Type_h2mod_pong);

							char* pongdata = new char[pongpak.ByteSize()];
							pongpak.SerializeToArray(pongdata, pongpak.ByteSize());
							sendto(comm_socket, pongdata, recvpak.ByteSize(), 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));

							delete[] pongdata;
						break;

					}
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
		authpak->set_name((char*)h2mod->get_local_player_name(),34);
		authpak->set_secureaddr(User.LocalSec);

		char* SendBuf = new char[h2pak.ByteSize()];
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

			memset(NetworkData, 0x00, 255);
			int recvresult = recvfrom(comm_socket, NetworkData, 255, 0, (sockaddr*)&SenderAddr, &SenderAddrSize);
			
			if (recvresult > 0)
			{
				H2ModPacket recvpak;
				recvpak.ParseFromArray(NetworkData, recvresult);

				if (recvpak.has_type())
				{
					switch (recvpak.type())
					{
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

			if (Connected == true)
			{
				H2ModPacket pack;
				pack.set_type(H2ModPacket_Type_h2mod_ping);

				char* SendBuf = new char[pack.ByteSize()];
				pack.SerializeToArray(SendBuf, pack.ByteSize());

				sendto(comm_socket, SendBuf, pack.ByteSize(), 0, (SOCKADDR*)&SendStruct, sizeof(SendStruct));

				delete[] SendBuf;
			}


			Sleep(500);
		}
		
	}

	return 0;
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


		std::thread SoundT(SoundThread);
		SoundT.detach();
	}

	TRACE_GAME("H2MOD - Initialized v0.1a");
	TRACE_GAME("H2MOD - BASE ADDR %08X", this->Base);

	//TRACE_GAME("H2MOD - Initializing H2MOD Network handlers");

	Network::Initialize();

	
	
	h2mod->ApplyHooks();
	securityUtil.addHooks();
	securityUtil.startScanning();
	
}

DWORD H2MOD::GetBase()
{
	return this->Base;
}
