
// tpbds -- Demo resource hub.

#include "main.h"
#include "resources.h"
#include "../core/filewin.h" // Not included by default.

// -- Archive system. -- 

// A lot is done directly on the file API; since it's cached by Windows it isn't slow.
// This eliminates the tedious task of creating all structures in memory first.

ResourceHub::ArchiveWriter::~ArchiveWriter()
{
	for (size_t iFile = 0; iFile < m_files.size(); ++iFile)
	{
		delete m_files[iFile];
	}
}

void ResourceHub::ArchiveWriter::AddFile(const std::string &path)
{
	TPB_ASSERT(!m_isPrepared);
	m_files.push_back(new File(path));
}

bool ResourceHub::ArchiveWriter::Prepare()
{
	TPB_ASSERT(!m_isPrepared);

	for (size_t iFile = 0; iFile < m_files.size(); ++iFile)
	{
		if (!m_files[iFile]->Load())
		{
			return false;
		}
	}

	m_isPrepared = true;
	return true;
}

bool ResourceHub::ArchiveWriter::Write()
{
	TPB_ASSERT(m_isPrepared);

	// Allocate table.
	const size_t tableSize = 1 + 2 * m_files.size(); // In elements, not bytes.
	const ScopedArr<uint32_t> pTable(new uint32_t[tableSize]);

	// Store number of entries.
	TPB_ASSERT(m_files.size() <= UINT32_MAX); // 32-bit limit.
	pTable[0] = (uint32_t) m_files.size();
	
	// Store hashes & offsets.
	uint32_t *pWrite = pTable.Get() + 1, offset = 0;
	for (size_t iFile = 0; iFile < m_files.size(); ++iFile)
	{
		*pWrite++ = m_files[iFile]->m_hash;
		*pWrite++ = offset;
		
		const size_t entrySize = sizeof(uint32_t) + m_files[iFile]->m_ID.length() + sizeof(char) + m_files[iFile]->m_size;
		TPB_ASSERT(entrySize <= UINT32_MAX); // 32-bit limit.
		offset += (uint32_t) entrySize;
	}

	// Write archive using FileWin.
	FileWin archFile;
	if (archFile.Create(m_archivePath))
	{
		// Start global CRC32.
		uint32_t globalCRC = StartCRC32(); 
	
		// Write table.
		if (archFile.WriteTo(pTable.Get(), tableSize * sizeof(uint32_t), &globalCRC))
		{
			// Write entries.
			size_t filesWritten = 0;
			for (size_t iFile = 0; iFile < m_files.size(); ++iFile)
			{
				// Write size.
				if (!archFile.WriteTo(&m_files[iFile]->m_size, sizeof(uint32_t), &globalCRC))
					break;

				// Write ID.
				if (!archFile.WriteTo(m_files[iFile]->m_ID.c_str(), m_files[iFile]->m_ID.length() + sizeof(char), &globalCRC))
					break;

				// Write data.
				if (!archFile.WriteTo(m_files[iFile]->m_data.GetPtr(), m_files[iFile]->m_size, &globalCRC))
					break;
			
				++filesWritten;
			}
			
			// All done?
			if (filesWritten == m_files.size())
			{
				// Store global CRC32. 
				globalCRC = StopCRC32(globalCRC);
				if (archFile.WriteTo(&globalCRC, sizeof(uint32_t)))
				{
					// Close explicitly.
					archFile.Close();
				
					// Success!
					return true;
				}
			}
		}
	}

	SetLastError("Can not write archive: " + m_archivePath);
	return false;
}

bool ResourceHub::ArchiveWriter::File::Load()
{
	m_data = LoadFile(m_path);
	if (m_data.IsValid())
	{
		m_size = m_data.GetSize();
		m_ID = m_path; // Most likely a relative path.
		m_hash = CalculateCRC32(m_ID.c_str(), m_ID.length());
		return true;
	}

	return false;
}

