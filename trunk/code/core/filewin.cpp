
// tpbds -- Synchronous Windows file I/O (32-bit limited).

#include "main.h"
#include "filewin.h"

// Validates HANDLE.
inline bool IsValid(HANDLE hObject) 
{
	return hObject != NULL && hObject != INVALID_HANDLE_VALUE;
}

bool FileWin::Exists(const std::string &path)
{
	WIN32_FIND_DATAA queryRes;
	HANDLE hSearch = INVALID_HANDLE_VALUE;
	hSearch = FindFirstFileA(path.c_str(), &queryRes);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		FindClose(hSearch);
		return true;
	}
	
	return false;	
}

bool FileWin::Delete(const std::string &path)
{
	return DeleteFileA(path.c_str()) != 0;
}

bool FileWin::Open(const std::string &path, bool readOnly, bool openAlways /* = false */)
{
	TPB_ASSERT(!IsValid(m_hObject));
	const DWORD dwDesiredAccess = GENERIC_READ | ((readOnly) ? 0 : GENERIC_WRITE);
	const DWORD dwCreationDisposition = (openAlways) ? OPEN_ALWAYS : OPEN_EXISTING;
	return CreateFile(path, dwDesiredAccess, dwCreationDisposition) != 0;
}

bool FileWin::Create(const std::string &path, bool createAlways /* = true */)
{
	TPB_ASSERT(!IsValid(m_hObject));
	const DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	const DWORD dwCreationDisposition = (createAlways) ? CREATE_ALWAYS : CREATE_NEW;
	return CreateFile(path, dwDesiredAccess, dwCreationDisposition) != 0;
}

void FileWin::Close() 
{ 
	if (IsValid(m_hObject)) 
		CloseHandle(m_hObject); 
	
	m_hObject = 0;
}

bool FileWin::WriteTo(const void *pSrc, size_t numBytes, uint32_t *pRunningCRC /* = NULL */)
{
	TPB_ASSERT(IsValid(m_hObject));
	TPB_ASSERT(pSrc != NULL);

	if (numBytes <= UINT32_MAX)
	{
		DWORD numBytesWritten;
		if (WriteFile(m_hObject, pSrc, (DWORD) numBytes, &numBytesWritten, NULL) != 0)
		{
			m_cursor += numBytesWritten;
			if (pRunningCRC != NULL) RunCRC32(*pRunningCRC, pSrc, numBytesWritten);
			return numBytes == numBytesWritten;
		}
	}
	else TPB_ASSERT(0); // 32-bit limit.

	return false;
}

bool FileWin::ReadFrom(void *pDest, size_t numBytes, uint32_t *pRunningCRC /* = NULL */)
{
	TPB_ASSERT(IsValid(m_hObject));
	TPB_ASSERT(pDest != NULL);

	if (numBytes <= UINT32_MAX)
	{
		DWORD numBytesRead;
		if (ReadFile(m_hObject, pDest, (DWORD) numBytes, &numBytesRead, NULL) != 0)
		{
			m_cursor += numBytesRead;
			if (pRunningCRC != NULL) RunCRC32(*pRunningCRC, pDest, numBytesRead);
			return numBytes == numBytesRead;
		}
	}
	else TPB_ASSERT(0); // 32-bit limit.

	return false;
}

bool FileWin::Seek(Position origin, int numBytes)
{
	TPB_ASSERT(IsValid(m_hObject));
	const size_t newCursor = SetFilePointer(m_hObject, numBytes, NULL, origin); 			
	if (newCursor != INVALID_SET_FILE_POINTER)
	{
		m_cursor = newCursor;
		return true;
	}
	
	return false;
}

void FileWin::Truncate()
{
	TPB_ASSERT(IsValid(m_hObject));
	SetEndOfFile(m_hObject);
}

size_t FileWin::GetSize() const
{
	TPB_ASSERT(IsValid(m_hObject));
	return GetFileSize(m_hObject, NULL); FIX_ME // Breaks if file size exceeds 32-bit.
}

// Wraps CreateFileA() for Open() and Create().
bool FileWin::CreateFile(const std::string &path, DWORD dwDesiredAccess, DWORD dwCreationDisposition)
{
	m_hObject = CreateFileA(
		path.c_str(),
		dwDesiredAccess,
		0, // Not shared.
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	return IsValid(m_hObject); 
}
