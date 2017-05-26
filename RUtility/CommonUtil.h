// CommonUtil.h
//
// 2014/03/12 Ian Wu/Real0000
//

#ifndef _COMMON_UTIL_H_
#define _COMMON_UTIL_H_

#ifdef WIN32
	#include <windows.h>
	#include <gl/glew.h>
	#include <gl/gl.h>
	#include <d3d12.h>
	#include <dxgi1_4.h>
	#define int64 int64_t
	#define uint64 uint64_t
#else
	#include <GL/glew.h>
	#include <GLUT/GLUT.h> 
	#define __int64 int64_t
#endif

#include <assert.h>
#include <vector>
#include <map>
#include <stdarg.h>
#include <algorithm>
#include <stack>
#include <set>
#include <list>
#include <stdarg.h>

#define GLM_FORCE_RADIANS
#include "vld.h"
#include "glm.hpp"
#include "gtc/constants.hpp"
#include "gtx/quaternion.hpp"
#include "gtc/matrix_transform.hpp"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

#include "wx/wx.h"

#include "MathExtend.h"

#define SAFE_DELETE(p) { if( nullptr !=  p ) delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p) { if( nullptr != p ) delete[] p; p = nullptr; }
#define SAFE_RELEASE(p) { if( nullptr != p ) p->Release(); p = nullptr; }

namespace R
{

void splitString(wxChar a_Key, wxString a_String, std::vector<wxString> &a_Output);
wxString getFileName(wxString a_File);
wxString getFileExt(wxString a_File);
wxString getFilePath(wxString a_File);
wxString getRelativePath(wxString a_ParentPath, wxString a_File);
wxString getAbsolutePath(wxString a_ParentPath, wxString a_RelativePath);
void showOpenGLErrorCode(wxString a_StepInfo);

void decomposeTRS(const glm::mat4 &a_Mat, glm::vec3 &a_TransOut, glm::vec3 &a_ScaleOut, glm::quat &a_RotOut);

template<typename T>
T* defaultNewFunc()
{
	return new T();
}

template<typename T>
void defaultDelFunc(T *a_pTarget)
{
	delete a_pTarget;
}

#define BIND_DEFAULT_ALLOCATOR(T, Manager) \
	Manager.setNewFunc(std::bind(&defaultNewFunc<T>)); \
	Manager.setDelFunc(std::bind(&defaultDelFunc<T>, std::placeholders::_1));

template<typename T>
class SerializedObjectPool
{
public:
	SerializedObjectPool(bool a_bPool = false)
		: m_NewFunc(nullptr), m_DelFunc(nullptr)
	{
		if( a_bPool )
		{
			m_ReqFunc = std::bind(&SerializedObjectPool<T>::requestItem, this);
			m_RecFunc = std::bind(&SerializedObjectPool<T>::recycleItem, this, std::placeholders::_1);
		}
		else
		{
			m_ReqFunc = std::bind(&SerializedObjectPool<T>::newItem, this);
			m_RecFunc = std::bind(&SerializedObjectPool<T>::deleteItem, this, std::placeholders::_1);
		}
	}

	virtual ~SerializedObjectPool()
	{
		clear();
	}
	
	void setNewFunc(std::function<T*()> l_NewFunc)
	{
		m_NewFunc = l_NewFunc;
	}

	void setDelFunc(std::function<void(T *)> l_DelFunc)
	{
		m_DelFunc = l_DelFunc;
	}
	
	unsigned int count()
	{
		return m_ObjectPool.size();
	}

	void clear()
	{
		for( unsigned int i=0 ; i<m_ObjectPool.size() ; ++i )
		{
			if( nullptr != m_ObjectPool[i] ) m_DelFunc(m_ObjectPool[i]);
		}
		m_ObjectPool.clear();
		m_FreeSlot.clear();
	}

	unsigned int retain(T** a_ppOutput = nullptr)
	{
		unsigned int l_Res = m_ReqFunc();
		if( nullptr != a_ppOutput ) *a_ppOutput = m_ObjectPool[l_Res];
		return l_Res;
	}

	void release(unsigned int a_ID)
	{
		m_RecFunc(a_ID);
	}

