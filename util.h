#pragma once
#define BUFSIZE 512

VOID getMsg(LPWSTR msg, ULONG ErrorId = 0);
DWORD GetInfo(HANDLE driveHandle, GET_STORAGE_DEPENDENCY_FLAG flags, DWORD infoSize, PSTORAGE_DEPENDENCY_INFO* pInfo, DWORD cbSize);
PSTORAGE_DEPENDENCY_INFO GetDriveInfo(LPCWSTR physicalDrive);
VOID firstDriveNameFromVolumeName(PWCHAR volName, LPWSTR driveName);
WCHAR freeDriveLetter();
BOOL mountVolume(LPWSTR volumeName, LPWSTR driveName);
VOID openDrive(LPCWSTR path, LPWSTR driveName);
DWORD mountImageDisk(LPCWSTR path, LPWSTR volumeName);
DWORD openImageDisk(LPCWSTR path);
