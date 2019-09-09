// Mesh.h
//
// 2017/09/20 Ian Wu/Real0000
//

#ifndef _MESH_H_
#define _MESH_H_

#include "RenderObject.h"

namespace R
{

class Material;
class MeshBatcher;

class RenderableMesh : public RenderableComponent
{
	friend class EngineComponent;
	friend class MeshBatcher;
public:
	virtual ~RenderableMesh();
	
	virtual void start();
	virtual void end();
	virtual void hiddenFlagChanged();
	virtual void updateListener(float a_Delta);
	virtual void transformListener(glm::mat4x4 &a_NewTransform);

	virtual unsigned int typeID(){ return COMPONENT_MESH; }

	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed();

	void setMesh(wxString a_SettingFile, std::function<void()> a_pCallback);
	void setStage(unsigned int a_Stage);
	unsigned int getStage(){ return m_Stage; }
	unsigned int getNumSubMesh(){ return m_SubMeshes.size(); }
	wxString getName(unsigned int a_Idx){ return m_SubMeshes[a_Idx]->m_Name; }
	void setMaterial(unsigned int a_Idx, std::shared_ptr<Material> a_pMaterial);
	std::shared_ptr<Material> getMaterial(unsigned int a_Idx){ return m_SubMeshes[a_Idx]->m_pMaterial; }
	unsigned int getCommandID(unsigned int a_Idx){ return m_SubMeshes[a_Idx]->m_CommandID; }
	std::shared_ptr<VertexBuffer> getVtxBuffer(){ return m_pVtxBuffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_pIndexBuffer; }

private:
	struct SubMeshData
	{
		wxString m_Name;
		std::shared_ptr<Material> m_pMaterial;
		IndirectDrawData m_PartData;
		int m_BatchID, m_CommandID;
		unsigned char m_bNeedRebatch : 1;// include index
		unsigned char m_bNeedUavSync : 1;
		unsigned char m_bVisible : 1;
		unsigned char m_bFlagUpdated : 1;
	};
	RenderableMesh(SharedSceneMember *a_pMember, std::shared_ptr<SceneNode> a_pNode);

	std::shared_ptr<VertexBuffer> m_pVtxBuffer;
	std::shared_ptr<IndexBuffer> m_pIndexBuffer;
	
	glm::aabb m_BaseBounding;
	std::vector< std::shared_ptr<SubMeshData> > m_SubMeshes;
	
	unsigned int m_Stage;
	bool m_bShadowed;
};

class MeshBatcher
{
private:
	struct RenderUnit
	{
		std::shared_ptr<Material> m_pMaterial;
		std::shared_ptr<VertexBuffer> m_pVtxBuffer;
		std::shared_ptr<IndexBuffer> m_pIndexBuffer;
		unsigned int m_RefCount;
	};
	struct UavBuffer
	{
		UavBuffer();
		~UavBuffer();

		int malloc(unsigned int a_Size);
		void free(int a_Offset);
		void sync();

		unsigned int m_ID;
		unsigned int m_Size;
		char *m_pBuffer;
		std::map<int, int> m_FreeSpace, m_AllocateMap;
		bool m_bDirty;
	};
	struct CommandUnit
	{
		unsigned int m_Offset;
		unsigned int m_Flags;
	};
public:
	MeshBatcher();
	virtual ~MeshBatcher();
	
	void batch(std::shared_ptr<RenderableMesh> a_pMesh);
	void remove(std::shared_ptr<RenderableMesh> a_pMesh);
	void update(std::shared_ptr<RenderableMesh> a_pMesh);
	void setFlag(std::shared_ptr<RenderableMesh> a_pMesh, unsigned int a_Flag);

	void flush();

private:
	void freeBatchID(int a_ID);
	void freeCommandOffset(int a_Slot);
	CommandUnit* getCommandUnit(int a_Slot);
	char* getCommandPoolPtr(int a_Offset);

	std::list<unsigned int> m_FreeUnitSlot;
	std::vector<RenderUnit> m_UnitMap;
	std::mutex m_Locker;

	UavBuffer *m_pCommandPool, *m_pCmdUnitPool;
};

}

#endif