#include "stdafx.h"
#include "util.h"

VOID getMsg(LPWSTR msg, ULONG ErrorId)
{
	if (0 == ErrorId) {
		ErrorId = GetLastError();
	}
	LPCVOID Message = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		ErrorId,
		0,
		(LPWSTR)&Message,
		16,
		NULL);

	wsprintf(msg, L"%s", (LPWSTR)&Message);
	LocalFree((HLOCAL)Message);
}

DWORD GetInfo(HANDLE driveHandle, GET_STORAGE_DEPENDENCY_FLAG flags, DWORD infoSize, PSTORAGE_DEPENDENCY_INFO* pInfo, DWORD cbSize)
{
	DWORD opStatus = GetStorageDependencyInformation(
		driveHandle,
		flags,
		infoSize,
		*pInfo,
		&cbSize);
	if (opStatus == ERROR_INSUFFICIENT_BUFFER)
	{
		free(*pInfo);

		infoSize = cbSize;
		*pInfo = (PSTORAGE_DEPENDENCY_INFO)malloc(infoSize);

		memset(*pInfo, 0, infoSize);

		if (*pInfo) {
			(*pInfo)->Version = STORAGE_DEPENDENCY_INFO_VERSION_2;
			cbSize = 0;

			return GetInfo(driveHandle, flags, infoSize, pInfo, cbSize);
		}
		else {
			return NULL;
		}
	}
	else
	{
		return opStatus;
	}
}

PSTORAGE_DEPENDENCY_INFO GetDriveInfo(LPCWSTR physicalDrive)
{
	PSTORAGE_DEPENDENCY_INFO pInfo = NULL;
	DWORD infoSize;
	DWORD cbSize;
	HANDLE driveHandle = INVALID_HANDLE_VALUE;
	GET_STORAGE_DEPENDENCY_FLAG flags = GET_STORAGE_DEPENDENCY_FLAG_PARENTS;
	DWORD opStatus;
	do {
		driveHandle = CreateFile(
			physicalDrive,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
			NULL);

		if (driveHandle == INVALID_HANDLE_VALUE)
		{
			break;
		}
		infoSize = sizeof(STORAGE_DEPENDENCY_INFO);
		pInfo = (PSTORAGE_DEPENDENCY_INFO)malloc(infoSize);
		if (pInfo == NULL) {
			break;
		}
		memset(pInfo, 0, infoSize);
		pInfo->Version = STORAGE_DEPENDENCY_INFO_VERSION_2;
		cbSize = 0;
		opStatus = GetInfo(driveHandle, flags, infoSize, &pInfo, cbSize);
	} while (0);

	if (driveHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(driveHandle);
	}
	return pInfo;
}

VOID firstDriveNameFromVolumeName(PWCHAR volName, LPWSTR driveName)
{
	DWORD  charCount = MAX_PATH + 1;
	PWCHAR names = NULL;
	PWCHAR nameIdx = NULL;
	BOOL   success = FALSE;
	for (;;)
	{
		names = (PWCHAR) new BYTE[charCount * sizeof(WCHAR)];

		if (!names)
		{
			return;
		}

		success = GetVolumePathNamesForVolumeName(
			volName, names, charCount, &charCount
		);

		WCHAR msg[20];
		getMsg(msg);
		if (success)
		{
			break;
		}

		if (GetLastError() != ERROR_MORE_DATA)
		{
			break;
		}

		delete[] names;
		names = NULL;
	}

	if (success)
	{
		wsprintf(driveName, L"%s", names);
	}
}

WCHAR freeDriveLetter()
{
	DWORD drives = GetLogicalDrives();

	WCHAR ch;
	WCHAR unusedLetter = NULL;
	INT pownum;
	DWORD mask;
	DWORD rst;
	LPWSTR driveName;
	driveName = (WCHAR*)malloc(4);
	for (ch = 'C'; ch <= 'Z'; ch++) {
		pownum = ch - 'A';
		mask = (DWORD)pow(2, pownum);
		rst = drives & mask;
		if (0 == rst) {
			if (NULL == unusedLetter) {
				unusedLetter = ch;
			}
		}
	}
	return unusedLetter;
}

BOOL mountVolume(LPWSTR volumeName, LPWSTR driveName)
{
	WCHAR mountPoint[4];
	WCHAR letter = freeDriveLetter();
	wsprintf(mountPoint, L"%c:\\", letter);
	size_t len = lstrlen(volumeName);
	if ('\\' != volumeName[len - 1]) {
		lstrcat(volumeName, L"\\");
	}
	BOOL ret = SetVolumeMountPoint(mountPoint, volumeName);
	if (0 == ret) {
		WCHAR msg[20];
		getMsg(msg);
	}
	wsprintf(driveName, L"%c:\\", letter);
	return ret;
}

VOID openDrive(LPCWSTR path, LPWSTR driveName)
{
	WCHAR info[20];
	wsprintf(info, L"Mounted \"%s\" on \"%s\"", path, driveName);
	MessageBox(NULL, info, L"Confirm", NULL);
}

