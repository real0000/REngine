// CommonUtil.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"

namespace R
{

STRING_ENUM_CLASS_INST(PixelFormat)

void splitString(wxChar a_Key, wxString a_String, std::vector<wxString> &a_Output)
{
	a_Output.clear();
	
	wxString l_Seg("");
	for( unsigned int i=0 ; i<a_String.size() ; i++ )
	{
		if( a_String[i] == a_Key )
		{
			if( !l_Seg.empty() )
			{
				a_Output.push_back(l_Seg);
				l_Seg.clear();
			}
		}
		else l_Seg += a_String[i];
	}
	
	if( !l_Seg.empty() ) a_Output.push_back(l_Seg);
}

wxString getFileName(wxString a_File, bool a_bWithExt)
{
	std::vector<wxString> l_Tokens;
	a_File.Replace("\\", "/");
	splitString(wxT('/'), a_File, l_Tokens);
	if( l_Tokens.empty() ) return wxT("");

	if( !a_bWithExt )
	{
		splitString(wxT('.'), l_Tokens.back(), l_Tokens);
		return l_Tokens.front();
	}

	return l_Tokens.back();
}

wxString getFileExt(wxString a_File)
{
	std::vector<wxString> l_Tokens;
	splitString(wxT('.'), a_File, l_Tokens);
	
	if( l_Tokens.empty() ) return wxT("");
	return l_Tokens.back();
}

wxString replaceFileExt(wxString a_File, wxString a_NewExt)
{
	std::vector<wxString> l_Tokens;
	splitString(wxT('.'), a_File, l_Tokens);
	l_Tokens.pop_back();

	wxString l_Res(wxT(""));
	for( unsigned int i=0 ; i<l_Tokens.size() ; ++i ) l_Res += l_Tokens[i];
	return l_Res + "." + a_NewExt;
}

wxString getFilePath(wxString a_File)
{
	std::vector<wxString> l_Tokens;
	splitString(wxT('/'), a_File, l_Tokens);
	if( l_Tokens.empty() ) return wxT("");

	l_Tokens.pop_back();
	
	wxString l_Res("");
	for( unsigned int i=0 ; i<l_Tokens.size() ; i++ )
	{
		l_Res += l_Tokens[i];
		if( i+1 != l_Tokens.size() ) l_Res += wxT("/");
	}

	return l_Res;
}

wxString getRelativePath(wxString a_ParentPath, wxString a_File)
{
	wxString l_Res("");
	std::vector<wxString> l_ParentToken;
	std::vector<wxString> l_FileToken;
	splitString(wxT('/'), a_File, l_FileToken);
	splitString(wxT('/'), a_ParentPath, l_ParentToken);
	unsigned long l_MinLength = l_ParentToken.size() < l_FileToken.size() ? l_ParentToken.size() : l_FileToken.size();
	for( unsigned int i=0 ; i<l_MinLength ; i++ )
	{
		if( 0 == wxStrcmp(l_ParentToken[0], l_FileToken[0]) )
		{
			l_ParentToken.erase(l_ParentToken.begin());
			l_FileToken.erase(l_FileToken.begin());
		}
		else
			break;
	}
	
	for( unsigned int i=0 ; i<l_ParentToken.size() ; i++ ) l_Res += wxT("../");
	for( unsigned int i=0 ; i<l_FileToken.size() ; i++ )
	{
		l_Res += l_FileToken[i];
		l_Res += wxT("/");
	}
	
	return l_Res;
}

wxString getAbsolutePath(wxString a_ParentPath, wxString a_RelativePath)
{
	std::vector<wxString> l_ParentToken;
	std::vector<wxString> l_FileToken;

	splitString(wxT('/'), a_ParentPath, l_ParentToken);
	splitString(wxT('/'), a_RelativePath, l_FileToken);

	for( unsigned int i=0 ; i<l_FileToken.size() ; i++ )
	{
		if( 0 == wxStrcmp(l_FileToken[0], wxT("..")) )
		{
			l_FileToken.erase(l_FileToken.begin());
			l_ParentToken.pop_back();
		}
	}

	wxString l_Res("");
	for( unsigned int i=0 ; i<l_ParentToken.size() ; i++ )
	{
		l_Res += l_ParentToken[i];
		l_Res += wxT("/");
	}

	for( unsigned int i=0 ; i<l_FileToken.size() ; i++ )
	{
		l_Res += l_FileToken[i];
		if( i+1 != l_FileToken.size() )
			l_Res += wxT("/");
	}

	return l_Res;
}

void regularFilePath(wxString &a_Path)
{
	if( a_Path.IsEmpty() ) return;
	a_Path.Replace(wxT("\\"), wxT("/"));
#ifdef WIN32
	if( wxT('/') != a_Path[0] && wxIsAbsolutePath(a_Path) ) return;
#else
	if( wxIsAbsolutePath(a_Path) ) return;
#endif
	a_Path = wxT("./") + a_Path;
	a_Path.Replace(wxT("//"), wxT("/"));
	a_Path.Replace(wxT("././"), wxT("./"));
}

wxString concatFilePath(wxString a_Left, wxString a_Right)
{
	a_Right.Replace(wxT("./"), wxT(""));
	if( !a_Left.EndsWith(wxT("/")) ) a_Left += wxT("/");
	return a_Left + a_Right;
}

void binary2Base64(void *a_pSrc, unsigned int a_Size, std::string &a_Output)
{
	std::vector<char> l_Buff;
	boost::iostreams::filtering_ostream l_Compress;
	l_Compress.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
	l_Compress.push(boost::iostreams::back_inserter(l_Buff));
	l_Compress.write(reinterpret_cast<const char *>(a_pSrc), a_Size);
	boost::iostreams::close(l_Compress);

	a_Output.resize(boost::beast::detail::base64::encoded_size(a_Size));
    a_Output.resize(boost::beast::detail::base64::encode((void *)(a_Output.data()), l_Buff.data(), l_Buff.size()));
}

void base642Binary(std::string &a_Src, std::vector<char> &a_Output)
{
	a_Output.clear();
	std::vector<char> l_Buff;
	l_Buff.resize(boost::beast::detail::base64::decoded_size(a_Src.size()));
	auto const l_Base64Size = boost::beast::detail::base64::decode(l_Buff.data(), a_Src.data(), a_Src.size());
	l_Buff.resize(l_Base64Size.first);

	boost::iostreams::filtering_ostream l_Decompress;
	l_Decompress.push(boost::iostreams::gzip_decompressor());
	l_Decompress.push(boost::iostreams::back_inserter(a_Output));
	l_Decompress.write(l_Buff.data(), l_Buff.size());
	boost::iostreams::close(l_Decompress);
}

std::pair<unsigned int, unsigned int> calculateSegment(unsigned int a_Count, unsigned int a_NumDevide, unsigned int a_SegmentIdx)
{
	std::pair<unsigned int, unsigned int> l_Res;
	unsigned int l_Unit = std::max<unsigned int>(a_Count / a_NumDevide + (0 == (a_Count % a_NumDevide) ? 0 : 1), 1);
	l_Res.first = a_SegmentIdx*l_Unit;
	l_Res.second = std::min<unsigned int>(l_Res.first + l_Unit, a_Count);
	return l_Res;
}
/*void showOpenGLErrorCode(wxString a_StepInfo)
{
#ifdef _DEBUG
	GLenum l_ErrorCode = glGetError();
	if( GL_NO_ERROR != l_ErrorCode )
	{
		char l_Buff[32];
		sprintf(l_Buff, ": %x", l_ErrorCode);
		wxMessageBox(a_StepInfo + l_Buff);
	}
#endif
}*/

void decomposeTRS(const glm::mat4 &a_Mat, glm::vec3 &a_TransOut, glm::vec3 &a_ScaleOut, glm::quat &a_RotOut)
{
	a_TransOut.x = a_Mat[3][0];
    a_TransOut.y = a_Mat[3][1];
    a_TransOut.z = a_Mat[3][2];

    glm::vec3 l_Col1(a_Mat[0][0], a_Mat[1][0], a_Mat[2][0]);
    glm::vec3 l_Col2(a_Mat[0][1], a_Mat[1][1], a_Mat[2][1]);
    glm::vec3 l_Col3(a_Mat[0][2], a_Mat[1][2], a_Mat[2][2]);

    a_ScaleOut.x = glm::length(l_Col1);
    a_ScaleOut.y = glm::length(l_Col2);
    a_ScaleOut.z = glm::length(l_Col3);
	
    if( 0.0f != a_ScaleOut.x ) l_Col1 /= a_ScaleOut.x;
    if( 0.0f != a_ScaleOut.y ) l_Col2 /= a_ScaleOut.y;
    if( 0.0f != a_ScaleOut.z ) l_Col3 /= a_ScaleOut.z;

	a_RotOut = glm::quat_cast(glm::mat3x3(l_Col1, l_Col2, l_Col3));
}

void composeTRS(const glm::vec3 &a_Trans, const glm::vec3 &a_Scale, const glm::quat &a_Rot, glm::mat4 &a_MatOut)
{
	a_MatOut = glm::scale(glm::identity<glm::mat4x4>(), a_Scale);
	a_MatOut = glm::mat4_cast(a_Rot) * a_MatOut;
	a_MatOut[3][0] = a_Trans.x;
	a_MatOut[3][1] = a_Trans.y;
	a_MatOut[3][2] = a_Trans.z;
}

unsigned int getPixelSize(PixelFormat::Key a_Key)
{
	assert(PixelFormat::unknown != a_Key);
	switch( a_Key )
	{
		case PixelFormat::rgba32_typeless:
		case PixelFormat::rgba32_float:
		case PixelFormat::rgba32_uint:
		case PixelFormat::rgba32_sint:				return 128;
		case PixelFormat::rgb32_typeless:
		case PixelFormat::rgb32_float:
		case PixelFormat::rgb32_uint:
		case PixelFormat::rgb32_sint:				return 96;
		case PixelFormat::rgba16_typeless:
		case PixelFormat::rgba16_float:
		case PixelFormat::rgba16_unorm:
		case PixelFormat::rgba16_uint:
		case PixelFormat::rgba16_snorm:
		case PixelFormat::rgba16_sint:
		case PixelFormat::rg32_typeless:
		case PixelFormat::rg32_float:
		case PixelFormat::rg32_uint:
		case PixelFormat::rg32_sint:
		case PixelFormat::r32g8x24_typeless: 
		case PixelFormat::d32_float_s8x24_uint:
		case PixelFormat::r32_float_x8x24_typeless:
		case PixelFormat::x32_typeless_g8x24_uint:
		case PixelFormat::y416:
		case PixelFormat::y210:
		case PixelFormat::y216:						return 64;
		case PixelFormat::rgb10a2_typeless:
		case PixelFormat::rgb10a2_unorm:
		case PixelFormat::rgb10a2_uint:
		case PixelFormat::r11g11b10_float:
		case PixelFormat::rgba8_typeless:
		case PixelFormat::rgba8_unorm:
		case PixelFormat::rgba8_unorm_srgb:
		case PixelFormat::rgba8_uint:
		case PixelFormat::rgba8_snorm:
		case PixelFormat::rgba8_sint:
		case PixelFormat::rg16_typeless:
		case PixelFormat::rg16_float:
		case PixelFormat::rg16_unorm:
		case PixelFormat::rg16_uint:
		case PixelFormat::rg16_snorm:
		case PixelFormat::rg16_sint:
		case PixelFormat::r32_typeless:
		case PixelFormat::d32_float:
		case PixelFormat::r32_float:
		case PixelFormat::r32_uint:
		case PixelFormat::r32_sint:
		case PixelFormat::r24g8_typeless:
		case PixelFormat::d24_unorm_s8_uint:
		case PixelFormat::r24_unorm_x8_typeless:
		case PixelFormat::x24_typeless_g8_uint:
		case PixelFormat::rgb9e5:
		case PixelFormat::rgbg8_unorm:
		case PixelFormat::grgb8_unorm:
		case PixelFormat::bgra8_unorm:
		case PixelFormat::bgrx8_unorm:
		case PixelFormat::rgb10_xr_bias_a2_unorm:
		case PixelFormat::bgra8_typeless:
		case PixelFormat::bgra8_unorm_srgb:
		case PixelFormat::bgrx8_typeless:
		case PixelFormat::bgrx8_unorm_srgb:
		case PixelFormat::ayuv:
		case PixelFormat::y410:
		case PixelFormat::yuy2:						return 32;
		case PixelFormat::p010:
		case PixelFormat::p016:						return 24;
		case PixelFormat::rg8_typeless:
		case PixelFormat::rg8_unorm:
		case PixelFormat::rg8_uint:
		case PixelFormat::rg8_snorm:
		case PixelFormat::rg8_sint:
		case PixelFormat::r16_typeless:
		case PixelFormat::r16_float:
		case PixelFormat::d16_unorm:
		case PixelFormat::r16_unorm:
		case PixelFormat::r16_uint:
		case PixelFormat::r16_snorm:
		case PixelFormat::r16_sint:
		case PixelFormat::b5g6r5_unorm:
		case PixelFormat::bgr5a1_unorm:
		case PixelFormat::ap8:
		case PixelFormat::bgra4_unorm:				return 16;
		case PixelFormat::nv12:
		case PixelFormat::opaque420:
		case PixelFormat::nv11:						return 12;
		case PixelFormat::r8_typeless:
		case PixelFormat::r8_unorm:
		case PixelFormat::r8_uint:
		case PixelFormat::r8_snorm:
		case PixelFormat::r8_sint:
		case PixelFormat::a8_unorm:
		case PixelFormat::ai44:
		case PixelFormat::ia44:
		case PixelFormat::p8:						return 8;
		case PixelFormat::r1_unorm:					return 1;
		case PixelFormat::bc1_typeless:
		case PixelFormat::bc1_unorm:
		case PixelFormat::bc1_unorm_srgb:
		case PixelFormat::bc4_typeless:
		case PixelFormat::bc4_unorm:
		case PixelFormat::bc4_snorm:				return 4;
		case PixelFormat::bc2_typeless:
		case PixelFormat::bc2_unorm:
		case PixelFormat::bc2_unorm_srgb:
		case PixelFormat::bc3_typeless:
		case PixelFormat::bc3_unorm:
		case PixelFormat::bc3_unorm_srgb:
		case PixelFormat::bc5_typeless:
		case PixelFormat::bc5_unorm:
		case PixelFormat::bc5_snorm:
		case PixelFormat::bc6h_typeless:
		case PixelFormat::bc6h_uf16:
		case PixelFormat::bc6h_sf16:
		case PixelFormat::bc7_typeless:
		case PixelFormat::bc7_unorm:
		case PixelFormat::bc7_unorm_srgb:			return 8;
		case PixelFormat::p208:
		case PixelFormat::v208:
		case PixelFormat::v408:						return 8;
		case PixelFormat::uint:						return 32;
		default:break;
	}

	assert(false && "invalid key");
	return 0;
}

#pragma region VirtualMemoryPool
//
// VirtualMemoryPool
//
VirtualMemoryPool::VirtualMemoryPool()
	: m_CurrSize(0)
{
}

VirtualMemoryPool::~VirtualMemoryPool()
{
	m_FreeSpaceList.clear();
	m_AllocateMap.clear();
}
	
int VirtualMemoryPool::malloc(int a_Size)
{
	int l_Res = -1;
	for( auto it = m_FreeSpaceList.begin() ; m_FreeSpaceList.end() != it ; ++it )
	{
		if( it->second >= a_Size )
		{
			l_Res = it->first;
			int l_NewOffset = it->first + a_Size;
			int l_NewSize = it->second - a_Size;
			m_FreeSpaceList.erase(it);
			if( 0 != l_NewSize ) m_FreeSpaceList[l_NewOffset] = l_NewSize;
			break;
		}
	}
	if( -1 != l_Res ) m_AllocateMap[l_Res] = a_Size;
	return l_Res;
}

void VirtualMemoryPool::free(int a_Offset)
{
	int l_Size = 0;
	{
		auto it = m_AllocateMap.find(a_Offset);
		assert(m_AllocateMap.end() != it);
		l_Size = it->second;
		m_AllocateMap.erase(it);
	}

	int l_PrevOffset = a_Offset;
	int l_NextOffset = a_Offset + l_Size;

	auto l_PrevIt = m_FreeSpaceList.end();
	for( auto it = m_FreeSpaceList.begin() ; m_FreeSpaceList.end() != it ; ++it )
	{
		if( it->first + it->second == l_PrevOffset )
		{
			it->second += l_Size;
			l_PrevIt = it;
		}
		else if( it->first == l_NextOffset )
		{
			if( m_FreeSpaceList.end() != l_PrevIt )
			{
				l_PrevIt->second += it->second;
				m_FreeSpaceList.erase(it);
			}
			else
			{
				int l_NewSize = it->second + l_Size;
				m_FreeSpaceList.erase(it);
				m_FreeSpaceList[a_Offset] = l_NewSize;
			}
			break;
		}
		else if( it->first > l_NextOffset )
		{
			m_FreeSpaceList[a_Offset] = l_Size;
			break;
		}
	}
}

void VirtualMemoryPool::purge()
{
	m_FreeSpaceList.clear();
	m_AllocateMap.clear();
	m_FreeSpaceList.insert(std::make_pair(0, m_CurrSize));
}

int VirtualMemoryPool::getAllocateSize(int a_Offset)
{
	auto it = m_AllocateMap.find(a_Offset);
	assert(m_AllocateMap.end() != it);
	return it->second;
}

void VirtualMemoryPool::init(int a_Size)
{
	assert(0 == m_CurrSize);
	m_CurrSize = a_Size;
	m_FreeSpaceList.insert(std::make_pair(0, a_Size));
}

void VirtualMemoryPool::extend(int a_Size)
{
	assert(0 != m_CurrSize);
	if( m_FreeSpaceList.empty() ) m_FreeSpaceList.insert(std::make_pair(m_CurrSize, a_Size));
	else
	{
		auto it = m_FreeSpaceList.rbegin();
		if( it->first + it->second == m_CurrSize ) it->second += a_Size;
		else m_FreeSpaceList.insert(std::make_pair(m_CurrSize, a_Size));
	}
	m_CurrSize += a_Size;
}
#pragma endregion

#pragma region ImageAtlas
#pragma region ImageAtlas::SplitNode
//
// ImageAtlas::SplitNode
//
ImageAtlas::SplitNode::SplitNode()
	: m_Parent(-1), m_Left(-1), m_Right(-1)
	, m_bUsed(false)
	, m_pRefPool(nullptr)
{
	memset(&m_NodeInfo, 0, sizeof(NodeInfo));
}

ImageAtlas::SplitNode::~SplitNode()
{
}

void ImageAtlas::SplitNode::release()
{
	if( -1 != m_Left ) (*m_pRefPool)[m_Left]->release();
	if( -1 != m_Right ) (*m_pRefPool)[m_Right]->release();
	m_pRefPool->release(m_NodeInfo.m_ID);
}
#pragma endregion

//
// ImageAtlas
//
ImageAtlas::ImageAtlas(glm::ivec2 a_Size, unsigned int a_InitArraySize)
	: m_NodePool(true)
	, m_MaxSize(a_Size)
	, m_CurrArraySize(a_InitArraySize)
	, m_ExtendSize(1)

{
	BIND_DEFAULT_ALLOCATOR(SplitNode, m_NodePool);

	for( unsigned int i=0 ; i<m_CurrArraySize ; ++i )
	{
		std::shared_ptr<SplitNode> l_pNode = nullptr;
		unsigned int l_ID = m_NodePool.retain(&l_pNode);
		m_Roots.push_back(l_ID);
		l_pNode->m_NodeInfo.m_Size = a_Size;
		l_pNode->m_NodeInfo.m_Index = i;
		l_pNode->m_NodeInfo.m_ID = 0;
		l_pNode->m_NodeInfo.m_Offset = glm::zero<glm::ivec2>();
	}
}

ImageAtlas::~ImageAtlas()
{
	m_NodePool.clear();
	m_Roots.clear();
}

unsigned int ImageAtlas::allocate(glm::ivec2 a_Size)
{
	assert(a_Size.x <= m_MaxSize.x && a_Size.y <= m_MaxSize.y);

	bool l_bFound = false;
	unsigned int l_Res = 0;
	for( unsigned int i=0 ; i<m_Roots.size() ; ++i )
	{
		if( insertNode(m_Roots[i], a_Size, l_Res) )
		{
			l_bFound = true;
			break;
		}
	}

	if( l_bFound ) return l_Res;

	m_CurrArraySize += m_ExtendSize;
	for( unsigned int i=0 ; i<m_ExtendSize ; ++i )
	{
		std::shared_ptr<SplitNode> l_pNode = nullptr;
		unsigned int l_ID = m_NodePool.retain(&l_pNode);
		m_Roots.push_back(l_ID);

		l_pNode->m_bUsed = false;
		l_pNode->m_Left = -1;
		l_pNode->m_Right = -1;
		l_pNode->m_Parent = -1;
		l_pNode->m_pRefPool = &m_NodePool;

		l_pNode->m_NodeInfo.m_Size = m_MaxSize;
		l_pNode->m_NodeInfo.m_Index = m_Roots.size() - 1;
		l_pNode->m_NodeInfo.m_ID = l_ID;
		l_pNode->m_NodeInfo.m_Offset = glm::zero<glm::ivec2>();
	}
	insertNode(m_Roots[m_Roots.size() - m_ExtendSize], a_Size, l_Res);
	return l_Res;
}

void ImageAtlas::release(unsigned int a_ID)
{
	std::shared_ptr<SplitNode> l_pNode = m_NodePool[a_ID];
	if( -1 == l_pNode->m_Parent )
	{
		l_pNode->m_bUsed = false;
		if( -1 != l_pNode->m_Left ) m_NodePool.release(l_pNode->m_Left);
		if( -1 != l_pNode->m_Right ) m_NodePool.release(l_pNode->m_Right);
		l_pNode->m_Left = l_pNode->m_Right = -1;
		return;
	}

	std::shared_ptr<SplitNode> l_pParent = m_NodePool[l_pNode->m_Parent];

	if( l_pNode->m_NodeInfo.m_ID == l_pParent->m_Left  ) l_pParent->m_Left = -1;
	else l_pParent->m_Right = -1;
 
	l_pNode->release();

	if( -1 == l_pParent->m_Left && -1 == l_pParent->m_Right ) release(l_pParent->m_NodeInfo.m_ID);
}

void ImageAtlas::releaseAll()
{
	for( unsigned int i=0 ; i<m_Roots.size() ; ++i ) release(m_Roots[i]);
}

ImageAtlas::NodeInfo& ImageAtlas::getInfo(int a_ID)
{
	return m_NodePool[a_ID]->m_NodeInfo;
}

void ImageAtlas::setExtendSize(unsigned int a_Extend)
{
	assert(a_Extend >= 1);
	m_ExtendSize = a_Extend;
}

bool ImageAtlas::insertNode(unsigned int a_ID, glm::ivec2 a_Size, unsigned int &a_Output)
{
	std::shared_ptr<SplitNode> l_pNode = m_NodePool[a_ID];
	if( -1 != l_pNode->m_Left || -1 != l_pNode->m_Right )
    {
		if( insertNode(l_pNode->m_Left, a_Size, a_Output) ) return true;
		return insertNode(l_pNode->m_Right, a_Size, a_Output);
    }
    else
    {
		if( l_pNode->m_bUsed ) return false;
        
        if( l_pNode->m_NodeInfo.m_Size.x < a_Size.x || l_pNode->m_NodeInfo.m_Size.y < a_Size.y ) return false;
        if( l_pNode->m_NodeInfo.m_Size.x == a_Size.x && l_pNode->m_NodeInfo.m_Size.y == a_Size.y )
        {
			l_pNode->m_bUsed = true;
			a_Output = l_pNode->m_NodeInfo.m_ID;

            return true;
        }
        
        std::shared_ptr<SplitNode> l_pLeft = nullptr;
		unsigned int l_TempID = m_NodePool.retain(&l_pLeft);
		l_pNode->m_Left = l_TempID;
		l_pLeft->m_bUsed = false;
		l_pLeft->m_Parent = l_pNode->m_NodeInfo.m_ID;
		l_pLeft->m_Left = l_pLeft->m_Right = -1;
		l_pLeft->m_pRefPool = &m_NodePool;
		l_pLeft->m_NodeInfo.m_ID = l_TempID;
		l_pLeft->m_NodeInfo.m_Index = l_pNode->m_NodeInfo.m_Index;

		std::shared_ptr<SplitNode> l_pRight = nullptr;
		l_TempID = m_NodePool.retain(&l_pRight);
		l_pNode->m_Right = l_TempID;
		l_pLeft->m_bUsed = false;
		l_pLeft->m_Parent = l_pNode->m_NodeInfo.m_ID;
		l_pLeft->m_Left = l_pLeft->m_Right = -1;
		l_pLeft->m_pRefPool = &m_NodePool;
		l_pLeft->m_NodeInfo.m_ID = l_TempID;
		l_pLeft->m_NodeInfo.m_Index = l_pNode->m_NodeInfo.m_Index;

		unsigned int l_DeltaWidth = l_pNode->m_NodeInfo.m_Size.x - a_Size.x;
        unsigned int l_DeltaHeight = l_pNode->m_NodeInfo.m_Size.y - a_Size.y;
        if( l_DeltaWidth < l_DeltaHeight )
        {
            l_pLeft->m_NodeInfo.m_Offset = l_pNode->m_NodeInfo.m_Offset;
            l_pLeft->m_NodeInfo.m_Size.x = l_pNode->m_NodeInfo.m_Size.x;
            l_pLeft->m_NodeInfo.m_Size.y = a_Size.y;
            
            l_pRight->m_NodeInfo.m_Offset.x = l_pNode->m_NodeInfo.m_Offset.x;
            l_pRight->m_NodeInfo.m_Offset.y = l_pNode->m_NodeInfo.m_Offset.y + a_Size.y;
            l_pRight->m_NodeInfo.m_Size.x = l_pNode->m_NodeInfo.m_Size.x;
            l_pRight->m_NodeInfo.m_Size.y = l_pNode->m_NodeInfo.m_Size.y - a_Size.y;
        }
        else
        {
            l_pLeft->m_NodeInfo.m_Offset = l_pNode->m_NodeInfo.m_Offset;
            l_pLeft->m_NodeInfo.m_Size.x = a_Size.x;
            l_pLeft->m_NodeInfo.m_Size.y = l_pNode->m_NodeInfo.m_Size.y;
            
            l_pRight->m_NodeInfo.m_Offset.x = l_pNode->m_NodeInfo.m_Offset.x + a_Size.x;
            l_pRight->m_NodeInfo.m_Offset.y = l_pNode->m_NodeInfo.m_Offset.y;
            l_pRight->m_NodeInfo.m_Size.x = l_pNode->m_NodeInfo.m_Size.x - a_Size.x;
            l_pRight->m_NodeInfo.m_Size.y = l_pNode->m_NodeInfo.m_Size.y;
        }
        
        return insertNode(l_pNode->m_Left, a_Size, a_Output);
    }

    return false;
}
#pragma endregion

#pragma region ThreadPool
//
// ThreadPool
//
ThreadPool::ThreadPool(unsigned int a_NumThread)
	: m_bLooping(true)
	, m_WorkingCount(0)
{
	for( unsigned int i=0 ; i<a_NumThread ; ++i )
	{
		m_Threads.emplace_back(std::thread(&ThreadPool::loop, this));
	}
}

ThreadPool::~ThreadPool()
{
	m_bLooping = false;
	m_Signal.notify_all();
	for( unsigned int i=0 ; i<m_Threads.size() ; ++i ) m_Threads[i].join();
	m_Threads.clear();
}

void ThreadPool::addJob(std::function<void()> a_Job)
{
	std::lock_guard<std::mutex> l_QueueLock(m_QueueLock);
	++m_WorkingCount;
	m_JobQueue.push_back(a_Job);
	m_Signal.notify_all();
}

void ThreadPool::join()
{
	while( 0 != m_WorkingCount ) std::this_thread::yield();
}

void ThreadPool::loop()
{
	while( m_bLooping )
	{
        std::unique_lock<std::mutex> l_Lock(m_SignalLock);
		if( 0 == m_WorkingCount ) m_Signal.wait(l_Lock);
		
		std::function<void()> l_Job = nullptr;
		{
			std::lock_guard<std::mutex> l_QueueLock(m_QueueLock);
			if( m_JobQueue.empty() ) continue;
			else
			{
				l_Job = m_JobQueue.front();
				m_JobQueue.pop_front();
			}
		}
		l_Job();
		--m_WorkingCount;
	}
}
#pragma endregion

#pragma region ThreadFence
//
// ThreadFence
//
ThreadFence::ThreadFence(unsigned int a_NumWorker)
	: m_Serial(1)
	, m_Complete(0)
{
	while( m_Complete.size() < a_NumWorker ) m_Complete.push_back(std::make_pair(0, 0));
}

ThreadFence::~ThreadFence()
{
}

uint64 ThreadFence::signal(unsigned int a_WorkerID)
{
	assert(a_WorkerID < m_Complete.size());
	m_Complete[a_WorkerID].second = m_Serial++;
	return m_Complete[a_WorkerID].second;
}

void ThreadFence::wait(unsigned int a_WorkerID, uint64 a_SignalVal)
{
	while( m_Complete[a_WorkerID].first < a_SignalVal ) std::this_thread::yield();
}

void ThreadFence::complete(unsigned int a_WorkerID)
{
	m_Complete[a_WorkerID].first = m_Complete[a_WorkerID].second;
}
#pragma endregion
/*#pragma region ThreadEventCallback
//
// ThreadEventCallback
//
thread_local ThreadEventCallback g_ThreadCallback;
ThreadEventCallback::ThreadEventCallback()
{
}

ThreadEventCallback::~ThreadEventCallback()
{
	for( unsigned int i=0 ; i<m_EndEvents.size() ; ++i ) m_EndEvents[i]();
}

ThreadEventCallback& ThreadEventCallback::getThreadLocal()
{
	return g_ThreadCallback;
}

void ThreadEventCallback::addEndEvent(std::function<void()> a_Func)
{
	m_EndEvents.push_back(a_Func);
}
#pragma endregion*/

}