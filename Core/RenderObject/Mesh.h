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
	friend EngineComponent;
	friend class MeshBatcher;
public:
	virtual ~RenderableMesh();
	
	virtual void start();
	virtual void end();
	virtual void staticFlagChanged();
	virtual void updateListener(float a_Delta);
	virtual void transformListener(glm::mat4x4 &a_NewTransform);
	virtual bool isHidden(){ return m_bHidden; }
	void setHidden(bool a_bHidden);

	virtual void setMeshData(std::shared_ptr<VertexBuffer> a_pVtxBuffer, std::shared_ptr<IndexBuffer> a_pIndexBuffer, std::pair<int, int> a_DrawParam, glm::vec3 a_BoxSize);
	virtual void setMaterial(std::shared_ptr<Material> a_pMaterial);
	std::shared_ptr<Material> getMaterial(){ return m_pMaterial; }
	std::shared_ptr<VertexBuffer> getVtxBuffer(){ return m_pVtxBuffer; }
	std::shared_ptr<IndexBuffer> getIndexBuffer(){ return m_pIndexBuffer; }
	int getCommandID(){ return m_CommandID; }

private:
	RenderableMesh(SharedSceneMember *a_pMember, std::shared_ptr<SceneNode> a_pNode);

	struct
	{
		unsigned char m_bNeedRebatch : 1;// include index
		unsigned char m_bNeedUavSync : 1;
	} m_Flag;
	std::shared_ptr<VertexBuffer> m_pVtxBuffer;
	std::shared_ptr<IndexBuffer> m_pIndexBuffer;
	std::pair<int, int> m_DrawParam;// index count, base vertex
	std::shared_ptr<Material> m_pMaterial;
	int m_BatchID, m_CommandID;
	bool m_bHidden;
	glm::vec3 m_BaseBounding;

	bool m_bValidCheckRequired;
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
		unsigned int m_Flag;
	};
public:
	MeshBatcher();
	virtual ~MeshBatcher();
	
	void batch(std::shared_ptr<RenderableMesh> a_pMesh);
	void remove(std::shared_ptr<RenderableMesh> a_pMesh);
	void update(std::shared_ptr<RenderableMesh> a_pMesh);

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