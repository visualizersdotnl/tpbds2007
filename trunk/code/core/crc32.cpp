
// tpbds -- 32-bit cyclic redundancy check.

#include "main.h"

static bool s_isInitialized = false;
static uint32_t s_table[256];

void InitializeCRC32()
{
	TPB_ASSERT(!s_isInitialized);
	s_isInitialized = true;

	for (uint32_t iByte = 0; iByte < 256; ++iByte)
	{
		uint32_t CRC = iByte;
		for (unsigned int iBit = 0; iBit < 8; ++iBit)
		{
			CRC = (CRC & 1) ? CRC >> 1 ^ 0xedb88320 : CRC >> 1;
		}

		s_table[iByte] = CRC;
	}
}

uint32_t CalculateCRC32(const void *pData, size_t numBytes)
{
	uint32_t CRC = StartCRC32();
	RunCRC32(CRC, pData, numBytes);
	return StopCRC32(CRC);
}

uint32_t StartCRC32() 
{ 
	return 0xffffffff; 
}

void RunCRC32(uint32_t &runningCRC, const void *pData, size_t numBytes)
{
	TPB_ASSERT(s_isInitialized);
	TPB_ASSERT(pData != NULL || numBytes == 0);

	const Byte *pBytes = static_cast<const Byte *>(pData);
	for (size_t iByte = 0; iByte < numBytes; ++iByte)
	{
		runningCRC = (runningCRC >> 8 & 0x00ffffff) ^ s_table[(runningCRC ^ *pBytes++) & 0xff];
	}
}

uint32_t StopCRC32(uint32_t runningCRC)
{
	return runningCRC ^ 0xffffffff; 
}
