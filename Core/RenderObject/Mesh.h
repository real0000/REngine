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
class IntersectHelper;

class RenderableMesh : public RenderableComponent
{
	friend class MeshBatcher;
	COMPONENT_HEADER(RenderableMesh)
public:
	typedef std::pair<int, std::shared_ptr<Asset>> MaterialData;
	union SortKey
	{
		uint64 m_Key;
		struct
		{
			uint64 m_bValid : 1;
			uint64 m_MaterialID : 27;
			uint64 m_MeshID : 27;
			uint64 m_SubMeshIdx : 9;
		} m_Members;
	};
public:
	virtual ~RenderableMesh();
	
	virtual void postInit();
	virtual void preEnd();
	virtual void hiddenFlagChanged();
	virtual void transformListener(const glm::mat4x4 &a_NewTransform);

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
	MaterialData getMaterial(unsigned int a_Slot);
	SortKey getSortKey(unsigned int a_Slot);
	unsigned int getMeshIdx(){ return m_MeshIdx; }
	int getWorldOffset(){ return m_WorldOffset; }
	IntersectHelper* getHelper(){ return m_pHelper; }

	template<typename T>
	void setMaterialParam(unsigned int a_Slot, std::string a_Name, T a_Param)
	{
		auto it = m_Materials.find(a_Slot);
		if( m_Materials.end() == it ) return;
		it->second.second->getComponent<MaterialAsset>()->setParam(a_Name, it->second.first, a_Param);
	}

protected:
	RenderableMesh(std::shared_ptr<Scene> a_pRefScene, std::shared_ptr<SceneNode> a_pNode);

private:
	void syncKeyMap();

	std::shared_ptr<Asset> m_pMesh;
	unsigned int m_MeshIdx;
	std::unordered_map<int, MaterialData> m_Materials;
	std::map<int, SortKey> m_KeyMap;

	bool m_bStatic;
	bool m_bShadowed;
	bool m_bAlphaTestShadow;
	int m_SkinOffset;
	int m_WorldOffset;
	glm::aabb m_BoundingBox;
	
	IntersectHelper *m_pHelper;
};

}

#endif