	T* getItem(unsigned int a_ID)
	{
		return m_ObjectPool[a_ID];
	}
	
	virtual T* operator[](unsigned int a_ID)
	{
		return m_ObjectPool[a_ID];
	}

private:
	// always new/delete
	unsigned int newItem()
	{
		T *l_pNewItem = m_NewFunc();
		unsigned int l_ID = m_ObjectPool.size();
		if( m_FreeSlot.empty() ) m_ObjectPool.push_back(l_pNewItem);
		else
		{
			l_ID = m_FreeSlot.front();
			m_FreeSlot.pop_front();
			m_ObjectPool[l_ID] = l_pNewItem;
		}

		return l_ID;
	}

	void deleteItem(unsigned int a_ID)
	{
		assert(nullptr != m_ObjectPool[a_ID]);
		m_DelFunc(m_ObjectPool[a_ID]);
		m_ObjectPool[a_ID] = nullptr;
		m_FreeSlot.push_back(a_ID);
	}

	// new once, use until clear
	unsigned int requestItem()
	{	
		unsigned int l_ID = 0;
		if( m_FreeSlot.empty() )
		{
			T *l_pNewItem = m_NewFunc();
			l_ID = m_ObjectPool.size();
			m_ObjectPool.push_back(l_pNewItem);
		}
		else
		{
			l_ID = m_FreeSlot.front();
			m_FreeSlot.pop_front();
		}

		return l_ID;
	}

	void recycleItem(unsigned int a_ID)
	{
		assert(std::find(m_FreeSlot.begin(), m_FreeSlot.end(), a_ID) == m_FreeSlot.end());
		m_FreeSlot.push_back(a_ID);
	}

	std::function<unsigned int()> m_ReqFunc;
	std::function<void(unsigned int)> m_RecFunc;
	std::function<T*()> m_NewFunc;
	std::function<void(T *)> m_DelFunc;

	std::vector<T *> m_ObjectPool;
	std::list<unsigned int> m_FreeSlot;
};

//define macro for api indepedence enum class
#define STRING_ENUM_CLASS(className, ...) \
class className								\
{											\
public:										\
	enum Key { __VA_ARGS__ };				\
	static Key fromString(wxString a_Name)	\
	{										\
		if( m_StringMap.empty() ) initMap();\
		assert(m_StringMap.end() != m_StringMap.find(a_Name));\
		return m_StringMap[a_Name];			\
	}										\
	static wxString toString(Key a_Key)		\
	{										\
		return m_Strings[(unsigned int)a_Key];\
	}										\
private:									\
	static void initMap()					\
	{										\
		std::vector<Key> l_KeyList{ __VA_ARGS__ };\
		wxString l_Src(#__VA_ARGS__);		\
		for( unsigned int i=0 ; i<l_Src.size() ; ++i )\
		{									\
			if( l_Src[i] == wxT(',') ) l_Src[i] = wxT(' ');\
		}									\
		std::vector<wxString> l_Items;		\
		splitString(wxT(' '), l_Src, l_Items);\
		for( unsigned int i=0 ; i<l_Items.size() ; ++i ) m_StringMap[l_Items[i]] = l_KeyList[i];\
	}										\
	static std::vector<wxString> m_Strings;\
	static std::map<wxString, Key> m_StringMap;\
};

#define STRING_ENUM_CLASS_INST(classname) \
std::vector<wxString> classname::m_Strings;\
std::map<wxString, classname::Key> classname::m_StringMap;

class SearchPathSystem
{ 
public:
	SearchPathSystem();
	virtual ~SearchPathSystem();

	wxString findFile(wxString a_Filename);
	void addSearchPath(wxString a_Path);
	void clearFileCache(){ m_FileCache.clear(); }

private:
	std::map<wxString, wxString> m_FileCache;
	std::vector<wxString> m_SearchPath;
};

class ThreadEventCallback
{
public:
	// don't use this constructor 
	ThreadEventCallback();
	virtual ~ThreadEventCallback();

	static ThreadEventCallback& getThreadLocal();
	void addEndEvent(std::function<void()> a_Func);

private:
	std::vector<std::function<void()> > m_EndEvents;
};

}

#endif