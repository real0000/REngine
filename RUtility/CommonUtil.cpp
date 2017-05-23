// CommonUtil.cpp
//
// 2014/03/12 Ian Wu/Real0000
//

#include "CommonUtil.h"

namespace R
{

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

#pragma region SearchPathSystem
//
// SearchPathSystem
//
SearchPathSystem::SearchPathSystem()
{
}

SearchPathSystem::~SearchPathSystem()
{
	m_SearchPath.clear();
	m_FileCache.clear();
}

wxString SearchPathSystem::findFile(wxString a_Filename)
{
	auto it = m_FileCache.find(a_Filename);
	if( m_FileCache.end() == it ) return it->second;

	for( unsigned int i=0 ; i<m_SearchPath.size() ; ++i )
	{
		wxString l_FilePath(m_SearchPath[i] + a_Filename);
		if( wxFileExists(l_FilePath) )
		{
			m_FileCache.insert(std::make_pair(a_Filename, l_FilePath));
			return l_FilePath;
		}
	}
	return wxT("");
}

void SearchPathSystem::addSearchPath(wxString a_Path)
{
	a_Path.Replace(wxT("\\"), wxT("/"));
	if( !a_Path.EndsWith("/") ) a_Path += wxT("/");
	assert(std::find(m_SearchPath.begin(), m_SearchPath.end(), a_Path) == m_SearchPath.end());
	m_SearchPath.push_back(a_Path);
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