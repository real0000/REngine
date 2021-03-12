// ModelImporter.cpp
//
// 2017/06/02 Ian Wu/Real0000
//

#include "RImporters.h"
#include "fbxsdk.h"

namespace R
{

#pragma region ModelNode
//
// ModelNode
//
ModelNode::ModelNode()
	: m_pParent(nullptr), m_NodeName(""), m_Transform(glm::identity<glm::mat4x4>())
{
}

ModelNode::~ModelNode()
{
	for( unsigned int i=0 ; i<m_Children.size() ; ++i ) delete m_Children[i];
	m_Children.clear();
}

ModelNode* ModelNode::find(wxString a_Name)
{
	if( m_NodeName == a_Name ) return this;
	for( unsigned int i=0 ; i<m_Children.size() ; ++i )
	{
		ModelNode *l_pNode = m_Children[i]->find(a_Name);
		if( nullptr != l_pNode ) return l_pNode;
	}
	return nullptr;
}

glm::mat4x4 ModelNode::getAbsoluteTransform()
{
	if( nullptr != m_pParent ) return m_pParent->getAbsoluteTransform() * m_Transform;
	return m_Transform;
}
#pragma endregion

#pragma region ModelData
//
// fbx sdk help function
//
template<typename SrcType, typename TVec>
static void setupVertexData(FbxMesh *a_pSrcMesh, SrcType *a_pSrcData, ModelData::Meshes *a_pTargetMesh, std::function<void(ModelData::Vertex&, TVec)> a_Lambda)
{
    if( nullptr == a_pSrcData ) return;

    switch( a_pSrcData->GetMappingMode() )
    {
        case FbxLayerElement::eByControlPoint:{
            auto &l_DirectArray = a_pSrcData->GetDirectArray();
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    for( int i=0 ; i<a_pSrcMesh->GetControlPointsCount() ; ++i ) a_Lambda(a_pTargetMesh->m_Vertex[i], l_DirectArray[i]);
                    }break;

                case FbxLayerElement::eIndexToDirect:{
                    auto &l_IndexArray = a_pSrcData->GetIndexArray();
                    for( int i=0 ; i<l_IndexArray.GetCount() ; ++i ) a_Lambda(a_pTargetMesh->m_Vertex[i], l_DirectArray[l_IndexArray[i]]);
                    }break;

                default:break;
            }
            }break;

        case FbxLayerElement::eByPolygonVertex:{
            auto &l_DirectArray = a_pSrcData->GetDirectArray();
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    for( int i=0 ; i<a_pSrcMesh->GetPolygonCount() ; ++i )
                    {
                        for( unsigned int j=0 ; j<3 ; ++j )
                        {
                            unsigned int l_PtIdx = a_pSrcMesh->GetPolygonVertex(i, j);
                            a_Lambda(a_pTargetMesh->m_Vertex[l_PtIdx], l_DirectArray[i*3 + j]);
                        }
                    }
                    }break;

                case FbxLayerElement::eIndexToDirect:{
                    auto &l_IndexArray = a_pSrcData->GetIndexArray();
                    for( int i=0 ; i<a_pSrcMesh->GetPolygonCount() ; ++i )
                    {
                        for( unsigned int j=0 ; j<3 ; ++j )
                        {
                            unsigned int l_PtIdx = a_pSrcMesh->GetPolygonVertex(i, j);
                            a_Lambda(a_pTargetMesh->m_Vertex[l_PtIdx], l_DirectArray[l_IndexArray[i*3 + j]]);
                        }
                    }
                    }break;
            }
            }break;

        default:break;
    }
}

