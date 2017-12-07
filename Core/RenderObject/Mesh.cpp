// Mesh.cpp
//
// 2017/09/20 Ian Wu/Real0000
//

#include "CommonUtil.h"
#include "RGDeviceWrapper.h"
#include "Core.h"
#include "Scene/Scene.h"
#include "Scene/Graph/ScenePartition.h"

#include "Material.h"
#include "Mesh.h"

namespace R
{

#pragma region RenderableMesh
//
// RenderableMesh
//
RenderableMesh::RenderableMesh(SharedSceneMember *a_pMember, std::shared_ptr<SceneNode> a_pNode)
	: RenderableComponent(a_pMember, a_pNode)
	, m_pVtxBuffer(nullptr), m_pIndexBuffer(nullptr), m_DrawParam(0, 0)
	, m_pMaterial(nullptr)
	, m_BatchID(-1), m_CommandID(-1)
	, m_BaseBounding(1.0f, 1.0f, 1.0f)
	, m_bShadowed(true)
	, m_bValidCheckRequired(true)
{
	m_Flag.m_bNeedRebatch = false;
	m_Flag.m_bNeedUavSync = false;
	m_Flag.m_bFlagUpdated = false;

	addTransformListener();
}

RenderableMesh::~RenderableMesh()
{
}

void RenderableMesh::start()
{
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
}

void RenderableMesh::end()
{
	m_pVtxBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_pMaterial = nullptr;

	if( isHidden() ) return;
	
	SharedSceneMember *l_pMembers = getSharedMember();
	std::shared_ptr<RenderableMesh> l_pThis = shared_from_base<RenderableMesh>();

	l_pMembers->m_pGraphs[SharedSceneMember::GRAPH_MESH]->remove(l_pThis);
	l_pMembers->m_pBatcher->remove(l_pThis);
}

void RenderableMesh::hiddenFlagChanged()
{
	if( isHidden() )
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->remove(shared_from_base<RenderableMesh>());
		removeTransformListener();
	}
	else
	{
		getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->add(shared_from_base<RenderableMesh>());
		addTransformListener();
	}
}

void RenderableMesh::updateListener(float a_Delta)
{
	if( m_bValidCheckRequired )
	{
		if( nullptr == m_pMaterial ||
			nullptr == m_pVtxBuffer ||
			nullptr == m_pIndexBuffer ||
			!m_pVtxBuffer->valid() ||
			!m_pIndexBuffer->valid()) return ;
		m_bValidCheckRequired = false;
	}

	if( GDEVICE()->supportExtraIndirectCommand() )
	{
		 if( m_pMaterial->needRebatch() ) m_Flag.m_bNeedRebatch = true;
		 if( m_pMaterial->needUavUpdate() ) m_Flag.m_bNeedUavSync = true;
		 m_pMaterial->syncEnd();
	}

	if( m_Flag.m_bNeedRebatch )
	{
		getSharedMember()->m_pBatcher->batch(shared_from_base<RenderableMesh>());
		m_Flag.m_bNeedRebatch = false;
	}
	if( m_Flag.m_bNeedUavSync )
	{
		getSharedMember()->m_pBatcher->update(shared_from_base<RenderableMesh>());
		m_Flag.m_bNeedUavSync = false;
	}
	if( m_Flag.m_bFlagUpdated )
	{
		unsigned int l_Flag = 0;
		if( m_bShadowed ) l_Flag |= 0x00000001 << 0;
		getSharedMember()->m_pBatcher->setFlag(shared_from_base<RenderableMesh>(), l_Flag);
		m_Flag.m_bFlagUpdated = false;
	}
}

void RenderableMesh::transformListener(glm::mat4x4 &a_NewTransform)
{
	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	decomposeTRS(a_NewTransform, l_Trans, l_Scale, l_Rot);

	boundingBox().m_Center = l_Trans;
	boundingBox().m_Size = l_Scale * m_BaseBounding;
}

void RenderableMesh::setShadowed(bool a_bShadow)
{
	if( m_bShadowed == a_bShadow ) return;
	m_bShadowed = a_bShadow;
	m_Flag.m_bFlagUpdated = true;
}

bool RenderableMesh::getShadowed()
{
	return m_bShadowed;
}

