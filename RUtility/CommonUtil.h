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
#include <thread>
#include <mutex>
#include <memory>
#include <stdarg.h>

#define GLM_FORCE_RADIANS
#include "vld.h"
#include "glm.hpp"
#include "gtc/constants.hpp"
#include "gtx/quaternion.hpp"
#include "gtc/matrix_transform.hpp"

#include "boost/beast/core/detail/base64.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/iostreams/filter/gzip.hpp"
#include "boost/iostreams/filtering_stream.hpp"

#include "wx/wx.h"

#include "MathExtend.h"

#define SAFE_DELETE(p) { if( nullptr !=  p ) delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p) { if( nullptr != p ) delete[] p; p = nullptr; }
#define SAFE_RELEASE(p) { if( nullptr != p ) p->Release(); p = nullptr; }

namespace R
{

void splitString(wxChar a_Key, wxString a_String, std::vector<wxString> &a_Output);
wxString getFileName(wxString a_File, bool a_bWithExt = true);
wxString getFileExt(wxString a_File);
wxString replaceFileExt(wxString a_File, wxString a_NewExt);
wxString getFilePath(wxString a_File);
wxString getRelativePath(wxString a_ParentPath, wxString a_File);
wxString getAbsolutePath(wxString a_ParentPath, wxString a_RelativePath);
bool isAbsolutePath(wxString a_Path);
void binary2Base64(void *a_pSrc, unsigned int a_Size, std::string &a_Output);
void base642Binary(std::string &a_Src, std::vector<char> &a_Output);
//void showOpenGLErrorCode(wxString a_StepInfo);

void decomposeTRS(const glm::mat4 &a_Mat, glm::vec3 &a_TransOut, glm::vec3 &a_ScaleOut, glm::quat &a_RotOut);
void composeTRS(const glm::vec3 &a_Trans, const glm::vec3 &a_Scale, const glm::quat &a_Rot, glm::mat4 &a_MatOut);

template<typename T>
std::shared_ptr<T> defaultNewFunc()
{
	return std::shared_ptr<T>(new T());
}

#define BIND_DEFAULT_ALLOCATOR(T, Manager) \
	Manager.setNewFunc(std::bind(&defaultNewFunc<T>));

template<typename T>
class SerializedObjectPool
{
public:
	SerializedObjectPool(bool a_bPool = false)
		: m_NewFunc(nullptr)
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
	
	void setNewFunc(std::function<std::shared_ptr<T>()> a_NewFunc)
	{
		m_NewFunc = a_NewFunc;
	}

	unsigned int count()
	{
		return m_ObjectPool.size();
	}

	void clear()
	{
		m_ObjectPool.clear();
		m_FreeSlot.clear();
	}

	unsigned int retain(std::shared_ptr<T>* a_ppOutput = nullptr)
	{
		unsigned int l_Res = m_ReqFunc();
		if( nullptr != a_ppOutput ) *a_ppOutput = m_ObjectPool[l_Res];
		return l_Res;
	}

	void release(unsigned int a_ID)
	{
		m_RecFunc(a_ID);
	}

	std::shared_ptr<T> getItem(unsigned int a_ID)
	{
		return m_ObjectPool[a_ID];
	}
	
	virtual std::shared_ptr<T> operator[](unsigned int a_ID)
	{
		return m_ObjectPool[a_ID];
	}

private:
	// always new/delete
	unsigned int newItem()
	{
		std::shared_ptr<T> l_pNewItem = m_NewFunc();
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
		m_ObjectPool[a_ID] = nullptr;
		m_FreeSlot.push_back(a_ID);
	}