static wxString getTextureName(fbxsdk::FbxSurfaceMaterial *a_pMat, const char *a_pName)
{
	fbxsdk::FbxProperty l_TextureProperty = a_pMat->FindProperty(a_pName);
	if( !l_TextureProperty.IsValid() ) return wxT("");

	fbxsdk::FbxLayeredTexture *l_pLayeredTexture = nullptr;
	for( int i=0 ; i<l_TextureProperty.GetSrcObjectCount<FbxLayeredTexture>() ; ++i )
	{
		l_pLayeredTexture = FbxCast<fbxsdk::FbxLayeredTexture>(l_TextureProperty.GetSrcObject<fbxsdk::FbxLayeredTexture>(i));
		if( nullptr != l_pLayeredTexture ) break;
	}
	if( nullptr != l_pLayeredTexture )
	{
		fbxsdk::FbxFileTexture *l_pTexture = FbxCast<fbxsdk::FbxFileTexture>(l_pLayeredTexture->GetSrcObject<fbxsdk::FbxFileTexture>(0));
		return nullptr != l_pTexture ? wxString(l_pTexture->GetRelativeFileName()) : wxT("");
	}

	fbxsdk::FbxFileTexture *l_pTexture = FbxCast<fbxsdk::FbxFileTexture>(l_TextureProperty.GetSrcObject<fbxsdk::FbxFileTexture>(0));
	return nullptr != l_pTexture ? wxString(l_pTexture->GetRelativeFileName()) : wxT("");
}

//
// ModelData
//
ModelData::ModelData()
	: m_pRootNode(nullptr)
{

}

ModelData::~ModelData()
{
	for( unsigned int i=0 ; i<m_Meshes.size() ; ++i ) delete m_Meshes[i];
	m_Meshes.clear();
	m_Materials.clear();
	SAFE_DELETE(m_pRootNode)
}

