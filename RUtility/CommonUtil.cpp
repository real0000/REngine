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