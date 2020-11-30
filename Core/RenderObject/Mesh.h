// Mesh.h
//
// 2017/09/20 Ian Wu/Real0000
//

#ifndef _MESH_H_
#define _MESH_H_

#include "RenderObject.h"

namespace R
{

class Asset;

class RenderableMesh : public RenderableComponent
{
	friend class EngineComponent;
	friend class MeshBatcher;
	typedef std::pair<int, std::shared_ptr<Asset>> MaterialData;
	COMPONENT_HEADER(RenderableMesh)
public:
	union SortKey
	{
		uint64 m_Key;
		struct
		{
			unsigned int m_bValid : 1;
			unsigned int m_MaterialID : 27;
			unsigned int m_MeshID : 27;
			unsigned int m_SubMeshIdx : 9;
		} m_Members;
	};
public:
	virtual ~RenderableMesh();
	
	virtual void start();
	virtual void end();
	virtual void hiddenFlagChanged();
	virtual void updateListener(float a_Delta);
	virtual void transformListener(glm::mat4x4 &a_NewTransform);

	virtual void loadComponent(boost::property_tree::ptree &a_Src);
	virtual void saveComponent(boost::property_tree::ptree &a_Dst);

	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed(){ return m_bShadowed; }
	virtual void setStatic(bool a_bStatic);
	virtual bool isStatic(){ return m_bStatic; }

	void setMesh(std::shared_ptr<Asset> a_pAsset, unsigned int a_MeshIdx);
	std::shared_ptr<Asset> getMesh(){ return m_pMesh; }
	void removeMaterial(unsigned int a_Slot);
	void setMaterial(unsigned int a_Slot, std::shared_ptr<Asset> a_pAsset);// must be material asset
	std::shared_ptr<Asset> getMaterial(unsigned int a_Slot);
	SortKey getSortKey(unsigned int a_Slot);
	unsigned int getMeshIdx(){ return m_MeshIdx; }
	int getWorldOffset(){ return m_WorldOffset; }

	template<typename T>
	void setMaterialParam(unsigned int a_Slot, std::string a_Name, T a_Param)
	{
		auto it = m_Materials.find(a_Slot);
		if( m_Materials.end() == it ) return;
		it->second.second->getComponent<MaterialAsset>()->setParam(a_Name, it->second.first, a_Param);
	}

private:
	RenderableMesh(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode);
	void syncKeyMap();

	std::shared_ptr<Asset> m_pMesh;
	unsigned int m_MeshIdx;
	std::map<int, MaterialData> m_Materials;
	std::map<int, SortKey> m_KeyMap;

	bool m_bStatic;
	bool m_bShadowed;
	bool m_bAlphaTestShadow;
	int m_SkinOffset;
	int m_WorldOffset;
};

}

#endif