void ModelData::init(wxString a_Filepath)
{
	FbxManager *l_pSdkManager = FbxManager::Create();
    FbxIOSettings *l_pIOCfg = FbxIOSettings::Create(l_pSdkManager, IOSROOT);
    l_pIOCfg->SetBoolProp(IMP_FBX_MATERIAL, true);
    l_pIOCfg->SetBoolProp(IMP_FBX_TEXTURE, true);
    l_pIOCfg->SetBoolProp(IMP_FBX_LINK, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_SHAPE, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_GOBO, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_ANIMATION, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    l_pSdkManager->SetIOSettings(l_pIOCfg);

    FbxImporter* l_pImporter = FbxImporter::Create(l_pSdkManager, "");
    bool l_bSuccess = l_pImporter->Initialize(a_Filepath.c_str(), -1, l_pIOCfg);
    assert(l_bSuccess);

    FbxScene* l_pScene = FbxScene::Create(l_pSdkManager, "SourceScene");
    l_pImporter->Import(l_pScene);
    l_pImporter->Destroy();

    std::map<FbxNode *, ModelNode *> l_NodeMap;
    std::map<FbxMesh *, unsigned int> l_MeshMap;
	std::vector<ModelNode *> l_NodeVec(l_pScene->GetNodeCount(), nullptr);
    {
        for( unsigned int i=0 ; i<l_NodeVec.size() ; ++i )
        {
            l_NodeVec[i] = new ModelNode();
            FbxNode *l_pCurrNode = l_pScene->GetNode(i);
            l_NodeMap[l_pCurrNode] = l_NodeVec[i];

            //FbxAMatrix &l_FbxMat = l_pCurrNode->EvaluateLocalTransform(FBXSDK_TIME_INFINITE, FbxNode::eSourcePivot, false, true);
			FbxVector4 l_Trans = l_pCurrNode->GetGeometricTranslation(FbxNode::eSourcePivot);
			FbxVector4 l_Scale = l_pCurrNode->GetGeometricScaling(FbxNode::eSourcePivot);
			FbxVector4 l_Rot = l_pCurrNode->GetGeometricRotation(FbxNode::eSourcePivot);
			FbxAMatrix l_FbxMat;
			l_FbxMat.SetTRS(l_Trans, l_Rot, l_Scale);

            l_NodeVec[i]->m_NodeName = l_pCurrNode->GetName();
            for( unsigned int j=0 ; j<4 ; ++j )
            {
                FbxVector4 l_Data = l_FbxMat.GetColumn(j);
                l_NodeVec[i]->m_Transform[j] = glm::vec4(l_Data[0], l_Data[1], l_Data[2], l_Data[3]);
            }

            FbxNodeAttribute *l_pNodeAttr = l_pCurrNode->GetNodeAttribute();
            if( nullptr != l_pNodeAttr )
            {
                switch( l_pNodeAttr->GetAttributeType() )
                {
                    case FbxNodeAttribute::eMesh:{
                        FbxMesh *l_pMesh = static_cast<FbxMesh *>(l_pNodeAttr);

                        unsigned int l_MeshIdx = 0;
                        auto l_MeshIt = l_MeshMap.find(l_pMesh);
                        if( l_MeshIt == l_MeshMap.end() )
                        {
                            l_MeshIdx = l_MeshMap.size();
                            l_MeshMap.insert(std::make_pair(l_pMesh, l_MeshIdx));
                        }
                        else l_MeshIdx = l_MeshIt->second;
                        l_NodeVec[i]->m_RefMesh.push_back(l_MeshIdx);
                        }break;

                    default:break;
                }
            }
        }

		std::vector<ModelNode *> l_TempRootNodes;
        for( unsigned int i=0 ; i<l_NodeVec.size() ; ++i )
        {
            FbxNode *l_pCurrNode = l_pScene->GetNode(i);
            if( nullptr != l_pCurrNode->GetParent() ) l_NodeVec[i]->m_pParent = l_NodeMap[l_pCurrNode->GetParent()];
			else l_TempRootNodes.push_back(l_NodeVec[i]);

            for( int j=0 ; j<l_pCurrNode->GetChildCount() ; ++j )
            {
                ModelNode *l_pChild = l_NodeMap[l_pCurrNode->GetChild(j)];
                l_NodeVec[i]->m_Children.push_back(l_pChild);
            }
        }

		if( !l_NodeVec.empty() )
		{
			if( 1 == l_TempRootNodes.size() ) m_pRootNode = l_TempRootNodes.front();
			else
			{
				m_pRootNode = new ModelNode();
				m_pRootNode->m_NodeName = wxT("RootNode");
				for( unsigned int i=0 ; i<l_TempRootNodes.size() ; ++i )
				{
					l_TempRootNodes[i]->m_pParent = m_pRootNode;
					m_pRootNode->m_Children.push_back(l_TempRootNodes[i]);
				}
			}
		}
    }

    m_Meshes.resize(l_MeshMap.size(), nullptr);
	std::vector<int> l_BoneRecord;
	union MaterialPack
	{
		uint64 m_Key;
		struct PackParam
		{
			uint64 m_BaseColorIdx : 16;
			uint64 m_MetalicIdx : 12;
			uint64 m_RoughnessIdx : 12;
			uint64 m_NormalIdx : 12;
			uint64 m_HeightIdx : 12;
		} m_PackParam;
	};
	const char * c_TextureSlots[TEXUSAGE_TYPECOUNT] = {
		fbxsdk::FbxSurfaceMaterial::sDiffuse,
		fbxsdk::FbxSurfaceMaterial::sReflection,
		fbxsdk::FbxSurfaceMaterial::sSpecular,
		fbxsdk::FbxSurfaceMaterial::sNormalMap,
		fbxsdk::FbxSurfaceMaterial::sDisplacementColor};
	m_Materials.insert(std::make_pair(DEFAULT_EMPTY_MAT_NAME, Material()));
	std::function<glm::vec3(float, float, float)> l_PosAssignFunc = ModelManager::singleton().getFlipYZ() ?
		(std::function<glm::vec3(float, float, float)>)[](float a_X, float a_Y, float a_Z) -> glm::vec3{ return glm::vec3(a_X, a_Z, a_Y); } :
		(std::function<glm::vec3(float, float, float)>)[](float a_X, float a_Y, float a_Z) -> glm::vec3{ return glm::vec3(a_X, a_Y, a_Z); };
    for( auto it = l_MeshMap.begin() ; it != l_MeshMap.end() ; ++it )
    {
        FbxMesh *l_pSrcMesh = it->first;
        unsigned int l_Idx = it->second;
        Meshes *l_pDstMesh = m_Meshes[l_Idx] = new Meshes();
        l_pDstMesh->m_Name = it->first->GetName();
        l_pDstMesh->m_Index = l_Idx;

		FbxNode *l_pRefNode = nullptr;
		glm::vec3 l_BoxCenter(0.0f, 0.0f, 0.0f);
		for( int i=0 ; i<it->first->GetNodeCount() ; ++i )
		{
			l_pRefNode = it->first->GetNode(i);
			l_pDstMesh->m_RefNode.push_back(l_NodeMap[l_pRefNode]);

			FbxVector4 l_FbxBoxMax, l_FbxBoxMin, l_FbxBoxCenter;
			l_pRefNode->EvaluateGlobalBoundingBoxMinMaxCenter(l_FbxBoxMin, l_FbxBoxMax, l_FbxBoxCenter);
			FbxVector4 l_Size((l_FbxBoxMax - l_FbxBoxMin));
			FbxVector4 l_Offset(l_FbxBoxCenter - (l_FbxBoxMax + l_FbxBoxMin) * 0.5f);
			l_pDstMesh->m_BoundingBox.m_Size.x = std::max<float>(l_pDstMesh->m_BoundingBox.m_Size.x, l_Size[0]);
			l_pDstMesh->m_BoundingBox.m_Size.y = std::max<float>(l_pDstMesh->m_BoundingBox.m_Size.y, l_Size[1]);
			l_pDstMesh->m_BoundingBox.m_Size.z = std::max<float>(l_pDstMesh->m_BoundingBox.m_Size.z, l_Size[2]);
			l_pDstMesh->m_BoundingBox.m_Center.x += l_Offset[0];
			l_pDstMesh->m_BoundingBox.m_Center.y += l_Offset[1];
			l_pDstMesh->m_BoundingBox.m_Center.z += l_Offset[2];
		}
		if( !l_pDstMesh->m_RefNode.empty() ) l_pDstMesh->m_BoundingBox.m_Center /= l_pDstMesh->m_RefNode.size();

		l_pRefNode = it->first->GetNode();
		if( l_pDstMesh->m_Name.empty() ) l_pDstMesh->m_Name = l_pRefNode->GetName();

        unsigned int l_NumVtx = l_pSrcMesh->GetControlPointsCount();
        l_pDstMesh->m_Vertex.resize(l_NumVtx);
		l_BoneRecord.resize(l_NumVtx);
		memset(l_BoneRecord.data(), 0, sizeof(int) * l_NumVtx);

        for( unsigned int i=0 ; i<l_NumVtx ; ++i )
        {
            FbxVector4 l_SrcVtx = l_pSrcMesh->GetControlPoints()[i];
            Vertex &l_TargetVtx = l_pDstMesh->m_Vertex[i];
            l_TargetVtx.m_Position = l_PosAssignFunc(l_SrcVtx[0], l_SrcVtx[1], l_SrcVtx[2]);
        }
        
        std::function<void(Vertex&, FbxVector4)> l_SetNormalFunc = [](Vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Normal = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
        setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetNormals(), l_pDstMesh, l_SetNormalFunc);

        std::function<void(Vertex&, FbxVector4)> l_SetTangentFunc = [](Vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Tangent = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
        setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetTangents(), l_pDstMesh, l_SetTangentFunc);

        std::function<void(Vertex&, FbxVector4)> l_SetBinormalFunc = [](Vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Binormal = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
        setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetBinormals(), l_pDstMesh, l_SetBinormalFunc);

		int l_LayerCount = std::min(4, l_pSrcMesh->GetLayerCount());
		for( int i=0 ; i<l_LayerCount ; ++i )
		{
			std::function<void(Vertex&, FbxVector2)> l_SetUVFunc = [&i](Vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord[i/2].x = a_Src[0]; a_Vtx.m_Texcoord[i/2].y = a_Src[1]; };
			std::function<void(Vertex&, FbxVector2)> l_SetUV2Func = [&i](Vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord[i/2].z = a_Src[0]; a_Vtx.m_Texcoord[i/2].w = a_Src[1]; };

			setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(i)->GetUVs(), l_pDstMesh, i % 2 == 0 ? l_SetUVFunc : l_SetUV2Func);
        }
			
        FbxAMatrix l_VtxTrans;
		for( int i=0 ; i<l_pSrcMesh->GetDeformerCount() ; ++i )
		{
			FbxDeformer *l_pDefornmer = l_pSrcMesh->GetDeformer(i);
			if( FbxDeformer::eSkin != l_pDefornmer->GetDeformerType() ) continue;

			l_pDstMesh->m_bHasBone	= true;
			FbxSkin *l_pSkin = static_cast<FbxSkin *>(l_pDefornmer);
			for( int j=0 ; j<l_pSkin->GetClusterCount() ; ++j )
			{
				FbxCluster *l_pCluster = l_pSkin->GetCluster(j);
				l_pCluster->GetTransformLinkMatrix(l_VtxTrans);
				
				int l_BondIndex = m_Bones.size();
				glm::mat4x4 l_DstBonsMat(l_VtxTrans.Get(0, 0), l_VtxTrans.Get(0, 1), l_VtxTrans.Get(0, 2), l_VtxTrans.Get(0, 3)
										,l_VtxTrans.Get(1, 0), l_VtxTrans.Get(1, 1), l_VtxTrans.Get(1, 2), l_VtxTrans.Get(1, 3)
										,l_VtxTrans.Get(2, 0), l_VtxTrans.Get(2, 1), l_VtxTrans.Get(2, 2), l_VtxTrans.Get(2, 3)
										,l_VtxTrans.Get(3, 0), l_VtxTrans.Get(3, 1), l_VtxTrans.Get(3, 2), l_VtxTrans.Get(3, 3));
				m_Bones.push_back(l_DstBonsMat);
				for( int k=0 ; k<l_pCluster->GetControlPointIndicesCount() ; ++k )
				{
					Vertex &l_Vtx = l_pDstMesh->m_Vertex[l_pCluster->GetControlPointIndices()[k]];
					int &l_WeightIdx = l_BoneRecord[l_pCluster->GetControlPointIndices()[k]];
					assert(l_WeightIdx < 4);
					l_Vtx.m_BoneId[l_WeightIdx] = l_BondIndex;
					l_Vtx.m_Weight[l_WeightIdx] = l_pCluster->GetControlPointWeights()[k];
					++l_WeightIdx;
				}
			}
		}

        for( int i=0 ; i<l_pSrcMesh->GetPolygonCount() ; ++i )
        {
            for( unsigned int j=0 ; j<3 ; ++j )
            {
                unsigned int l_PtIdx = l_pSrcMesh->GetPolygonVertex(i, j);
                l_pDstMesh->m_Indicies.push_back(l_PtIdx);        
            }
        }
		
		l_pDstMesh->m_RefMaterial = DEFAULT_EMPTY_MAT_NAME;
		FbxLayerElementArrayTemplate<int> *l_pMaterialIndicies = nullptr;
		if( l_pSrcMesh->GetMaterialIndices(&l_pMaterialIndicies) )
		{
			int l_MatIdx = l_pMaterialIndicies->GetAt(0);
			fbxsdk::FbxSurfaceMaterial *l_pMat = l_pRefNode->GetMaterial(l_MatIdx);
			wxString l_MatName(l_pMat->GetName());
			if( m_Materials.end() != m_Materials.find(l_MatName) ) l_pDstMesh->m_RefMaterial = l_MatName;
			else
			{
				bool l_bEmpty = true;
				Material l_NewMat = Material();
				for( int j=0 ; j<TEXUSAGE_TYPECOUNT ; ++j )
				{
					wxString l_TextureName(getTextureName(l_pMat, c_TextureSlots[j]));
					if( l_TextureName.IsEmpty() ) continue;

					l_bEmpty = false;
					regularFilePath(l_TextureName);
					l_NewMat.insert(std::make_pair((DefaultTextureUsageType)j, l_TextureName));
				}

				if( !l_bEmpty )
				{
					l_pDstMesh->m_RefMaterial = l_MatName;
					m_Materials.insert(std::make_pair(l_MatName, l_NewMat));
				}
			}
		}
    }

    l_pScene->Clear();
	l_pScene->Destroy();
	l_pIOCfg->Destroy();
	l_pSdkManager->Destroy();
}

ModelNode* ModelData::find(wxString a_Name)
{
	return nullptr != m_pRootNode ? m_pRootNode->find(a_Name) : nullptr;
}
#pragma endregion

#pragma region ModelManager
//
// ModelManager
//
ModelManager& ModelManager::singleton()
{
	static ModelManager s_Inst;
	return s_Inst;
}

ModelManager::ModelManager()
	: SearchPathSystem(std::bind(&ModelManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&defaultNewFunc<ModelData>))
{
}

ModelManager::~ModelManager()
{
}

void ModelManager::loadFile(std::shared_ptr<ModelData> a_pInst, wxString a_Path)
{
	a_pInst->init(a_Path);
}
#pragma endregion

}