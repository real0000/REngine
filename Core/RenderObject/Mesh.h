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
public:
	virtual ~RenderableMesh();
	
	virtual void start();
	virtual void end();
	virtual void hiddenFlagChanged();
	virtual void updateListener(float a_Delta);
	virtual void transformListener(glm::mat4x4 &a_NewTransform);

	virtual unsigned int typeID(){ return COMPONENT_MESH; }

	virtual void setShadowed(bool a_bShadow);
	virtual bool getShadowed(){ return m_bShadowed; }
	virtual void setStatic(bool a_bStatic);
	virtual bool isStatic(){ return m_bStatic; }

	void setMesh(std::shared_ptr<Asset> a_pAsset, unsigned int a_MeshIdx);
	std::shared_ptr<Asset> getMesh(){ return m_pMesh; }
	void setMaterial(std::shared_ptr<Asset> a_pAsset);// must be material asset
	std::shared_ptr<Asset> getMaterial(){ return m_pMaterial; }

private:
	RenderableMesh(SharedSceneMember *a_pMember, std::shared_ptr<SceneNode> a_pNode);

	std::shared_ptr<Asset> m_pMesh;
	unsigned int m_MeshIdx;
	std::shared_ptr<Asset> m_pMaterial;

	int m_InstanceID;
	bool m_bStatic;
	bool m_bShadowed;
};

}

#endif