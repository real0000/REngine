// ModelImporter.cpp
//
// 2017/06/02 Ian Wu/Real0000
//

#include "RImporters.h"
#include "fbxsdk.h"

namespace R
{

template<typename T>
int compareVec(const T &a_Left, const T &a_Right)
{
	for( int i=0 ; i<a_Left.length() ; ++i )
	{
		if( a_Left[i] < a_Right[i] ) return -1;
		else if( a_Left[i] == a_Right[i] ) continue;
		return 1;
	}
	return 0;
}

bool operator<(const ModelData::Vertex &a_Left, const ModelData::Vertex &a_Right)
{
	int l_Res = 0;
#define KEY_COMPARE(param) \
	l_Res = compareVec(a_Left.##param, a_Right.##param);\
	if( l_Res < 0 ) return true;			\
	else if( l_Res > 0 ) return false;

	KEY_COMPARE(m_Position)
	for( unsigned int i=0 ; i<4 ; ++i )
	{
		KEY_COMPARE(m_Texcoord[i])
	}
	KEY_COMPARE(m_Normal)
	KEY_COMPARE(m_Tangent)
	KEY_COMPARE(m_Binormal)
#undef KEY_COMPARE
	return false;
}

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
static void setupVertexData(fbxsdk::FbxMesh *a_pSrcMesh, SrcType *a_pSrcData, unsigned int a_CtrlIdx, unsigned int a_VtxCounter, ModelData::Vertex &a_Target, std::function<void(ModelData::Vertex&, TVec)> a_Lambda)
{
    if( nullptr == a_pSrcData ) return;

    auto &l_DirectArray = a_pSrcData->GetDirectArray();
    auto &l_IndexArray = a_pSrcData->GetIndexArray();
    switch( a_pSrcData->GetMappingMode() )
    {
        case FbxLayerElement::eByControlPoint:{
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    a_Lambda(a_Target, l_DirectArray[a_CtrlIdx]);
                    }break;

                case FbxLayerElement::eIndexToDirect:{
                    a_Lambda(a_Target, l_DirectArray[l_IndexArray[a_CtrlIdx]]);
                    }break;

                default:break;
            }
            }break;

        case FbxLayerElement::eByPolygonVertex:{
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    a_Lambda(a_Target, l_DirectArray[a_VtxCounter]);
                    }break;

                case FbxLayerElement::eIndexToDirect:{
                    a_Lambda(a_Target, l_DirectArray[l_IndexArray[a_VtxCounter]]);
                    }break;
            }
            }break;

        default:break;
    }
}

