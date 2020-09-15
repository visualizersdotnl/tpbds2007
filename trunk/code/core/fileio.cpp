
// tpbds -- Synchronous file I/O.

#include "main.h"
#include "filewin.h"

OpaqueData LoadFile(const std::string &path)
{
	FileWin file;
	if (file.Open(path, true))
	{
		const size_t size = file.GetSize();
		ScopedArr<Byte> pData(new Byte[size]);
		if (file.ReadFrom(pData.Get(), size))
		{
			return OpaqueData(path, pData.Extract(), size);
		}
	}

	SetLastError("Can not load file: " + path);
	return OpaqueData();
}

bool WriteFile(const std::string &path, const void *pSrc, size_t numBytes, bool appendTo /* = false */)
{
	FileWin file;
	if (file.Open(path, false, !appendTo))
	{
		if (appendTo)
			file.Seek(FileWin::END, 0);
		
		if (file.WriteTo(pSrc, numBytes))
		{
			FIX_ME // Forced truncation and close shouldn't be necessary, yet at some point it was...
			file.Truncate();
			file.Close();
			
			return true;
		}
	}

	SetLastError("Can not write " + ToString(numBytes) + " bytes to file: " + path);
	return false;
}