void RenderableMesh::setMeshData(std::shared_ptr<VertexBuffer> a_pVtxBuffer, std::shared_ptr<IndexBuffer> a_pIndexBuffer, std::pair<int, int> a_DrawParam, glm::vec3 a_BoxSize)
{
	m_BaseBounding = a_BoxSize;
	m_pVtxBuffer = a_pVtxBuffer;
	m_pIndexBuffer = a_pIndexBuffer;
	m_DrawParam = a_DrawParam;
	
	glm::vec3 l_Trans, l_Scale;
	glm::quat l_Rot;
	decomposeTRS(getSharedMember()->m_pSceneNode->getTransform(), l_Trans, l_Scale, l_Rot);
	boundingBox().m_Center = l_Trans;
	boundingBox().m_Size = l_Scale * m_BaseBounding;
	
	getSharedMember()->m_pGraphs[SharedSceneMember::GRAPH_MESH]->update(shared_from_base<RenderableMesh>());

	if( !GDEVICE()->supportExtraIndirectCommand() )
	{
		m_Flag.m_bNeedRebatch = true;
		m_Flag.m_bNeedUavSync = true;
	}
	else
	{
		m_Flag.m_bNeedUavSync = true;	
	}

	m_bValidCheckRequired = nullptr == m_pMaterial ||
							nullptr == m_pVtxBuffer ||
							nullptr == m_pIndexBuffer ||
							!m_pVtxBuffer->valid() ||
							!m_pIndexBuffer->valid();
}

void RenderableMesh::setMaterial(std::shared_ptr<Material> a_pMaterial)
{
	if( a_pMaterial == m_pMaterial ) return;

	if( GDEVICE()->supportExtraIndirectCommand() )
	{
		if( nullptr == m_pMaterial ||
			!m_pMaterial->canBatch(a_pMaterial) )
		{
			m_Flag.m_bNeedRebatch = true;
			m_Flag.m_bNeedUavSync = true;
		}
		else
		{
			m_Flag.m_bNeedUavSync = true;
		}
	}
	else
	{
		m_Flag.m_bNeedRebatch = true;
		m_Flag.m_bNeedUavSync = true;
	}
	m_pMaterial = a_pMaterial;
	
	m_bValidCheckRequired = nullptr == m_pMaterial ||
							nullptr == m_pVtxBuffer ||
							nullptr == m_pIndexBuffer ||
							!m_pVtxBuffer->valid() ||
							!m_pIndexBuffer->valid();
}
#pragma endregion

#pragma region MeshBatcher
#pragma region MeshBatcher::UavBuffer
//
// MeshBatcher::UavBuffer
//
MeshBatcher::UavBuffer::UavBuffer()
	: m_ID(0), m_Size(1024)
	, m_pBuffer(nullptr)
	, m_bDirty(false)
{
	m_ID = GDEVICE()->requestUavBuffer(m_pBuffer, sizeof(unsigned int), m_Size / sizeof(unsigned int));
	m_FreeSpace[0] = m_Size;
}

MeshBatcher::UavBuffer::~UavBuffer()
{
	GDEVICE()->freeUavBuffer(m_ID);
	m_FreeSpace.clear();
	m_AllocateMap.clear();
}

int MeshBatcher::UavBuffer::malloc(unsigned int a_Size)
{
	int l_Res = -1;
	for( auto it = m_FreeSpace.begin() ; m_FreeSpace.end() != it ; ++it )
	{
		if( it->second >= (int)a_Size )
		{
			l_Res = it->first;
			int l_NewOffset = it->first + a_Size;
			int l_NewSize = it->second - a_Size;
			m_FreeSpace.erase(it);
			if( 0 != l_NewSize ) m_FreeSpace[l_NewOffset] = l_NewSize;
			break;
		}
	}
	if( -1 != l_Res ) m_AllocateMap[l_Res] = a_Size;
	else
	{
		unsigned int l_NewSize = m_Size *= 2;
		if( m_FreeSpace.empty() )
		{
			while( l_NewSize - m_Size < a_Size ) l_NewSize *= 2;
			m_AllocateMap.insert(std::make_pair(m_Size, a_Size));
			if( m_Size + a_Size != l_NewSize ) m_FreeSpace.insert(std::make_pair(m_Size + a_Size, l_NewSize - (m_Size + a_Size)));
		}
		else
		{
			std::pair<int, int> l_LastSpace = *m_FreeSpace.rbegin();
			while( l_NewSize - m_Size + l_LastSpace.second < a_Size ) l_NewSize *= 2;
			m_AllocateMap.insert(std::make_pair(l_LastSpace.first, a_Size));

			auto l_EraseIt = m_FreeSpace.rbegin().base();
			++l_EraseIt;
			m_FreeSpace.erase(l_EraseIt);
			if( l_LastSpace.first + a_Size != l_NewSize )
			{
				m_FreeSpace.insert(std::make_pair(l_LastSpace.first + a_Size, l_NewSize - (l_LastSpace.first + a_Size)));
			}
		}
		
		m_Size = l_NewSize;
		GDEVICE()->resizeUavBuffer(m_ID, m_pBuffer, m_Size / sizeof(unsigned int));
		
	}
	return l_Res;
}