static wxString getTextureName(fbxsdk::FbxSurfaceMaterial *a_pMat, const char *a_pName)
{
	fbxsdk::FbxProperty l_TextureProperty = a_pMat->FindPropertyHierarchical(a_pName);
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

    fbxsdk::FbxScene* l_pScene = fbxsdk::FbxScene::Create(l_pSdkManager, "SourceScene");
    l_pImporter->Import(l_pScene);
    l_pImporter->Destroy();

	{// scene converter
		FbxMaterialConverter l_MatConverter(*l_pSdkManager);
		l_MatConverter.ConnectTexturesToMaterials(*l_pScene);

		FbxGeometryConverter l_GeoConverter(l_pSdkManager);
		l_GeoConverter.Triangulate(l_pScene, true);
		l_GeoConverter.SplitMeshesPerMaterial(l_pScene, true);
	}

    std::map<fbxsdk::FbxNode *, ModelNode *> l_NodeMap;
    std::map<fbxsdk::FbxMesh *, unsigned int> l_MeshMap;
	std::vector<ModelNode *> l_NodeVec(l_pScene->GetNodeCount(), nullptr);
    {
        for( unsigned int i=0 ; i<l_NodeVec.size() ; ++i )
        {
            l_NodeVec[i] = new ModelNode();
            fbxsdk::FbxNode *l_pCurrNode = l_pScene->GetNode(i);
            l_NodeMap[l_pCurrNode] = l_NodeVec[i];

            //FbxAMatrix &l_FbxMat = l_pCurrNode->EvaluateLocalTransform(FBXSDK_TIME_INFINITE, FbxNode::eSourcePivot, false, true);
			FbxVector4 l_Trans = l_pCurrNode->LclTranslation.Get();
			FbxVector4 l_Scale = l_pCurrNode->LclScaling.Get();
			FbxVector4 l_Rot = l_pCurrNode->LclRotation.Get();
			FbxAMatrix l_FbxMat;
			l_FbxMat.SetTRS(l_Trans, l_Rot, l_Scale);

            l_NodeVec[i]->m_NodeName = l_pCurrNode->GetName();
			FBXMAT_TO_GLMMAT(l_FbxMat, l_NodeVec[i]->m_Transform)

			for( int j=0 ; j<l_pCurrNode->GetNodeAttributeCount() ; ++j )
			{
				fbxsdk::FbxNodeAttribute *l_pNodeAttr = l_pCurrNode->GetNodeAttributeByIndex(j);
				if( nullptr != l_pNodeAttr )
				{
					switch( l_pNodeAttr->GetAttributeType() )
					{
						case fbxsdk::FbxNodeAttribute::eMesh:{
							fbxsdk::FbxMesh *l_pMesh = static_cast<fbxsdk::FbxMesh *>(l_pNodeAttr);

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
        }

		std::vector<ModelNode *> l_TempRootNodes;
        for( unsigned int i=0 ; i<l_NodeVec.size() ; ++i )
        {
            fbxsdk::FbxNode *l_pCurrNode = l_pScene->GetNode(i);
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
	
	const char *c_TextureSlots[TEXUSAGE_TYPECOUNT] = {
		fbxsdk::FbxSurfaceMaterial::sDiffuse,
		fbxsdk::FbxSurfaceMaterial::sReflection,
		fbxsdk::FbxSurfaceMaterial::sSpecular,
		fbxsdk::FbxSurfaceMaterial::sNormalMap,
		fbxsdk::FbxSurfaceMaterial::sDisplacementColor};
	m_Materials.insert(std::make_pair(DEFAULT_EMPTY_MAT_NAME, Material()));
	for( int i=0 ; i<l_pScene->GetMaterialCount() ; ++i )
	{
		fbxsdk::FbxSurfaceMaterial *l_pMat = l_pScene->GetMaterial(i);
		wxString l_MatName(l_pMat->GetName());
		if( m_Materials.end() == m_Materials.find(l_MatName) )
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

			if( !l_bEmpty ) m_Materials.insert(std::make_pair(l_MatName, l_NewMat));
		}
	}

    m_Meshes.resize(l_MeshMap.size(), nullptr);
	std::vector<int> l_BoneRecord;
	std::function<glm::vec3(float, float, float)> l_PosAssignFunc = ModelManager::singleton().getFlipYZ() ?
		(std::function<glm::vec3(float, float, float)>)[](float a_X, float a_Y, float a_Z) -> glm::vec3{ return glm::vec3(a_X, a_Z, a_Y); } :
		(std::function<glm::vec3(float, float, float)>)[](float a_X, float a_Y, float a_Z) -> glm::vec3{ return glm::vec3(a_X, a_Y, a_Z); };
	std::map<Vertex, unsigned int> l_VtxSet;
	std::map<int, std::vector<unsigned int>> l_CtrlMap;
	unsigned int l_VtxCounter = 0;
    for( auto it = l_MeshMap.begin() ; it != l_MeshMap.end() ; ++it )
    {
        fbxsdk::FbxMesh *l_pSrcMesh = it->first;
        unsigned int l_Idx = it->second;
        Meshes *l_pDstMesh = m_Meshes[l_Idx] = new Meshes();
        l_pDstMesh->m_Name = it->first->GetName();
        l_pDstMesh->m_Index = l_Idx;

		fbxsdk::FbxNode *l_pRefNode = nullptr;
		glm::vec3 l_BoxCenter(0.0f, 0.0f, 0.0f);
		for( int i=0 ; i<it->first->GetNodeCount() ; ++i )
		{
			l_pRefNode = it->first->GetNode(i);
			l_pDstMesh->m_RefNode.push_back(l_NodeMap[l_pRefNode]);
		}

		l_pRefNode = it->first->GetNode();
		if( l_pDstMesh->m_Name.empty() ) l_pDstMesh->m_Name = l_pRefNode->GetName();

		l_VtxSet.clear();
		l_CtrlMap.clear();
		l_BoneRecord.clear();
		l_BoneRecord.reserve(l_pSrcMesh->GetControlPointsCount());
        unsigned int l_NumTriangle = l_pSrcMesh->GetPolygonCount();
		auto l_pSrcVtxArray = l_pSrcMesh->GetControlPoints();
		l_VtxCounter = 0;

		fbxsdk::FbxAMatrix l_GeoTrans, l_GeoNormalTrans;
		{
			l_GeoTrans.SetIdentity();
			l_GeoNormalTrans.SetIdentity();
			if( l_pRefNode->GetNodeAttribute() )
			{
				l_GeoTrans.SetTRS(l_pRefNode->GetGeometricTranslation(fbxsdk::FbxNode::eSourcePivot)
								, l_pRefNode->GetGeometricRotation(fbxsdk::FbxNode::eSourcePivot)
								, l_pRefNode->GetGeometricScaling(fbxsdk::FbxNode::eSourcePivot));
			}
			FbxAMatrix l_FbxMat;
			l_FbxMat.SetTRS(l_pRefNode->LclTranslation.Get(), l_pRefNode->LclRotation.Get(), l_pRefNode->LclScaling.Get());
			l_GeoTrans = l_FbxMat.Inverse() * l_pRefNode->EvaluateLocalTransform() * l_GeoTrans;

			l_GeoNormalTrans = l_GeoTrans;
			l_GeoNormalTrans.SetT(fbxsdk::FbxVector4(0.0f, 0.0f, 0.0f, 0.0f));
		}

		glm::vec3 l_BoxMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), l_BoxMin(FLT_MAX, FLT_MAX, FLT_MAX);
		const int c_ReverseFaceIdxAdjust[] = {2, 0, -2};
		for( unsigned int i=0 ; i<l_NumTriangle ; ++i )
        {	
			for( unsigned int j=0 ; j<3 ; ++j )
			{
				int l_TrianglePos = ModelManager::singleton().getReverseFace() ? (2 - j) : j;
				unsigned int l_AdjustVtxCounter = ModelManager::singleton().getReverseFace() ? (l_VtxCounter + c_ReverseFaceIdxAdjust[j]) : l_VtxCounter;
				int l_CtrlPtIdx = l_pSrcMesh->GetPolygonVertex(i, l_TrianglePos);

				Vertex l_ThisVtx = {};
				
				FbxVector4 l_SrcVtx = l_GeoTrans.MultT(l_pSrcMesh->GetControlPoints()[l_CtrlPtIdx]);
				l_ThisVtx.m_Position = l_PosAssignFunc(l_SrcVtx[0], l_SrcVtx[1], l_SrcVtx[2]);
				l_BoxMax = glm::max(l_ThisVtx.m_Position, l_BoxMax);
				l_BoxMin = glm::min(l_ThisVtx.m_Position, l_BoxMin);

				std::function<void(Vertex&, FbxVector4)> l_SetNormalFunc = [=](Vertex &a_Vtx, FbxVector4 a_Src)
				{
					a_Src = l_GeoNormalTrans.MultT(a_Src);
					a_Vtx.m_Normal = l_PosAssignFunc(a_Src[0], a_Src[1], a_Src[2]);
				};
				setupVertexData(l_pSrcMesh, l_pSrcMesh->GetElementNormal(), l_CtrlPtIdx, l_AdjustVtxCounter, l_ThisVtx, l_SetNormalFunc);
				
				std::function<void(Vertex&, FbxVector4)> l_SetTangentFunc = [=](Vertex &a_Vtx, FbxVector4 a_Src)
				{
					a_Src = l_GeoNormalTrans.MultT(a_Src);
					a_Vtx.m_Tangent = l_PosAssignFunc(a_Src[0], a_Src[1], a_Src[2]);
				};
				setupVertexData(l_pSrcMesh, l_pSrcMesh->GetElementTangent(), l_CtrlPtIdx, l_AdjustVtxCounter, l_ThisVtx, l_SetTangentFunc);
				
				std::function<void(Vertex&, FbxVector4)> l_SetBinormalFunc = [=](Vertex &a_Vtx, FbxVector4 a_Src)
				{
					a_Src = l_GeoNormalTrans.MultT(a_Src);
					a_Vtx.m_Binormal = l_PosAssignFunc(a_Src[0], a_Src[1], a_Src[2]);
				};
				setupVertexData(l_pSrcMesh, l_pSrcMesh->GetElementBinormal(), l_CtrlPtIdx, l_AdjustVtxCounter, l_ThisVtx, l_SetBinormalFunc);

				int l_LayerCount = std::min(4, l_pSrcMesh->GetLayerCount());
				for( int k=0 ; k<l_LayerCount ; ++k )
				{
					std::function<void(Vertex&, FbxVector2)> l_SetUVFunc = [&k](Vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord[k/2].x = a_Src[0]; a_Vtx.m_Texcoord[k/2].y = a_Src[1]; };
					std::function<void(Vertex&, FbxVector2)> l_SetUV2Func = [&k](Vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord[k/2].z = a_Src[0]; a_Vtx.m_Texcoord[k/2].w = a_Src[1]; };

					setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(k)->GetUVs(), l_CtrlPtIdx, l_AdjustVtxCounter, l_ThisVtx, k % 2 == 0 ? l_SetUVFunc : l_SetUV2Func);
				}

				unsigned int l_VtxIdx = 0;
				{
					auto l_VtxIt = l_VtxSet.find(l_ThisVtx);
					if( l_VtxSet.end() == l_VtxIt )
					{
						l_VtxIdx = l_VtxSet.size();
						l_VtxSet.insert(std::make_pair(l_ThisVtx, l_VtxIdx));
					}
					else l_VtxIdx = l_VtxIt->second;
				}

				{// setup ctrl : vtx map
					auto l_CtrlIt = l_CtrlMap.find(l_CtrlPtIdx);
					if( l_CtrlMap.end() == l_CtrlIt )
					{
						std::vector<unsigned int> l_NewVec(1, l_VtxIdx);
						l_CtrlMap.insert(std::make_pair(l_CtrlPtIdx, l_NewVec));
					}
					else l_CtrlIt->second.push_back(l_VtxIdx);
				}
				l_pDstMesh->m_Indicies.push_back(l_VtxIdx);
				
				++l_VtxCounter;
			}
		}
        
		l_pDstMesh->m_BoundingBox.m_Size = l_BoxMax - l_BoxMin;
		l_pDstMesh->m_BoundingBox.m_Center = (l_BoxMax + l_BoxMin) * 0.5f;

		l_pDstMesh->m_Vertex.resize(l_VtxSet.size());
		for( auto l_VtxIt=l_VtxSet.begin() ; l_VtxIt!=l_VtxSet.end() ; ++l_VtxIt )
		{
			memcpy(&(l_pDstMesh->m_Vertex[l_VtxIt->second]), &(l_VtxIt->first), sizeof(Vertex));
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
					int l_CtrlIdx = l_pCluster->GetControlPointIndices()[k];
					std::vector<unsigned int> &l_VtxIndicies = l_CtrlMap[l_CtrlIdx];
					int &l_WeightIdx = l_BoneRecord[l_CtrlIdx];
					assert(l_WeightIdx < 4);
					float l_Weight = l_pCluster->GetControlPointWeights()[k];
					++l_WeightIdx;

					for( unsigned int l=0 ; l<l_VtxIndicies.size() ; ++l )
					{
						Vertex &l_Vtx = l_pDstMesh->m_Vertex[l_VtxIndicies[l]];
						l_Vtx.m_BoneId[l_WeightIdx] = l_BondIndex;
						l_Vtx.m_Weight[l_WeightIdx] = l_Weight;
					}
				}
			}
		}

		l_pDstMesh->m_RefMaterial = DEFAULT_EMPTY_MAT_NAME;
		FbxLayerElementArrayTemplate<int> *l_pMaterialIndicies = nullptr;
		if( l_pSrcMesh->GetMaterialIndices(&l_pMaterialIndicies) )
		{
			int l_Count = l_pMaterialIndicies->GetCount();
			for( int i=0 ; i<l_Count ; ++i )
			{
				int l_MatIdx = l_pMaterialIndicies->GetAt(i);
				fbxsdk::FbxSurfaceMaterial *l_pMat = l_pRefNode->GetMaterial(l_MatIdx);
				wxString l_MatName(l_pMat->GetName());
				if( m_Materials.end() != m_Materials.find(l_MatName) )
				{
					l_pDstMesh->m_RefMaterial = l_MatName;
					break;
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
	, m_bFlipYZ(false)
	, m_bReverseFace(false)
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