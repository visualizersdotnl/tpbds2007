
// tpbds -- Demo resource hub.

// ResourceHub is a system that provides controlled and centralized loading of resources, 
// including the option to build and load an archive instead of separate files.

// Tip: if you run into trouble with resources, run in debug mode.
// Assertions detect all kinds of programmer error and inconsistencies.

// Notes:
// - Archive system has a 32-bit limit and does not support (very) large files.
// - Shader model 3.0.
// - Shader entry point name is main().

#ifndef _RESOURCES_H_
#define _RESOURCES_H_

class ResourceHub : public NoCopy
{
public:
	// Declared below, but required by ArchiveLoader.
	class Resource;

	// -- Archive system. --

	class ArchiveWriter : public NoCopy
	{
	public:
		ArchiveWriter(const std::string &archivePath) :
			m_archivePath(archivePath),
			m_isPrepared(false) {}
			
		~ArchiveWriter();
		
		void AddFile(const std::string &path);
		bool Prepare();
		bool Write();
		
	private:
		const std::string m_archivePath;
		bool m_isPrepared;

		// Physical file container.
		class File 
		{
		public:
			File(const std::string &path) :
				m_path(path) {}
			
			bool Load();
				
			std::string m_path;
			
			// Set by Load().
			OpaqueData m_data;
			size_t m_size;
			std::string m_ID;
			uint32_t m_hash;
		};
		
		std::vector<File *> m_files;
	};

	class ArchiveLoader : public NoCopy
	{
	public:
		ArchiveLoader(const std::string &archivePath) :
			m_archivePath(archivePath),
			m_isLoaded(false),
			m_pDataChunk(NULL) {}
			
		~ArchiveLoader() 
		{ 
			delete[] m_pDataChunk; 
		}
				
		bool Load();
		bool IsLoaded() const { return m_isLoaded; }

		OpaqueData GetFileData(const std::string &path) const;

	private:
		const std::string m_archivePath;

		bool m_isLoaded;
		const Byte *m_pDataChunk;

		// Archive file descriptor.
		struct File
		{
			uint32_t hash;
			uint32_t size;
			std::string ID;
			size_t dataOffset;
		};

		std::vector<const File> m_files;
	};

	// -- Resource types. --

	// All types must inherit from Resource.
	class Resource : public NoCopy
	{
		friend class ResourceHub;

	public:
		virtual unsigned int GetNumPaths() const = 0; // A resource can consist of multiple physical files.
		virtual const std::string GetPath(unsigned int iPath) const = 0;
		virtual bool IsCreated() const = 0;
	
	protected:
		enum Type
		{
			// All types are implemented below this class, in sequential order.
			TEXTURE, // class Texture + RequestTexture()
			SHADERS, // class Shaders + RequestShaders()
			MESH3DS, // class Mesh3DS + RequestMesh3DS()
			MESHLWO, // class MeshLWO + RequestMeshLWO()
			RAWDATA  // class RawData + RequestRawData()
		};

		static OpaqueData GetFileData(const std::string &path, const ArchiveLoader *pArchLoader)
		{
			if (pArchLoader != NULL)
			{
				return pArchLoader->GetFileData(path);
			}
			else
			{
				return LoadFile(path);
			}
		}

		Resource(Type type) :
			m_type(type) {}

		virtual ~Resource() {}

		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader) = 0;
		virtual void Destroy(Renderer &renderer) = 0;

	private:
		Type GetType() const 
		{ 
			return m_type; 
		}