	// new once, use until clear
	unsigned int requestItem()
	{	
		unsigned int l_ID = 0;
		if( m_FreeSlot.empty() )
		{
			std::shared_ptr<T> l_pNewItem = m_NewFunc();
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
	std::function<std::shared_ptr<T>()> m_NewFunc;

	std::vector< std::shared_ptr<T> > m_ObjectPool;
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
		if( m_StringMap.empty() ) initMap();\
		return m_Strings[static_cast<unsigned int>(a_Key)];\
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
		splitString(wxT(' '), l_Src, m_Strings);\
		for( unsigned int i=0 ; i<m_Strings.size() ; ++i ) m_StringMap[m_Strings[i]] = l_KeyList[i];\
	}										\
	static std::vector<wxString> m_Strings;\
	static std::map<wxString, Key> m_StringMap;\
};

#define STRING_ENUM_CLASS_INST(classname) \
std::vector<wxString> classname::m_Strings;\
std::map<wxString, classname::Key> classname::m_StringMap;

class VirtualMemoryPool
{
public:
	VirtualMemoryPool();
	virtual ~VirtualMemoryPool();
	
	int malloc(int a_Size);//return offset
	void free(int a_Offset);
	void purge();

	void init(int a_Size);
	void extend(int a_Size);

	int getCurrentSize(){ return m_CurrSize; }

private:
	int m_CurrSize;
	std::map<int, int> m_FreeSpaceList, m_AllocateMap;
};

enum TextureType
{
	TEXTYPE_SIMPLE_2D = 0,
	TEXTYPE_SIMPLE_2D_ARRAY,
	TEXTYPE_SIMPLE_CUBE,
	TEXTYPE_SIMPLE_CUBE_ARRAY,
	TEXTYPE_SIMPLE_3D,
	TEXTYPE_DEPTH_STENCIL_VIEW,
	TEXTYPE_RENDER_TARGET_VIEW,
};

STRING_ENUM_CLASS(PixelFormat,
	unknown,
    rgba32_typeless,
    rgba32_float,
    rgba32_uint,
    rgba32_sint,
    rgb32_typeless,
    rgb32_float,
    rgb32_uint,
    rgb32_sint,
    rgba16_typeless,
    rgba16_float,
    rgba16_unorm,
    rgba16_uint,
    rgba16_snorm,
    rgba16_sint,
    rg32_typeless,
    rg32_float,
    rg32_uint,
    rg32_sint,
    r32g8x24_typeless,
    d32_float_s8x24_uint,
    r32_float_x8x24_typeless,
    x32_typeless_g8x24_uint,
    rgb10a2_typeless,
    rgb10a2_unorm,
    rgb10a2_uint,
    r11g11b10_float,
    rgba8_typeless,
    rgba8_unorm,
    rgba8_unorm_srgb,
    rgba8_uint,
    rgba8_snorm,
    rgba8_sint,
    rg16_typeless,
    rg16_float,
    rg16_unorm,
    rg16_uint,
    rg16_snorm,
    rg16_sint,
    r32_typeless,
    d32_float,
    r32_float,
    r32_uint,
    r32_sint,
    r24g8_typeless,
    d24_unorm_s8_uint,
    r24_unorm_x8_typeless,
    x24_typeless_g8_uint,
    rg8_typeless,
    rg8_unorm,
    rg8_uint,
    rg8_snorm,
    rg8_sint,
    r16_typeless,
    r16_float,
    d16_unorm,
    r16_unorm,
    r16_uint,
    r16_snorm,
    r16_sint,
    r8_typeless,
    r8_unorm,
    r8_uint,
    r8_snorm,
    r8_sint,
    a8_unorm,
    r1_unorm,
    rgb9e5,
    rgbg8_unorm,
    grgb8_unorm,
    bc1_typeless,
    bc1_unorm,
    bc1_unorm_srgb,
    bc2_typeless,
    bc2_unorm,
    bc2_unorm_srgb,
    bc3_typeless,
    bc3_unorm,
    bc3_unorm_srgb,
    bc4_typeless,
    bc4_unorm,
    bc4_snorm,
    bc5_typeless,
    bc5_unorm,
    bc5_snorm,
    b5g6r5_unorm,
    bgr5a1_unorm,
    bgra8_unorm,
    bgrx8_unorm,
    rgb10_xr_bias_a2_unorm,
    bgra8_typeless,
    bgra8_unorm_srgb,
    bgrx8_typeless,
    bgrx8_unorm_srgb,
    bc6h_typeless,
    bc6h_uf16,
    bc6h_sf16,
    bc7_typeless,
    bc7_unorm,
    bc7_unorm_srgb,
    ayuv,
    y410,
    y416,
    nv12,
    p010,
    p016,
    opaque420,
    yuy2,
    y210,
    y216,
    nv11,
    ai44,
    ia44,
    p8,
    ap8,
    bgra4_unorm,
    p208,
    v208,
    v408,
    uint)
unsigned int getPixelSize(PixelFormat::Key a_Key);

enum OctreePos
{
	OCT_NX_NY_NZ = 0,
	OCT_NX_NY_PZ,
	OCT_NX_PY_NZ,
	OCT_NX_PY_PZ,
	OCT_PX_NY_NZ,
	OCT_PX_NY_PZ,
	OCT_PX_PY_NZ,
	OCT_PX_PY_PZ,

