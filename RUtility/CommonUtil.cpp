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


wxString getFileName(wxString a_File)
{
	std::vector<wxString> l_Tokens;
	splitString(wxT('/'), a_File, l_Tokens);
	if( l_Tokens.empty() ) return wxT("");

	return l_Tokens.back();
}

wxString getFileExt(wxString a_File)
{
	std::vector<wxString> l_Tokens;
	splitString(wxT('.'), a_File, l_Tokens);
	
	if( l_Tokens.empty() ) return wxT("");
	return l_Tokens.back();
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
		if( i+1 != l_Tokens.size() )
			l_Res += wxT("/");
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
	
	for( unsigned int i=0 ; i<l_ParentToken.size() ; i++ )
		l_Res += wxT("../");
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

void showOpenGLErrorCode(wxString a_StepInfo)
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
}

void decomposeTRS(const glm::mat4 &a_Mat, glm::vec3 &a_TransOut, glm::vec3 &a_ScaleOut, glm::quat &a_RotOut)
{
	a_TransOut.x = a_Mat[0][3];
    a_TransOut.y = a_Mat[1][3];
    a_TransOut.z = a_Mat[2][3];

    glm::vec3 l_Col1(a_Mat[0][0], a_Mat[0][1], a_Mat[0][2]);
    glm::vec3 l_Col2(a_Mat[1][0], a_Mat[1][1], a_Mat[1][2]);
    glm::vec3 l_Col3(a_Mat[2][0], a_Mat[2][1], a_Mat[2][2]);

    a_ScaleOut.x = glm::length(l_Col1);
    a_ScaleOut.y = glm::length(l_Col2);
    a_ScaleOut.z = glm::length(l_Col3);
	
    if( 0.0f != a_ScaleOut.x ) l_Col1 /= a_ScaleOut.x;
    if( 0.0f != a_ScaleOut.y ) l_Col2 /= a_ScaleOut.y;
    if( 0.0f != a_ScaleOut.z ) l_Col3 /= a_ScaleOut.z;

	a_RotOut = glm::quat_cast(glm::mat3x3(l_Col1, l_Col2, l_Col3));
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

#pragma region ThreadEventCallback
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
#pragma endregion

}