bool ResourceHub::ArchiveLoader::Load()
{
	TPB_ASSERT(!m_isLoaded);

	bool crcMismatch = false;

	// Load archive using FileWin.
	FileWin archFile;
	if (archFile.Open(m_archivePath, true))
	{
		// Start global CRC32.
		uint32_t globalCRC = StartCRC32();

		uint32_t numEntries;
		if (archFile.ReadFrom(&numEntries, sizeof(uint32_t), &globalCRC))
		{
			#pragma pack(push, 1)
			
			// Each table entry represents a file in the data chunk.
			struct TableEntry
			{
				uint32_t hash;
				uint32_t offset;
			};
			
			#pragma pack(pop)
			
			// Load table.
			ScopedArr<TableEntry> pTable(new TableEntry[numEntries]);
			if (archFile.ReadFrom(pTable.Get(), numEntries * sizeof(TableEntry), &globalCRC))
			{
				// Load data chunk (remainder minus global CRC32).
				const size_t chunkSize = archFile.GetSize() - sizeof(uint32_t) - archFile.GetCursor();
				ScopedArr<Byte> pChunk(new Byte[chunkSize]);
				if (archFile.ReadFrom(pChunk.Get(), chunkSize, &globalCRC))
				{
					// Perform checksum test.
					uint32_t CRC;
					if (archFile.ReadFrom(&CRC, sizeof(uint32_t)))
					{
						globalCRC = StopCRC32(globalCRC);
						if (globalCRC == CRC)
						{
							// Chunk is valid: use it.
							m_pDataChunk = pChunk.Extract();
							
							// Build list of files.
							for (uint32_t iEntry = 0; iEntry < numEntries; ++iEntry)
							{
								const TableEntry &tabEntry = pTable[iEntry];
								const Byte *pFile = m_pDataChunk + tabEntry.offset;
								
								File file;
								file.hash = tabEntry.hash;
								file.size = *reinterpret_cast<const uint32_t *>(pFile);
								file.ID = std::string(reinterpret_cast<const char *>(pFile + sizeof(uint32_t)));
								file.dataOffset = tabEntry.offset + sizeof(uint32_t) + file.ID.length() + sizeof(char);
								m_files.push_back(file);
							}

							m_isLoaded = true;
							return true;
						}
						else
						{
							crcMismatch = true;
						}
					}
				}
			}
		}
	}
	
	if (!crcMismatch)
		SetLastError("Can not load archive: " + m_archivePath);
	else
		SetLastError("Archive corrupt: " + m_archivePath);

	return false;
}

OpaqueData ResourceHub::ArchiveLoader::GetFileData(const std::string &path) const
{
	TPB_ASSERT(m_isLoaded);

	const uint32_t hash = CalculateCRC32(path.c_str(), path.length());

	// A linear search is fast enough for now.
	for (size_t iFile = 0; iFile < m_files.size(); ++iFile)
	{
		if (hash == m_files[iFile].hash)
		{
			const File &file = m_files[iFile];
			return OpaqueData(file.ID, m_pDataChunk + file.dataOffset, file.size, false); // Not the owner!
		}
	}

	// Not found.
	SetLastError("File not found: " + path + ", in archive:" + m_archivePath);
	return OpaqueData();
}

// -- Resource types. --

bool ResourceHub::Texture::Create(Renderer &renderer, const ArchiveLoader *pArchLoader)
{
	const OpaqueData fileData = GetFileData(m_path, pArchLoader);
	if (fileData.IsValid())
	{
		m_pTexture = renderer.CreateTextureFromFileInMemory(fileData, m_generateMipLevels, m_noRoundingToPow2);
		return m_pTexture != NULL;
	}

	return NULL;
}

void ResourceHub::Texture::Destroy(Renderer &renderer)
{
	renderer.DestroyTexture(m_pTexture);
	m_pTexture = NULL;
}

// Wrapper for ResourceHub::Shaders.
class D3DXBuffer : public NoCopy
{
public:
	D3DXBuffer(ID3DXBuffer *pBuffer) { m_pBuffer = pBuffer; }
	~D3DXBuffer() { SAFE_RELEASE(m_pBuffer); }

	const LPVOID GetPtr()
	{
		TPB_ASSERT(m_pBuffer != NULL);
		return m_pBuffer->GetBufferPointer();
	}
	
	DWORD GetSize()
	{
		TPB_ASSERT(m_pBuffer != NULL);
		return m_pBuffer->GetBufferSize();
	}

	ID3DXBuffer *m_pBuffer;
};