		const Type m_type;		
	};

	class Texture : public Resource
	{
		friend class ResourceHub;

	public:
		const Renderer::Texture *Get() const
		{
			TPB_ASSERT(IsCreated());
			return m_pTexture;
		}

		virtual unsigned int GetNumPaths() const
		{ 
			return 1; 
		}
		
		virtual const std::string GetPath(unsigned int iPath) const 
		{ 
			TPB_ASSERT(!iPath); return m_path; 
		}

		virtual bool IsCreated() const 
		{ 
			return m_pTexture != NULL; 
		};

	private:
		Texture(const std::string &path, bool generateMipLevels, bool noRoundingToPow2) :
			Resource(TEXTURE),
			m_path(path),
			m_generateMipLevels(generateMipLevels),
			m_noRoundingToPow2(noRoundingToPow2),
			m_pTexture(NULL) {}

		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader);
		virtual void Destroy(Renderer &renderer);

		const std::string m_path;
		const bool m_generateMipLevels;
		const bool m_noRoundingToPow2;
		
		Renderer::Texture *m_pTexture;
	};

	class Shaders : public Resource
	{
		friend class ResourceHub;
		
	public:
		const Renderer::ShaderPair *Get() const
		{ 
			TPB_ASSERT(IsCreated());
			return m_pShaderPair;
		}

		virtual unsigned int GetNumPaths() const { return 2; }
		
		virtual const std::string GetPath(unsigned int iPath) const
		{
			switch (iPath)
			{
			case 0:
				return m_vsPath;
			
			case 1:
				return m_psPath;
			
			default:
				TPB_ASSERT(0);
				return "";
			}
		}

		virtual bool IsCreated() const
		{ 
			return m_pShaderPair != NULL;
		}
		
	private:
		Shaders(const std::string &vsPath, const std::string &psPath, uint32_t reqFlexVtxBits) :
			Resource(SHADERS),
			m_vsPath(vsPath),
			m_psPath(psPath),
			m_reqFlexVtxBits(reqFlexVtxBits),
			m_pShaderPair(NULL) {}

		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader);
		virtual void Destroy(Renderer &renderer);

		// profile - FXC-style target profile (e.g. "ps_3_0").
		ID3DXBuffer *CompileShaderFromFile(const std::string &path, const std::string &profile);

		const std::string m_vsPath;
		const std::string m_psPath;
		const uint32_t m_reqFlexVtxBits;
		
		Renderer::ShaderPair *m_pShaderPair;
	};
	
	class Mesh3DS : public Resource
	{
		friend class ResourceHub;

	public:
		const Mesh *Get() const 
		{ 
			TPB_ASSERT(IsCreated());
			return m_pMesh; 
		}

		virtual unsigned int GetNumPaths() const
		{
			return 1;
		}
		
		virtual const std::string GetPath(unsigned int iPath) const
		{ 
			TPB_ASSERT(!iPath); return m_path;
		}
		
		virtual bool IsCreated() const 
		{ 
			return m_pMesh != NULL; 
		}

		static const uint32_t kFlexVertexFlags;

	private:
		Mesh3DS(const std::string &path, float scaleTo, unsigned objIndex = 0) :
			Resource(MESH3DS),
			m_path(path),
			m_scaleTo(scaleTo),
			m_objIndex(objIndex),
			m_pMesh(NULL) {}
		
		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader);
		virtual void Destroy(Renderer &renderer);

		const std::string m_path;
		const float m_scaleTo;
		const unsigned int m_objIndex;
		
		Mesh *m_pMesh;
	};

	class MeshLWO : public Resource
	{
		friend class ResourceHub;

	public:
		const Mesh *Get() const 
		{ 
			TPB_ASSERT(IsCreated());
			return m_pMesh; 
		}

		virtual unsigned int GetNumPaths() const
		{ 
			return 1;
		}
		
		virtual const std::string GetPath(unsigned int iPath) const
		{
			TPB_ASSERT(!iPath); return m_path;
		}
		
		virtual bool IsCreated() const
		{
			return m_pMesh != NULL;
		}

		static const uint32_t kFlexVertexFlags;

	private:
		MeshLWO(const std::string &path) :
			Resource(MESHLWO),
			m_path(path),
			m_pMesh(NULL) {}
		
		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader);
		virtual void Destroy(Renderer &renderer);

		const std::string m_path;
		
		Mesh *m_pMesh;
	};

	class RawData : public Resource
	{
		friend class ResourceHub;
	
	public:
		const OpaqueData *Get() const 
		{
			TPB_ASSERT(IsCreated());
			return &m_opaqueData;
		}

		virtual unsigned int GetNumPaths() const
		{
			return 1;
		}

		virtual const std::string GetPath(unsigned int iPath) const
		{
			TPB_ASSERT(!iPath);
			return m_path;
		}
		
		virtual bool IsCreated() const
		{
			return m_opaqueData.IsValid();
		}
	
	private:
		RawData(const std::string &path) :
			Resource(RAWDATA),
			m_path(path) {}

		virtual bool Create(Renderer &renderer, const ArchiveLoader *pArchLoader);
		virtual void Destroy(Renderer &renderer);

		const std::string m_path;
		
		OpaqueData m_opaqueData;
	};

	// -- Resource hub. --

	ResourceHub(Renderer &renderer);
	~ResourceHub();

private:
	// Abbrev.
	typedef std::map<const std::string, Resource *> ResourceMap;
	typedef std::pair<const std::string, Resource *> MapPair;

	// Return value indicates if loading is necessary or not.
	// Please take any assertion seriously as it indicates misuse of ResourceHub.
	ResourceMap::const_iterator RequestResource(const std::string &path, Resource::Type type);

public:
	// Not returning a constant pointer (this also affects RequestResource()'s return type).
	// The classes are sealed off anyway and it saves typing.
	Texture *RequestTexture(const std::string &path, bool generateMipLevels, bool noRoundingToPow2);
	Shaders *RequestShaders(const std::string &vsPath, const std::string &psPath, uint32_t reqFlexVtxBits);
	Mesh3DS *RequestMesh3DS(const std::string &path, float scaleTo, unsigned int objIndex = 0);
	MeshLWO *RequestMeshLWO(const std::string &path);
	RawData *RequestRawData(const std::string &path);

	// After IsLoaded() it is no longer possible to request resources.
	// This will result in an assertion and NULL.
	bool Load(bool loadFromArchive, const std::string &archivePath);
	bool IsLoaded() const { return m_loadState == LOADED; }

	// Writes all requested resources to a single archive.
	// Does not until IsLoaded()!
	bool WriteArchive(const std::string &path);

private:
	Renderer &m_renderer;

	enum LoadState
	{
		NOT_LOADED,
		LOADING,
		LOADED
	} m_loadState;

	ResourceMap m_resources;
};

#endif // _RESOURCES_H_
