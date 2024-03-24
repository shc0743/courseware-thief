#include <Windows.h>
#include "../../resource/tool.h"
#include "cwdef.h"
#include <unordered_map>
using namespace std;


static std::mutex gmx;
static std::map<std::wstring, std::vector<CwIndex_MetaData>> file_data;


void CwIndexData_InsertItem(
	const std::wstring& path,
	const std::wstring& storageId,
	ULONGLONG stealTime,
	ULONGLONG modifyTime
) {
	gmx.lock();

	do {
		HANDLE hFile = CreateFileW(L"Files\\index.db", GENERIC_ALL, 
			FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
		if (hFile == INVALID_HANDLE_VALUE) break;

		SetFilePointerEx(hFile, LARGE_INTEGER{ .QuadPart = 0 }, NULL, FILE_END);

		CwIndex_MetaData* data0 = new CwIndex_MetaData;
		ZeroMemory(data0, sizeof(CwIndex_MetaData));
		data0->header = CwIndex_MetaHeader;
		data0->dwMetaVersion = 1;
		wcscpy_s(data0->fileId, storageId.c_str());
		data0->steal_time = stealTime;
		data0->file_modify_time = modifyTime;
		data0->fileSize = MyGetFileSizeW(path.c_str());
		data0->filenameSize = path.length() * sizeof(WCHAR);

		const void* data1 = path.c_str();

		DWORD written = 0;
		WriteFile(hFile, data0, sizeof(CwIndex_MetaData), &written, NULL);
		WriteFile(hFile, data1, (DWORD)data0->filenameSize, &written, NULL);

		CloseHandle(hFile);
		delete data0;
	} while (0);

	gmx.unlock();
}

bool CwIndexData_LoadFileData() {
	bool bResult = false;
	gmx.lock();

	do {
		file_data.clear();

		HANDLE hFile = CreateFileW(L"Files\\index.db", GENERIC_READ, 
			FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
		if (hFile == INVALID_HANDLE_VALUE) break;

		WCHAR pstr[4096]{};
		CwIndex_MetaData md{};
		DWORD toRead = sizeof(md), readed = 0;

		if (0) {
		failed:
			CloseHandle(hFile);
			break;
		}
		while (ReadFile(hFile, &md, toRead, &readed, NULL)) {
			if (readed == 0) break;

			if (CwIndex_MetaHeader != md.header) {
				SetLastError(ERROR_FILE_CORRUPT);
				goto failed;
			}
			if (md.filenameSize > 4096) {
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				goto failed;
			}
			ZeroMemory(pstr, sizeof(pstr));
			(void)ReadFile(hFile, &pstr, (DWORD)md.filenameSize, &readed, NULL);
			if (pstr[0] == 0) continue;
			wstring filename = pstr;
			if (filename.find(L"\\") != filename.npos)
				filename.erase(0, filename.find_last_of(L"\\") + 1);
			if (!file_exists(L"Files\\"s + md.fileId + L"\\" + filename))
				continue; // 忽略不存在的文件

			try {
				auto& data = file_data.at(pstr);
				// 追加data
				data.push_back(md);
			}
			catch (std::out_of_range) {
				// 创建data
				wstring sz = pstr;
				vector<CwIndex_MetaData> mds; mds.push_back(md);
				file_data.insert(make_pair(sz, mds));
			}
			
		}

		CloseHandle(hFile);
		bResult = true;
	} while (0);

	gmx.unlock();
	return bResult;
}


bool CwIndexData_GetFileData(
	const std::wstring& path,
	std::vector<CwIndex_MetaData>& output
) {
	try {
		output.clear();
		auto& data = file_data.at(path);
		output.operator=(data);
		return true;
	}
	catch (std::out_of_range) {
		return false;
	}
}

std::vector<std::wstring> CwIndexData_GetFileNames() {
	std::vector<std::wstring> vec;
	for (auto& i : file_data) vec.push_back(i.first);
	return vec;
}



