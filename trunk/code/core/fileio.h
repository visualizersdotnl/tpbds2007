
// tpbds -- Synchronous file I/O.

#ifndef _FILE_IO_H_
#define _FILE_IO_H_

OpaqueData LoadFile(const std::string &path);
bool WriteFile(const std::string &path, const void *pSrc, size_t numBytes, bool appendTo = false);

#endif // _FILE_IO_H_