DWORD mountImageDisk(LPCWSTR path, LPWSTR volumeName)
{
	DWORD ret;
	HANDLE hVhd;
	VIRTUAL_STORAGE_TYPE vst =
	{
		VIRTUAL_STORAGE_TYPE_DEVICE_ISO,
		VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT
	};
	OPEN_VIRTUAL_DISK_PARAMETERS oparams;
	oparams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
	oparams.Version1.RWDepth = 0;

	ret = OpenVirtualDisk(
		&vst,
		path,
		VIRTUAL_DISK_ACCESS_ATTACH_RO | VIRTUAL_DISK_ACCESS_GET_INFO | VIRTUAL_DISK_ACCESS_DETACH,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		&oparams,
		&hVhd
	);

	if (ERROR_SUCCESS == ret) {
		ATTACH_VIRTUAL_DISK_PARAMETERS aparams;
		aparams.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
		ret = AttachVirtualDisk(
			hVhd,
			NULL,
			ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY | ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME | ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER,
			// ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY | ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME,
			NULL,
			&aparams,
			NULL
		);

		if (ERROR_SUCCESS == ret)
		{
			WCHAR physicalDrive[MAX_PATH];
			ULONG bufferSize = sizeof(physicalDrive);
			ret = GetVirtualDiskPhysicalPath(hVhd, &bufferSize, physicalDrive);

			if (ERROR_SUCCESS == ret) {
				PSTORAGE_DEPENDENCY_INFO pInfo = NULL;
				DWORD entry;
				pInfo = GetDriveInfo(physicalDrive);
				for (entry = 0; entry < pInfo->NumberEntries; entry++) {
					wsprintf(volumeName, L"%s", pInfo->Version2Entries[entry].DependentVolumeName);
				}
				if (pInfo != NULL)
				{
					free(pInfo);
				}
			}
			else {
				ULONG ErrorId = GetLastError();
				WCHAR info[20];
				wsprintf(info, L"%d", ErrorId);
				WCHAR msg[20];
				getMsg(msg, ErrorId);
			}
		}
		else
		{
			WCHAR msg[20];
			getMsg(msg);
		}

		if (hVhd != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hVhd);
		}
	}
	else
	{
		WCHAR msg[20];
		getMsg(msg);
	}

	return ret;
}

DWORD openImageDisk(LPCWSTR path)
{
	BOOL found = FALSE;
	WCHAR driveName[4];

	WCHAR volumePathName[MAX_PATH];
	GetVolumePathName(path, volumePathName, MAX_PATH);
	WCHAR volName[MAX_PATH];
	GetVolumeNameForVolumeMountPoint(volumePathName, volName, MAX_PATH);

	WCHAR fullPath[MAX_PATH];
	wsprintf(fullPath, L"%s%s", volName, &path[3]);

	DWORD ret = 1;
	WCHAR volumeName[MAX_PATH];
	HANDLE hFVol = FindFirstVolume(volumeName, sizeof(volumeName) / sizeof(WCHAR));
	do {
		UINT type = GetDriveType(volumeName);
		if (type == DRIVE_CDROM) {
			PSTORAGE_DEPENDENCY_INFO pInfo = NULL;
			DWORD entry;
			pInfo = GetDriveInfo(volumeName);
			if (pInfo != NULL) {
				for (entry = 0; entry < pInfo->NumberEntries; entry++) {
					WCHAR HostVolumeName[MAX_PATH];
					WCHAR dependentVolumeRelativePath[MAX_PATH];
					wsprintf(HostVolumeName, L"%s", pInfo->Version2Entries[entry].HostVolumeName);
					wsprintf(dependentVolumeRelativePath, L"%s", pInfo->Version2Entries[entry].DependentVolumeRelativePath);
					WCHAR dependPath[MAX_PATH];
					wsprintf(dependPath, L"%s%s", HostVolumeName, dependentVolumeRelativePath);
					if (0 == lstrcmp(dependPath, fullPath))
					{
						WCHAR DependentVolumeName[MAX_PATH];
						wsprintf(DependentVolumeName, L"%s", pInfo->Version2Entries[entry].DependentVolumeName);
						lstrcat(DependentVolumeName, L"\\");
						firstDriveNameFromVolumeName(DependentVolumeName, driveName);
						if (0 != wcslen(driveName)) {
							found = TRUE;
							break;
						}
						else
						{
							if (mountVolume(volumeName, driveName)) {
								found = TRUE;
							}
						}
					}
				}
			}
		}
		continue;
	} while (FindNextVolume(hFVol, volumeName, sizeof(volumeName) / sizeof(WCHAR)));
	FindVolumeClose(hFVol);

	if (FALSE == found) {
		DWORD mountret = mountImageDisk(path, volumeName);
		if (0 == mountret) {
			if (mountVolume(volumeName, driveName)) {
				found = TRUE;
			}
		}
	}

	if (TRUE == found) {
		openDrive(path, driveName);
		ret = 0;
	}
	return ret;
}