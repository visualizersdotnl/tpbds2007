
// tpbds -- 32-bit cyclic redundancy check.

#ifndef _CRC_H_
#define _CRC_H_

// Initializes a table used by CalculateCRC32().
void InitializeCRC32();

// Calculate CRC at once.
uint32_t CalculateCRC32(const void *pData, size_t numBytes);

// Mechanism to "run" a CRC.
// Useful when sequentially processing chunks of data.
uint32_t StartCRC32();
void RunCRC32(uint32_t &runningCRC, const void *pData, size_t numBytes);
uint32_t StopCRC32(uint32_t runningCRC);

#endif // _CRC_H_