void MeshBatcher::UavBuffer::free(int a_Offset)
{
	int l_Size = 0;
	{
		auto it = m_AllocateMap.find(a_Offset);
		assert(m_AllocateMap.end() != it);
		l_Size = it->second;
		m_AllocateMap.erase(l_Size);
	}

	int l_PrevOffset = a_Offset;
	int l_NextOffset = a_Offset + l_Size;

	auto l_PrevIt = m_FreeSpace.end();
	for( auto it = m_FreeSpace.begin() ; m_FreeSpace.end() != it ; ++it )
	{
		if( it->first + it->second == l_PrevOffset )
		{
			it->second += l_Size;
			l_PrevIt = it;
		}
		else if( it->first == l_NextOffset )
		{
			if( m_FreeSpace.end() != l_PrevIt )
			{
				l_PrevIt->second += it->second;
				m_FreeSpace.erase(it);
			}
			else
			{
				int l_NewSize = it->second + l_Size;
				m_FreeSpace.erase(it);
				m_FreeSpace[a_Offset] = l_NewSize;
			}
			break;
		}
		else if( it->first > l_NextOffset )
		{
			m_FreeSpace[a_Offset] = l_Size;
			break;
		}
	}
}

void MeshBatcher::UavBuffer::sync()
{
	if( !m_bDirty ) return;
	GDEVICE()->syncUavBuffer(true, 1, m_ID);
	m_bDirty = false;
}
#pragma endregion

//
// MeshBatcher
//
MeshBatcher::MeshBatcher()
	: m_pCommandPool(new UavBuffer()), m_pCmdUnitPool(new UavBuffer())
{
}

MeshBatcher::~MeshBatcher()
{
	m_UnitMap.clear();
	SAFE_DELETE(m_pCommandPool)
	SAFE_DELETE(m_pCmdUnitPool)
}
	
void MeshBatcher::batch(std::shared_ptr<RenderableMesh> a_pMesh)
{
	std::lock_guard<std::mutex> l_Guard(m_Locker);

	freeBatchID(a_pMesh->m_BatchID);
	freeCommandOffset(a_pMesh->m_CommandID);

	std::vector<RenderUnit>::iterator l_UnitIt;
	if( GDEVICE()->supportExtraIndirectCommand() )
	{
		l_UnitIt = std::find_if(m_UnitMap.begin(), m_UnitMap.end(), [a_pMesh](RenderUnit const &a_Unit)->bool
		{
			return (nullptr != a_Unit.m_pMaterial) && a_Unit.m_pMaterial->canBatch(a_pMesh->getMaterial());
		});
	}
	else
	{
		l_UnitIt = std::find_if(m_UnitMap.begin(), m_UnitMap.end(), [a_pMesh](RenderUnit const &a_Unit)->bool
		{
			return (nullptr != a_Unit.m_pMaterial) &&
				a_Unit.m_pMaterial == a_pMesh->getMaterial() &&
				a_Unit.m_pVtxBuffer == a_pMesh->getVtxBuffer() &&
				a_Unit.m_pIndexBuffer == a_pMesh->getIndexBuffer();
		});
	}
	if( m_UnitMap.end() == l_UnitIt )
	{
		if( m_FreeUnitSlot.empty() )
		{
			RenderUnit l_NewUnit;
			l_NewUnit.m_pVtxBuffer = a_pMesh->getVtxBuffer();
			l_NewUnit.m_pIndexBuffer = a_pMesh->getIndexBuffer();
			l_NewUnit.m_pMaterial = a_pMesh->getMaterial();
			++l_NewUnit.m_RefCount;

			a_pMesh->m_BatchID = m_UnitMap.size();
			m_UnitMap.push_back(l_NewUnit);
		}
		else
		{
			a_pMesh->m_BatchID = m_FreeUnitSlot.front();
			m_FreeUnitSlot.pop_front();

			RenderUnit &l_NewUnit = m_UnitMap[a_pMesh->m_BatchID];
			l_NewUnit.m_pVtxBuffer = a_pMesh->getVtxBuffer();
			l_NewUnit.m_pIndexBuffer = a_pMesh->getIndexBuffer();
			l_NewUnit.m_pMaterial = a_pMesh->getMaterial();
			++l_NewUnit.m_RefCount;
		}
	}
	else
	{
		a_pMesh->m_BatchID = l_UnitIt - m_UnitMap.begin();
		++m_UnitMap[a_pMesh->m_BatchID].m_RefCount;
	}
	
	int l_Offset = m_pCmdUnitPool->malloc(sizeof(CommandUnit));
	a_pMesh->m_CommandID = l_Offset / sizeof(CommandUnit);
	CommandUnit *l_pCmdOffset = getCommandUnit(a_pMesh->m_CommandID);
	l_pCmdOffset->m_Offset = m_pCommandPool->malloc(a_pMesh->getMaterial()->getProgram()->getIndirectCommandSize());
	m_pCmdUnitPool->m_bDirty = true;
}

