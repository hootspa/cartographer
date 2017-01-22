#include <Windows.h>
#include "H2MOD_RawMouseInput.h"
#include "H2MOD.h"
#include "xliveless.h"
#include "Hook.h"

typedef int(__cdecl * mouse_sensitivity)(int, int);
mouse_sensitivity pmouse_sensitivity;

POINT last;

int Mouse_Sensitivity(int a1, int a2)
{
	POINT current;
	GetCursorPos(&current);

	return a1 * 0x1680;
}

void RawMouseInput::Initialize()
{
	//pmouse_sensitivity = (mouse_sensitivity)DetourFunc((BYTE*)h2mod->GetBase() + 0x61CD3, (BYTE*)Mouse_Sensitivity, 5);
	//VirtualProtect(pmouse_sensitivity, 4, PAGE_EXECUTE_READWRITE, NULL);
}
