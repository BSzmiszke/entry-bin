#include "StdAfx.h"
#include "MappedFile.h"
#include "Debug.h"

CMappedFile::CMappedFile() :
m_hFM(NULL),
m_lpMapData(NULL),
m_dataOffset(0),
m_mapSize(0),
m_seekPosition(0),
m_pLZObj(NULL),
m_pbBufLinkData(NULL),
m_dwBufLinkSize(0),
m_pbAppendResultDataBlock(NULL),
m_dwAppendResultDataSize(0)
{
}

CMappedFile::~CMappedFile()
{
	Destroy();
}

BOOL CMappedFile::Create(const char * filename)
{
	Destroy();
	return CFileBase::Create(filename, FILEMODE_READ);
}

BOOL CMappedFile::Create(const char * filename, const void** dest, int offset, int size)
{
	if (!CMappedFile::Create(filename))
		return NULL;

	int ret = Map(dest, offset, size);
	return (ret) > 0;
}

LPCVOID CMappedFile::Get()
{
	return m_lpData;
}

void CMappedFile::Link(DWORD dwBufSize, const void* c_pvBufData)
{
	m_dwBufLinkSize=dwBufSize;
	m_pbBufLinkData=(BYTE*)c_pvBufData;
}

void CMappedFile::BindLZObject(CLZObject * pLZObj)
{
	assert(m_pLZObj == NULL);
	m_pLZObj = pLZObj;

	Link(m_pLZObj->GetSize(), m_pLZObj->GetBuffer());
}

void CMappedFile::BindLZObjectWithBufferedSize(CLZObject * pLZObj)
{
	assert(m_pLZObj == NULL);
	m_pLZObj = pLZObj;

	Link(m_pLZObj->GetBufferSize(), m_pLZObj->GetBuffer());
}

auto CMappedFile::AppendDataBlock(const void* pBlock, std::size_t blockSize) -> std::byte*
{
	static std::vector<std::byte> data;
	data.resize(m_dwBufLinkSize + blockSize);

	std::memcpy(data.data(), m_pbBufLinkData, m_dwBufLinkSize);
	std::memcpy(data.data() + m_dwBufLinkSize, pBlock, blockSize);

	Link(data.size(), data.data());

	return data.data();
}

#include <memory>

void CMappedFile::Destroy()
{
	if (m_pLZObj)
	{
		std::destroy_at(m_pLZObj);
		m_pLZObj = nullptr;
	}

	if (m_lpMapData != nullptr)
	{
		Unmap(m_lpMapData);
		m_lpMapData = nullptr;
	}

	if (m_hFM != nullptr)
	{
		CloseHandle(m_hFM);
		m_hFM = nullptr;
	}

	if (m_pbAppendResultDataBlock != nullptr)
	{
		delete[] m_pbAppendResultDataBlock;
		m_pbAppendResultDataBlock = nullptr;
	}

	m_dwAppendResultDataSize = 0;

	m_pbBufLinkData = nullptr;
	m_dwBufLinkSize = 0;

	m_seekPosition = 0;
	m_dataOffset = 0;
	m_mapSize = 0;

	CFileBase::Destroy();
}

int CMappedFile::Seek(DWORD offset, int iSeekType)
{
	switch (iSeekType)
	{
		case SEEK_TYPE_BEGIN:
			if (offset > m_dwSize)
				offset = m_dwSize;

			m_seekPosition = offset;
			break;
			
		case SEEK_TYPE_CURRENT:
			m_seekPosition = min(m_seekPosition + offset, Size());
			break;

		case SEEK_TYPE_END:
			m_seekPosition = max(0, Size() - offset);
			break;
	}

	return m_seekPosition;
}

// Przechowujemy uchwyt mapowania pliku i informacje o systemie w zmiennych statycznych, 
// aby unikn规 konieczno渃i ich tworzenia lub pobierania za ka縟ym razem, gdy wywo硑wana jest funkcja Map().

static HANDLE s_hFM = NULL;
static SYSTEM_INFO s_sysInfo;

int CMappedFile::Map(const void **dest, int offset, int size)
{
	m_dataOffset = offset;

	if (size == 0)
		m_mapSize = m_dwSize;
	else
		m_mapSize = size;

	if (m_dataOffset + m_mapSize > m_dwSize)
		return NULL;

	if (s_sysInfo.dwAllocationGranularity == 0)
		GetSystemInfo(&s_sysInfo);

	DWORD dwSysGran = s_sysInfo.dwAllocationGranularity;
	DWORD dwFileMapStart = (m_dataOffset / dwSysGran) * dwSysGran;
	DWORD dwMapViewSize = (m_dataOffset % dwSysGran) + m_mapSize;
	INT iViewDelta = m_dataOffset - dwFileMapStart;
	

	m_hFM = CreateFileMapping(m_hFile,				// handle
							  NULL,					// security
							  PAGE_READONLY,		// flProtect
							  0,					// high
							  m_dataOffset + m_mapSize,	// low
							  NULL);				// name

	if (!m_hFM)
	{
		OutputDebugString("CMappedFile::Map !m_hFM\n");
		return NULL;
	}	

	m_lpMapData = MapViewOfFile(m_hFM,
								FILE_MAP_READ,
								0,
								dwFileMapStart,
								dwMapViewSize);

	if (!m_lpMapData) // Success
	{
		TraceError("CMappedFile::Map !m_lpMapData %lu", GetLastError());
		return 0;
	}
	
	// 2004.09.16.myevan.MemoryMappedFile 98/ME 俺荐 力茄 巩力 眉农
	//g_dwCount++;
	//Tracenf("MAPFILE %d", g_dwCount);
	
	m_lpData = (char*) m_lpMapData + iViewDelta;
	*dest = (char*) m_lpData;
	m_seekPosition = 0;

	Link(m_mapSize, m_lpData);

	return (m_mapSize);
}

BYTE * CMappedFile::GetCurrentSeekPoint()
{
	return m_pbBufLinkData+m_seekPosition;
	//return m_pLZObj ? m_pLZObj->GetBuffer() + m_seekPosition : (BYTE *) m_lpData + m_seekPosition;
}


DWORD CMappedFile::Size()
{
	return m_dwBufLinkSize;
	/*
	if (m_pLZObj)
		return m_pLZObj->GetSize();

	return (m_mapSize);
	*/
}

DWORD CMappedFile::GetPosition()
{
	return m_dataOffset;
}

BOOL CMappedFile::Read(void * dest, int bytes)
{
	if (m_seekPosition + bytes > Size())
		return FALSE;

	memcpy_s(dest, bytes, GetCurrentSeekPoint(), bytes);
	m_seekPosition += bytes;
	return TRUE;
}

DWORD CMappedFile::GetSeekPosition(void)
{
	return m_seekPosition;
}

void CMappedFile::Unmap(LPCVOID data)
{
	m_lpData = nullptr;
}