	NUM_OCT_POS
};

template<typename T>
class SearchPathSystem
{ 
public:
	SearchPathSystem(std::function<void(std::shared_ptr<T>, wxString)> a_pFileLoader, std::function<std::shared_ptr<T>()> a_pAllocFunc)
		: m_FileLoader(a_pFileLoader)
		, m_ManagedObject()
	{
		m_ManagedObject.setNewFunc(a_pAllocFunc);
	}

	virtual ~SearchPathSystem()
	{
		m_FileIDMap.clear();
		m_ManagedObject.clear();
	}

	std::pair<int, std::shared_ptr<T> > getData(wxString a_Filename)
	{
		std::shared_ptr<T> l_pNewFile = nullptr;
		int l_ID = -1;
		wxString l_FilePath(wxT(""));
		{
			std::lock_guard<std::mutex> l_Guard(m_Locker);

			auto it = m_FileIDMap.find(a_Filename);
			if( m_FileIDMap.end() != it ) return std::make_pair(it->second, m_ManagedObject[it->second]);

			l_FilePath = findFullPath(a_Filename);
			if( l_FilePath.empty() ) return std::make_pair(-1, nullptr);

			l_ID = m_ManagedObject.retain(&l_pNewFile);
			m_FileIDMap.insert(std::make_pair(l_FilePath, l_ID));
		}
		m_FileLoader(l_pNewFile, l_FilePath);
		return std::make_pair(l_ID, l_pNewFile);
	}

	std::shared_ptr<T> getData(int a_ID)
	{
		return m_ManagedObject[a_ID];
	}

	std::pair<int, std::shared_ptr<T> > addData(wxString a_Path)
	{
		std::lock_guard<std::mutex> l_Guard(m_Locker);

		std::shared_ptr<T> l_pNewFile = nullptr;
		int l_ID = m_ManagedObject.retain(&l_pNewFile);
		m_FileIDMap.insert(std::make_pair(a_Path, l_ID));
		return std::make_pair(l_ID, l_pNewFile);
	}

	void removeData(wxString a_Filename)
	{
		std::lock_guard<std::mutex> l_Guard(m_Locker);
		auto it = m_FileIDMap.find(a_Filename);
		if( m_FileIDMap.end() == it ) return;

		m_ManagedObject.release(it->second);
		m_FileIDMap.erase(it);
	}

	wxString findFullPath(wxString a_Filename)
	{
		if( a_Filename.StartsWith("./") ) a_Filename = a_Filename.Remove(0, 2);
		
		for( unsigned int i=0 ; i<m_SearchPath.size() ; ++i )
		{
			wxString l_FilePath(m_SearchPath[i] + a_Filename);
			if( wxFileExists(l_FilePath) ) return l_FilePath;
		}
		return wxT("");
	}
		
