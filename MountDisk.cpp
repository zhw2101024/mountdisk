// MountDisk.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "MountDisk.h"

extern HMODULE hInstance;

LONG APIENTRY FMExtensionProcW(HWND hwnd, WPARAM wEvent, LPARAM lParam)
{
	#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)

	LONG returnValue = 0L;

	switch (wEvent)
	{
	case FMEVENT_LOAD:
	{
		((LPFMS_LOAD)lParam)->dwSize = sizeof(FMS_LOAD);
		((LPFMS_LOAD)lParam)->hMenu = NULL;
		returnValue = (LONG)TRUE;
	}
	break;
	case FMEVENT_TOOLBARLOAD:
	{
		static EXT_BUTTON extbtn[] = { { IDM_FIRSTBUTTON, 0, 0} };
		((LPFMS_TOOLBARLOAD)lParam)->dwSize = sizeof(FMS_TOOLBARLOAD);
		((LPFMS_TOOLBARLOAD)lParam)->lpButtons = (LPEXT_BUTTON)&extbtn;
		((LPFMS_TOOLBARLOAD)lParam)->cButtons = 1;
		((LPFMS_TOOLBARLOAD)lParam)->cBitmaps = 1;
		((LPFMS_TOOLBARLOAD)lParam)->idBitmap = IDB_BTN;
		returnValue = (LONG)TRUE;
	}
	break;
	case IDM_FIRSTBUTTON:
	{
		FMS_GETFILESEL temp;
		LRESULT focus = SendMessage(hwnd, FM_GETFILESEL, 0, (LPARAM)&temp);
		LPWSTR path = temp.szName;
		DWORD ret = openImageDisk(path);
		if (0 == ret) {
			SendMessage(hwnd, FM_REFRESH_WINDOWS, 0, 0);
		}
		else {
			MessageBox(NULL, L"Failed to mount disk", L"Error", NULL);
		}
	}
	break;
	}
	return returnValue;
}