bool ResourceHub::Shaders::Create(Renderer &renderer, const ArchiveLoader *pArchLoader)
{
	if (pArchLoader != NULL)
	{
		// Use precompiled bytecode.
		const OpaqueData vsBytecode = GetFileData(m_vsPath + ".bytecode", pArchLoader);
		if (vsBytecode.IsValid())
		{
			const OpaqueData psBytecode = GetFileData(m_psPath + ".bytecode", pArchLoader);
			if (psBytecode.IsValid())
			{
				m_pShaderPair = renderer.CreateShaderPair(vsBytecode.GetPtr(), psBytecode.GetPtr(), m_reqFlexVtxBits);
				return true;
			}
		}
	}
	else
	{
		// Compile shaders.
		D3DXBuffer vsBytecode(CompileShaderFromFile(m_vsPath, "vs_3_0"));
		if (vsBytecode.m_pBuffer != NULL)
		{
			D3DXBuffer psBytecode(CompileShaderFromFile(m_psPath, "ps_3_0"));
			if (psBytecode.m_pBuffer != NULL)
			{
				m_pShaderPair = renderer.CreateShaderPair(vsBytecode.GetPtr(), psBytecode.GetPtr(), m_reqFlexVtxBits);
				return true;
			}
		}
	}

	return false;
}

void ResourceHub::Shaders::Destroy(Renderer &renderer)
{
	renderer.DestroyShaderPair(m_pShaderPair);
	m_pShaderPair = NULL;
}

ID3DXBuffer *ResourceHub::Shaders::CompileShaderFromFile(const std::string &path, const std::string &profile)
{
	ID3DXBuffer *pBytecode = NULL;
	ID3DXBuffer *pErrors = NULL;

	HRESULT hRes = D3DXCompileShaderFromFileA(
		path.c_str(),
		NULL,
		NULL,
		"main",
		profile.c_str(),
		D3DXSHADER_OPTIMIZATION_LEVEL3, FIX_ME // Should be a toggle: D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION
		&pBytecode,
		&pErrors,
		NULL);

	if (hRes != D3D_OK)
	{
		if (pErrors != NULL)
		{
			const std::string compilerErrors(static_cast<char *>(pErrors->GetBufferPointer()));
			SetLastError("Can not compile shader.\n\n" + compilerErrors);
			pErrors->Release();
		}
		else
		{
			SetLastError("Can not compile shader: " + path);
		}

		return NULL;
	}
	else if (pErrors != NULL) FIX_ME // Warnings! Handle them.
	{
//		const std::string compilerWarnings(static_cast<char *>(pErrors->GetBufferPointer()));
		pErrors->Release();
	}

	return pBytecode;
}

const uint32_t ResourceHub::Mesh3DS::kFlexVertexFlags = FV_POSITION | FV_COLOR | FV_UV | FV_NORMAL | FV_TANGENT | FV_BINORMAL;

bool ResourceHub::Mesh3DS::Create(Renderer &renderer, const ArchiveLoader *pArchLoader)
{
	TPB_ASSERT(!IsCreated());

	const OpaqueData fileData = GetFileData(m_path, pArchLoader);
	if (fileData.IsValid())
	{
		m_pMesh = new Mesh(renderer, kFlexVertexFlags, false);
		if (m_pMesh->LoadFrom3DSInMemory(fileData, m_scaleTo, m_objIndex))
		{
			return true;
		}
		
		delete m_pMesh;
		m_pMesh = NULL;
	}
	
	return false;
}

void ResourceHub::Mesh3DS::Destroy(Renderer &renderer)
{
	delete m_pMesh;
	m_pMesh = NULL;
}

const uint32_t ResourceHub::MeshLWO::kFlexVertexFlags = FV_POSITION | FV_COLOR | FV_UV | FV_NORMAL | FV_TANGENT | FV_BINORMAL;

bool ResourceHub::MeshLWO::Create(Renderer &renderer, const ArchiveLoader *pArchLoader)
{
	TPB_ASSERT(!IsCreated());

	const OpaqueData fileData = GetFileData(m_path, pArchLoader);
	if (fileData.IsValid())
	{
		m_pMesh = new Mesh(renderer, kFlexVertexFlags, false);
		if (m_pMesh->LoadFromLWOInMemory(fileData))
		{
			return true;
		}
		
		delete m_pMesh;
		m_pMesh = NULL;
	}
	
	return false;
}