	void addSearchPath(wxString a_Path)
	{
		a_Path.Replace(wxT("\\"), wxT("/"));
		if( !a_Path.EndsWith("/") ) a_Path += wxT("/");
		assert(std::find(m_SearchPath.begin(), m_SearchPath.end(), a_Path) == m_SearchPath.end());
		m_SearchPath.push_back(a_Path);
	}
	
	void gc()
	{
		std::vector<wxString> l_EraseKey;
		for( auto it = m_FileIDMap.begin() ; it != m_FileIDMap.end() ; ++it )
		{
			if( 1 == m_ManagedObject[it->second].use_count() ) m_ManagedObject.release(it->second);
			l_EraseKey.push_back(it->first);
		}

		for( unsigned int i=0 ; i<l_EraseKey.size() ; ++i ) m_FileIDMap.erase(l_EraseKey[i]);
	}

	void finalize()
	{
		m_ManagedObject.clear();
		m_FileIDMap.clear();
	}

	void updateFilepath(wxString a_Old, wxString a_New)
	{
		auto it = m_FileIDMap.find(a_Old);
		if( m_FileIDMap.end() == it ) return;
		int l_Idx = it->second;
		m_FileIDMap.erase(it);
		m_FileIDMap.insert(std::make_pair(a_New, l_Idx));
	}

private:
	std::map<wxString, int> m_FileIDMap;
	SerializedObjectPool<T> m_ManagedObject;
	std::vector<wxString> m_SearchPath;
	std::function<void(std::shared_ptr<T>, wxString)> m_FileLoader;

	std::mutex m_Locker;
};

class ImageAtlas
{
public:
	struct NodeInfo
	{
		NodeInfo() : m_Index(0), m_ID(0), m_Offset(0, 0), m_Size(0, 0){}

		unsigned int m_Index;
		unsigned int m_ID;
		glm::ivec2 m_Offset;
		glm::ivec2 m_Size;
	};

	ImageAtlas(glm::ivec2 a_Size, unsigned int a_InitArraySize);
	virtual ~ImageAtlas();

	unsigned int allocate(glm::ivec2 a_Size);
	void release(unsigned int a_ID);
	void releaseAll();

	NodeInfo& getInfo(int a_ID);
	glm::ivec2 getMaxSize(){ return m_MaxSize; }
	unsigned int getArraySize(){ return m_CurrArraySize; }
	void setExtendSize(unsigned int a_Extend);

private:
	struct SplitNode
	{
		SplitNode();
		~SplitNode();

		void release();
			    
		int m_Parent;
		int m_Left, m_Right;
		NodeInfo m_NodeInfo;
		bool m_bUsed;

		SerializedObjectPool<SplitNode> *m_pRefPool;
	};

	bool insertNode(unsigned int a_Node, glm::ivec2 a_Size, unsigned int &a_Output);
	
	SerializedObjectPool<SplitNode> m_NodePool;
	std::vector<unsigned int> m_Roots;
	glm::ivec2 m_MaxSize;
	unsigned int m_CurrArraySize;
	unsigned int m_ExtendSize;
};

class ThreadPool
{
public:
	ThreadPool(unsigned int a_NumThread);
	virtual ~ThreadPool();

	void addJob(std::function<void()> a_Job);
	void join();

private:
	void loop();

	bool m_bLooping;
	std::deque<std::function<void()>> m_JobQueue;
	std::vector<std::thread> m_Threads;
	std::condition_variable m_Signal;
	std::mutex m_SignalLock, m_QueueLock;
	unsigned int m_WorkingCount;
};
/*class ThreadEventCallback
{
public:
	// don't use this constructor 
	ThreadEventCallback();
	virtual ~ThreadEventCallback();

	static ThreadEventCallback& getThreadLocal();
	void addEndEvent(std::function<void()> a_Func);

private:
	std::vector<std::function<void()> > m_EndEvents;
};*/

}

#endif