void MeshBatcher::remove(std::shared_ptr<RenderableMesh> a_pMesh)
{
	std::lock_guard<std::mutex> l_Guard(m_Locker);

	freeBatchID(a_pMesh->m_BatchID);
	freeCommandOffset(a_pMesh->m_CommandID);

	a_pMesh->m_BatchID = -1;
	a_pMesh->m_CommandID = -1;
}

void MeshBatcher::update(std::shared_ptr<RenderableMesh> a_pMesh)
{
	CommandUnit *l_pCmdOffset = getCommandUnit(a_pMesh->m_CommandID);
	if( nullptr == l_pCmdOffset ) return;

	char *l_pBuffer = getCommandPoolPtr(l_pCmdOffset->m_Offset);
	a_pMesh->m_pMaterial->assignIndirectCommand(l_pBuffer, a_pMesh->m_pVtxBuffer, a_pMesh->m_pIndexBuffer, a_pMesh->m_DrawParam);
	m_pCommandPool->m_bDirty = true;
}

void MeshBatcher::setFlag(std::shared_ptr<RenderableMesh> a_pMesh, unsigned int a_Flag)
{
	CommandUnit *l_pCmdOffset = getCommandUnit(a_pMesh->m_CommandID);
	if( nullptr == l_pCmdOffset ) return;

	l_pCmdOffset->m_Flags = a_Flag;
	m_pCmdUnitPool->m_bDirty = true;
}

void MeshBatcher::flush()
{
	m_pCmdUnitPool->sync();
	m_pCommandPool->sync();
}

void MeshBatcher::freeBatchID(int a_ID)
{
	if( -1 == a_ID ) return;

	--m_UnitMap[a_ID].m_RefCount;
	if( 0 == m_UnitMap[a_ID].m_RefCount )
	{
		m_UnitMap[a_ID].m_pIndexBuffer = nullptr;
		m_UnitMap[a_ID].m_pVtxBuffer = nullptr;
		m_UnitMap[a_ID].m_pMaterial = nullptr;
		m_FreeUnitSlot.push_back(a_ID);
	}
}

void MeshBatcher::freeCommandOffset(int a_Slot)
{
	if( -1 == a_Slot ) return ;

	CommandUnit *l_pUnit = getCommandUnit(a_Slot);
	m_pCommandPool->free(l_pUnit->m_Offset);
	m_pCmdUnitPool->free(a_Slot * sizeof(MeshBatcher::CommandUnit));
}

MeshBatcher::CommandUnit* MeshBatcher::getCommandUnit(int a_Slot)
{
	if( -1 == a_Slot ) return nullptr;
	return reinterpret_cast<MeshBatcher::CommandUnit *>(m_pCmdUnitPool->m_pBuffer + sizeof(MeshBatcher::CommandUnit) * a_Slot);
}

char* MeshBatcher::getCommandPoolPtr(int a_Offset)
{
	return m_pCommandPool->m_pBuffer + a_Offset;
}
#pragma endregion

}