void ResourceHub::MeshLWO::Destroy(Renderer &renderer)
{
	delete m_pMesh;
	m_pMesh = NULL;
}

bool ResourceHub::RawData::Create(Renderer &renderer, const ArchiveLoader *pArchLoader)
{
	TPB_ASSERT(!IsCreated());

	OpaqueData fileData = GetFileData(m_path, pArchLoader);
	if (fileData.IsValid())
	{
		if (fileData.IsOwner())
		{
			// Assume ownership.
			m_opaqueData = fileData;
		}
		else
		{
			// Make a copy.
			const size_t size = fileData.GetSize();
			Byte *pData = new Byte[size];
			memcpy(pData, fileData.GetPtr(), size);
			m_opaqueData = OpaqueData(fileData.GetID(), pData, size);
		}
		
		return true;
	}

	return false;
}

void ResourceHub::RawData::Destroy(Renderer &renderer) {}

// -- Resource hub. --

ResourceHub::ResourceHub(Renderer &renderer) :
	m_renderer(renderer),
	m_loadState(NOT_LOADED) {}

ResourceHub::~ResourceHub()
{
	if (m_loadState != NOT_LOADED)
	{
		for (ResourceMap::const_iterator iResource = m_resources.begin(); iResource != m_resources.end(); ++iResource)
		{
			Resource *pResource = iResource->second;
			pResource->Destroy(m_renderer);
			delete pResource;
		}
	}
}

ResourceHub::ResourceMap::const_iterator ResourceHub::RequestResource(const std::string &path, Resource::Type type)
{
	// Still taking requests?
	TPB_ASSERT(m_loadState == NOT_LOADED);

	// Has this file already been requested?
	ResourceMap::const_iterator iResource = m_resources.find(path);
	if (iResource == m_resources.end())
	{
		// No.
		return iResource;
	}

	// Did prior request succeed?
	TPB_ASSERT(iResource->second != NULL);

	// If it did, are the types equal? If not, why are the paths identical?
	TPB_ASSERT(iResource->second->GetType() == type);

	// It's safe!
	return iResource;
}

ResourceHub::Texture *ResourceHub::RequestTexture(const std::string &path, bool generateMipLevels, bool noRoundingToPow2)
{
	ResourceMap::const_iterator iResource = RequestResource(path, Resource::TEXTURE);
	if (iResource == m_resources.end())
	{
		Texture *pTexture = new Texture(path, generateMipLevels, noRoundingToPow2);
		m_resources.insert(MapPair(path, pTexture));
		return pTexture;
	}

	return static_cast<ResourceHub::Texture *>(iResource->second);
}

ResourceHub::Shaders *ResourceHub::RequestShaders(const std::string &vsPath, const std::string &psPath, uint32_t reqFlexVtxBits)
{
	ResourceMap::const_iterator iResource = RequestResource(vsPath + psPath, Resource::SHADERS);
	if (iResource == m_resources.end())
	{
		Shaders *pShaders = new Shaders(vsPath, psPath, reqFlexVtxBits);
		m_resources.insert(MapPair(vsPath + psPath, pShaders));
		return pShaders;
	}

	return static_cast<ResourceHub::Shaders *>(iResource->second);
}

ResourceHub::Mesh3DS *ResourceHub::RequestMesh3DS(const std::string &path, float scaleTo, unsigned int objIndex /* = 0 */)
{
	ResourceMap::const_iterator iResource = RequestResource(path, Resource::MESH3DS);
	if (iResource == m_resources.end())
	{
		Mesh3DS *pMesh3DS = new Mesh3DS(path, scaleTo, objIndex);
		m_resources.insert(MapPair(path, pMesh3DS));
		return pMesh3DS;
	}

	return static_cast<ResourceHub::Mesh3DS *>(iResource->second);
}

ResourceHub::MeshLWO *ResourceHub::RequestMeshLWO(const std::string &path)
{
	ResourceMap::const_iterator iResource = RequestResource(path, Resource::MESHLWO);
	if (iResource == m_resources.end())
	{
		MeshLWO *pMeshLWO = new MeshLWO(path);
		m_resources.insert(MapPair(path, pMeshLWO));
		return pMeshLWO;
	}

	return static_cast<ResourceHub::MeshLWO *>(iResource->second);
}

