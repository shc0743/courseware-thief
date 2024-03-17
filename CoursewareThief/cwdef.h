#pragma once


constexpr ULONGLONG CwIndex_MetaHeader = 0xcd8b0e38eca761a1;

struct CwIndex_MetaData
{
	ULONGLONG header;
	DWORD dwMetaVersion;
	wchar_t fileId[64];
	ULONGLONG steal_time;
	ULONGLONG file_modify_time;
	ULONGLONG fileSize;
	size_t filenameSize;
};
struct CwIndex_AppData
{
	CwIndex_MetaData meta;
	//shared_ptr<wchar_t> path;
};


void CwIndexData_InsertItem(
	const std::wstring& path,
	const std::wstring& storageId,
	ULONGLONG stealTime,
	ULONGLONG modifyTime
);
bool CwIndexData_LoadFileData();
bool CwIndexData_GetFileData(
	const std::wstring& path,
	std::vector<CwIndex_MetaData>& output
);
std::vector<std::wstring> CwIndexData_GetFileNames();



