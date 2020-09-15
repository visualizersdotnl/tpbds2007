
// tpbds -- Synchronous Windows file I/O (32-bit limited).

#ifndef _FILE_WIN_H_
#define _FILE_WIN_H_

class FileWin : public NoCopy
{
public:
	enum Position
	{
		BEGIN   = FILE_BEGIN,
		CURRENT = FILE_CURRENT,
		END     = FILE_END
	};

	static bool Exists(const std::string &path); // Also works on folders.
	static bool Delete(const std::string &path);

	FileWin() :
		m_hObject(NULL),
		m_cursor(0) {}
		
	~FileWin() 
	{
		Close();
	}

	bool Open(const std::string &path, bool readOnly, bool openAlways = false);
	bool Create(const std::string &path, bool createAlways = true);
	void Close();
	bool WriteTo(const void *pSrc, size_t numBytes, uint32_t *pRunningCRC = NULL);
	bool ReadFrom(void *pDest, size_t numBytes, uint32_t *pRunningCRC = NULL);
	bool Seek(Position origin, int numBytes);
	void Truncate();

	size_t GetSize() const;
	size_t GetCursor() const { return m_cursor; }

private:
	bool CreateFile(const std::string &path, DWORD dwDesiredAccess, DWORD dwCreationDisposition);

	HANDLE m_hObject;
	size_t m_cursor;
};

#endif // _FILE_WIN_H_