ResourceHub::RawData *ResourceHub::RequestRawData(const std::string &path)
{
	ResourceMap::const_iterator iResource = RequestResource(path, Resource::RAWDATA);
	if (iResource == m_resources.end())
	{
		RawData *pRawData = new RawData(path);
		m_resources.insert(MapPair(path, pRawData));
		return pRawData;
	}

	return static_cast<ResourceHub::RawData *>(iResource->second);
}

bool ResourceHub::Load(bool loadFromArchive, const std::string &archivePath)
{
	TPB_ASSERT(m_loadState == NOT_LOADED);
	m_loadState = LOADING;

	ArchiveLoader archLoader(archivePath);
	if (loadFromArchive)
	{
		// Blocking load.
		while (!archLoader.IsLoaded())
		{
			if (!archLoader.Load())
			{
				return false;
			}
		}
	}
	
	for (ResourceMap::const_iterator iResource = m_resources.begin(); iResource != m_resources.end(); ++iResource)
	{
		Resource *pResource = iResource->second;
		TPB_ASSERT(pResource != NULL);
		if (!pResource->IsCreated()) // Resource can be created by LoadSingleResource().
		{
			if (!pResource->Create(m_renderer, (loadFromArchive) ? &archLoader : NULL))
			{
				return false;
			}
		}
	}

	m_loadState = LOADED;
	return true;
}

bool ResourceHub::WriteArchive(const std::string &path)
{
	TPB_ASSERT(IsLoaded());
	
	ArchiveWriter archWriter(path);
	std::vector<const std::string> tempFiles;

	// Add files.
	bool filesAdded = true;
	for (ResourceMap::const_iterator iResource = m_resources.begin(); iResource != m_resources.end(); ++iResource)
	{
		// Verify existence.
		TPB_ASSERT(iResource->second != NULL);

		if (iResource->second->GetType() != Resource::SHADERS)
		{
			for (unsigned int iPath = 0; iPath < iResource->second->GetNumPaths(); ++iPath)
			{
				archWriter.AddFile(iResource->second->GetPath(iPath));
			}
		}
		else // Shaders are a special case because compiled bytecode should be stored, not the source.
		{
			// Downcast!
			ResourceHub::Shaders *pShaders = static_cast<ResourceHub::Shaders *>(iResource->second);

			const std::string *pPathStrings[2] = { &pShaders->m_vsPath, &pShaders->m_psPath };
			const std::string profiles[2] = { "vs_3_0", "ps_3_0" };
			
			for (unsigned int iShader = 0; iShader < 2; ++iShader)
			{
				const std::string tempPath = *pPathStrings[iShader] + ".bytecode";

				// Linear search check to ensure we don't duplicate.
				// This is secure because all supplied paths are either relative or fully absolute.
				bool isDupe = false;
				for (size_t iTempFile = 0; iTempFile < tempFiles.size(); ++iTempFile)
				{
					if (tempPath == tempFiles[iTempFile])
					{
						isDupe = true;
						break;
					}
				}
				
				if (isDupe)
					continue; // Next!
				
				// Compile to bytecode.
				D3DXBuffer bytecode(pShaders->CompileShaderFromFile(*pPathStrings[iShader], profiles[iShader]));
				if (bytecode.m_pBuffer == NULL)
					return false;
				
				// Write bytecode to temporary file.
				if (WriteFile(tempPath, bytecode.GetPtr(), bytecode.GetSize()))
				{
					archWriter.AddFile(tempPath);
					tempFiles.push_back(tempPath); // Mark temporary file.
				}
				else
				{
					filesAdded = false;
					break; // Fail! Break out.
				}
			}
		}
	}

	// Prepare and write archive, if possible.
	const bool isPrepared = archWriter.Prepare();
	const bool isWritten = (isPrepared) ? archWriter.Write() : false;
	
	// Delete temporary files.
	for (size_t iTempFile = 0; iTempFile < tempFiles.size(); ++iTempFile)
	{
		FileWin::Delete(tempFiles[iTempFile]);
	}

	return isWritten;	
}
