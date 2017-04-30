// ExternModel.cpp
//
// 2014/06/06 Ian Wu/Real0000
//

#include "CommonUtil.h"

#include "Module/Model/DrawBuffer.h"
#include "Module/Model/ExternModel.h"

namespace R
{

static unsigned int s_UtilSerial = 0; 

template<typename SrcType, typename TVec>
static void setupVertexData(FbxMesh *a_pSrcMesh, SrcType *a_pSrcData, model_file::model_meshes *a_pTargetMesh, std::function<void(model_file::model_vertex&, TVec)>a_Lambda)
{
    if( nullptr == a_pSrcData ) return;

    switch( a_pSrcData->GetMappingMode() )
    {
        case FbxLayerElement::eByControlPoint:{
            auto &l_DirectArray = a_pSrcData->GetDirectArray();
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    for( unsigned int i=0 ; i<a_pSrcMesh->GetControlPointsCount() ; ++i ) a_Lambda(a_pTargetMesh->m_Vertex[i], l_DirectArray[i]);
                    }break;

                case FbxLayerElement::eIndexToDirect:{
                    auto &l_IndexArray = a_pSrcData->GetIndexArray();
                    for( unsigned int i=0 ; i<l_IndexArray.GetCount() ; ++i ) a_Lambda(a_pTargetMesh->m_Vertex[i], l_DirectArray[l_IndexArray[i]]);
                    }break;

                default:break;
            }
            }break;

        case FbxLayerElement::eByPolygonVertex:{
            auto &l_DirectArray = a_pSrcData->GetDirectArray();
            switch( a_pSrcData->GetReferenceMode() )
            {
                case FbxLayerElement::eDirect:{
                    for( unsigned int i=0 ; i<a_pSrcMesh->GetPolygonCount() ; ++i )
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
                    for( unsigned int i=0 ; i<a_pSrcMesh->GetPolygonCount() ; ++i )
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

static std::string getFbxMaterialTexture(FbxSurfaceMaterial *a_pSrcMaterial, const char *a_pTypeName)
{
    FbxProperty l_DiffTexture = a_pSrcMaterial->FindProperty(a_pTypeName);
    if( l_DiffTexture.IsValid() )
    {
        FbxLayeredTexture *l_pLayeredTex = 0 != l_DiffTexture.GetSrcObjectCount<FbxLayeredTexture>() ? l_DiffTexture.GetSrcObject<FbxLayeredTexture>(0) : nullptr;
        if( nullptr != l_pLayeredTex )
        {
            FbxFileTexture *l_pTexFile = 0 != l_pLayeredTex->GetSrcObjectCount<FbxFileTexture>() ? l_pLayeredTex->GetSrcObject<FbxFileTexture>(0) : nullptr;
            return nullptr == l_pTexFile ? "" : l_pTexFile->GetRelativeFileName();
        }
    }
    
    FbxFileTexture *l_pTexFile = 0 != l_DiffTexture.GetSrcObjectCount<FbxFileTexture>() ? l_DiffTexture.GetSrcObject<FbxFileTexture>(0) : nullptr;
    return nullptr == l_pTexFile ? "" : l_pTexFile->GetRelativeFileName();
}

//
// ExternModelMesh
//
ExternModelMesh::ExternModelMesh()
	: m_NormalTex(wxT("")), m_DiffuseTex(wxT(""))
{
}

ExternModelMesh::~ExternModelMesh()
{
}
	
void ExternModelMesh::parseMeshData(const FbxScene *a_pSceneData, const FbxMesh *a_pMeshData, std::vector<VertexPosTex2NormalBone> &a_VtxOutput, std::vector<unsigned int> &a_IdxOutput)
{
    m_Name = a_pMeshData->GetName();

	for( unsigned int i=0 ; i<a_pMeshData->GetNodeCount() ; ++i )
	{
		m_Meshes[l_Idx]->m_RefNode.push_back(l_NodeIdxMap[l_pRefNode]);
	}
    
    

    FbxVector4 l_FbxBoxMax, l_FbxBoxMin, l_FbxBoxCenter;
    l_pRefNode->EvaluateGlobalBoundingBoxMinMaxCenter(l_FbxBoxMin, l_FbxBoxMax, l_FbxBoxCenter);
    l_FbxBoxMax -= l_FbxBoxMin;
    m_Meshes[l_Idx]->m_BoxSize = glm::vec3(l_FbxBoxMax[0], l_FbxBoxMax[1], l_FbxBoxMax[2]) * 0.5f;

    unsigned int l_NumVtx = l_pSrcMesh->GetControlPointsCount();
    m_Meshes[l_Idx]->m_Vertex.resize(l_NumVtx);
    FbxVector4 l_Trans = l_pRefNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    FbxVector4 l_Scale = l_pRefNode->GetGeometricScaling(FbxNode::eSourcePivot);
    FbxVector4 l_Rot = l_pRefNode->GetGeometricRotation(FbxNode::eSourcePivot);
    FbxAMatrix l_VtxTrans;
    l_VtxTrans.SetTRS(l_Trans, l_Rot, l_Scale);
    glm::mat4x4 l_MultiMat;
    for( unsigned int i=0 ; i<4 ; ++i )
    {
        FbxVector4 l_SrcVec = l_VtxTrans.GetColumn(i);
        l_MultiMat[i] = glm::vec4(l_SrcVec[0], l_SrcVec[1], l_SrcVec[2], l_SrcVec[3]);
    }
    for( unsigned int i=0 ; i<l_NumVtx ; ++i )
    {
        FbxVector4 l_SrcVtx = l_pSrcMesh->GetControlPoints()[i];
        glm::vec4 l_Temp = glm::vec4(l_SrcVtx[0], l_SrcVtx[1], l_SrcVtx[2], 1.0f) * l_MultiMat;
        model_vertex &l_TargetVtx = m_Meshes[l_Idx]->m_Vertex[i];
        l_TargetVtx.m_Position = glm::vec3(l_Temp[0], l_Temp[1], l_Temp[2]);
    }
        
    std::function<void(model_vertex&, FbxVector4)> l_SetNormalFunc = [](model_vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Normal = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
    setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetNormals(), m_Meshes[l_Idx], l_SetNormalFunc);

    std::function<void(model_vertex&, FbxVector4)> l_SetTangentFunc = [](model_vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Tangent = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
    setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetTangents(), m_Meshes[l_Idx], l_SetTangentFunc);

    std::function<void(model_vertex&, FbxVector4)> l_SetBinormalFunc = [](model_vertex &a_Vtx, FbxVector4 a_Src){ a_Vtx.m_Binormal = glm::vec3(a_Src[0], a_Src[1], a_Src[2]); };
    setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetBinormals(), m_Meshes[l_Idx], l_SetBinormalFunc);

    std::function<void(model_vertex&, FbxVector2)> l_SetUVFunc = [](model_vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord.x = a_Src[0]; a_Vtx.m_Texcoord.y = a_Src[1]; };
    setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(0)->GetUVs(), m_Meshes[l_Idx], l_SetUVFunc);

    if( l_pSrcMesh->GetLayerCount() > 1 )
    {
        std::function<void(model_vertex&, FbxVector2)> l_SetUV2Func = [](model_vertex &a_Vtx, FbxVector2 a_Src){ a_Vtx.m_Texcoord.z = a_Src[0]; a_Vtx.m_Texcoord.w = a_Src[1]; };
        setupVertexData(l_pSrcMesh, l_pSrcMesh->GetLayer(1)->GetUVs(), m_Meshes[l_Idx], l_SetUV2Func);
    }

    for( unsigned int i=0 ; i<l_pSrcMesh->GetPolygonCount() ; ++i )
    {
        for( unsigned int j=0 ; j<3 ; ++j )
        {
            unsigned int l_PtIdx = l_pSrcMesh->GetPolygonVertex(i, j);
            m_Meshes[l_Idx]->m_Indicies.push_back(l_PtIdx);        
        }
    }

    if( 0 != l_pRefNode->GetMaterialCount() )
    {
        FbxSurfaceMaterial *l_pSrcMaterial = l_pRefNode->GetMaterial(0);
        m_Meshes[l_Idx]->m_Texures[TEXUSAGE_DIFFUSE] = getFbxMaterialTexture(l_pSrcMaterial, FbxSurfaceMaterial::sDiffuse);
            
        std::string l_TexName(getFbxMaterialTexture(l_pSrcMaterial, FbxSurfaceMaterial::sNormalMap));
        if( 0 != l_TexName.length() ) m_Meshes[l_Idx]->m_Texures[TEXUSAGE_NORMAL] = l_TexName;

        l_TexName = getFbxMaterialTexture(l_pSrcMaterial, FbxSurfaceMaterial::sSpecular);
        if( 0 != l_TexName.length() ) m_Meshes[l_Idx]->m_Texures[TEXUSAGE_SPECULAR] = l_TexName;
    }
}

void ExternModelMesh::parseBoneData(std::map<wxString, unsigned int> &l_NameIdxMap, const aiMesh *l_pMeshData, std::vector<VertexPosTex2NormalBone> &l_VtxOutput, unsigned int l_VtxBase)
{
	if( l_pMeshData->HasBones() )
	{
		for( unsigned int i=0 ; i<l_pMeshData->mNumBones ; i++ )
		{
			const aiBone *l_pBoneData = l_pMeshData->mBones[i];
			unsigned int l_BondeID = l_NameIdxMap[l_pBoneData->mName.C_Str()];
			m_BoneID.push_back(l_BondeID);
			for( unsigned int j=0 ; j<l_pBoneData->mNumWeights ; j++ )
			{
				const aiVertexWeight &l_Weight = l_pBoneData->mWeights[j];
				for( unsigned int k=0 ; k<4 ; k++ )
				{
					if( -1 == l_VtxOutput[l_Weight.mVertexId + l_VtxBase].m_BoneID[k] )
					{
						l_VtxOutput[l_Weight.mVertexId].m_BoneID[k] = m_BoneID.size() - 1;
						l_VtxOutput[l_Weight.mVertexId].m_BoneWeight[k] = l_Weight.mWeight;
						break;
					}
				}
			}
		}
	}

	for( unsigned int i=0 ; i<l_VtxOutput.size() ; i++ )
	{
		for( unsigned int j=0 ; j<4 ; j++ )
		{
			if( -1 == l_VtxOutput[i].m_BoneID[j] ) l_VtxOutput[i].m_BoneID[j] = 0;
		}
	}
}

//
// ExternModelAnime
//
ExternModelAnime::ExternModelAnime()
	: m_Name(wxT(""))
	, m_Length(0.0f)
{
}

ExternModelAnime::~ExternModelAnime()
{
	for( unsigned int i=0 ; i<m_Nodes.size() ; i++ )
		delete m_Nodes[i];
	m_Nodes.clear();
}
	
void ExternModelAnime::parseAnimeData(std::map<wxString, unsigned int> &l_NameIdxMap, const aiAnimation *l_pAnimData, std::vector<ExternModelNode *> &l_Nodes)
{
	m_Name = l_pAnimData->mName.C_Str();

	unsigned int l_FPS = l_pAnimData->mTicksPerSecond;
	if( 0 == l_FPS ) l_FPS = 60;
	m_Length = l_pAnimData->mDuration / float(l_FPS);
	while( m_Nodes.size() < l_pAnimData->mNumChannels )
	{
		AnimeNodes *l_pNewNode = new AnimeNodes();
		aiNodeAnim *l_pNodeAnimData = l_pAnimData->mChannels[m_Nodes.size()];
		m_Nodes.push_back(l_pNewNode);

		l_pNewNode->m_NodeID = l_NameIdxMap[l_pNodeAnimData->mNodeName.C_Str()];
		for( unsigned int i=0 ; i<l_pNodeAnimData->mNumPositionKeys ; i++ )
		{
			aiVectorKey l_Key = l_pNodeAnimData->mPositionKeys[i];
			l_pNewNode->m_PosKey.push_back(std::make_pair((unsigned int)(l_Key.mTime * l_FPS), glm::vec3(l_Key.mValue.x, l_Key.mValue.y, l_Key.mValue.z)));
		}
		for( unsigned int i=0 ; i<l_pNodeAnimData->mNumScalingKeys ; i++ )
		{
			aiVectorKey l_Key = l_pNodeAnimData->mScalingKeys[i];
			l_pNewNode->m_ScaleKey.push_back(std::make_pair((unsigned int)(l_Key.mTime * l_FPS), glm::vec3(l_Key.mValue.x, l_Key.mValue.y, l_Key.mValue.z)));
		}
		for( unsigned int i=0 ; i<l_pNodeAnimData->mNumRotationKeys ; i++ )
		{
			aiQuatKey l_Key = l_pNodeAnimData->mRotationKeys[i];
			l_pNewNode->m_RotKey.push_back(std::make_pair((unsigned int)(l_Key.mTime * l_FPS), glm::quat(l_Key.mValue.w, l_Key.mValue.x, l_Key.mValue.y, l_Key.mValue.z)));
		}

		glm::vec3 l_Pos, l_Scale;
		glm::quat l_Rot;
		decomposeTRS(l_Nodes[l_pNewNode->m_NodeID]->m_Translate, l_Pos, l_Scale, l_Rot);
		initAnimeNode(l_pNewNode->m_PosKey, l_pNewNode->m_PosKeyMap, l_Pos, l_pNodeAnimData);
		initAnimeNode(l_pNewNode->m_ScaleKey, l_pNewNode->m_ScaleKeyMap, l_Scale, l_pNodeAnimData);
		initAnimeNode(l_pNewNode->m_RotKey, l_pNewNode->m_RotKeyMap, l_Rot, l_pNodeAnimData);
	}
}

template<typename l_T>
void ExternModelAnime::initAnimeNode(std::vector< std::pair<unsigned int, l_T> > &l_Keys, std::vector<unsigned int> &l_Maps, l_T &l_Default, const aiNodeAnim *l_pNodeAnimData)
{
	unsigned int l_LastKey = ceil(m_Length * 60.0f);
	if( l_Keys.empty() )
	{
		l_Keys.push_back(std::make_pair(0, l_Default));
		l_Keys.push_back(std::make_pair(l_LastKey, l_Default));
	}
	else if( 1 == l_Keys.size() )
	{
		l_Keys[0].first = 0;
		l_Keys.push_back(std::make_pair(l_LastKey, l_Keys[0].second));
	}
	else
	{
		if( 0 != l_Keys[0].first )
		{
			switch( l_pNodeAnimData->mPreState )
			{
				default:
				case aiAnimBehaviour_DEFAULT:{
					l_Keys.insert(l_Keys.begin(), std::make_pair(0, l_Default));
					}break;

				case aiAnimBehaviour_CONSTANT:{
					l_Keys.insert(l_Keys.begin(), std::make_pair(0, l_Keys[0].second));
					}break;

				case aiAnimBehaviour_LINEAR:{
					l_T l_NewKey;
					unsigned int l_Time0 = l_Keys[0].first;
					l_T l_Key0 = l_Keys[0].second;
					unsigned int l_Time1 = l_Keys[1].first;
					l_T l_Key1 = l_Keys[1].second;
					for( unsigned int i=0 ; i<((unsigned int)l_NewKey.length()) ; i++ )
					{
						l_NewKey[i] = (l_Key1[i] - l_Key0[i]) / float(l_Time1 - l_Time0) * l_Time0;
						l_NewKey[i] = l_Key0[i] - l_NewKey[i];
					}
					l_Keys.insert(l_Keys.begin(), std::make_pair(0, l_NewKey));
					}break;

				case aiAnimBehaviour_REPEAT:{
					l_T l_NewKey;
					unsigned int l_Time0 = l_Keys.back().first;
					l_T l_Key0 = l_Keys.back().second;
					unsigned int l_Time1 = l_Keys[0].first;
					l_T l_Key1 = l_Keys[0].second;
					unsigned int l_ToEnd = l_LastKey - l_Time0;
					for( unsigned int i=0 ; i<((unsigned int)l_NewKey.length()) ; i++ )
					{
						l_NewKey[i] = (l_Key1[i] - l_Key0[i]) / float(l_ToEnd + l_Time1) * l_ToEnd;
						l_NewKey[i] = l_Key0[i] + l_NewKey[i];
					}
					l_Keys.insert(l_Keys.begin(), std::make_pair(0, l_NewKey));
					}break;
			}
		}

		if( l_LastKey != l_Keys.back().first )
		{
			switch( l_pNodeAnimData->mPreState )
			{
				default:
				case aiAnimBehaviour_DEFAULT:{
					l_Keys.push_back(std::make_pair(l_LastKey, l_Default));
					}break;

				case aiAnimBehaviour_CONSTANT:{
					l_Keys.push_back(std::make_pair(l_LastKey, l_Keys.back().second));
					}break;

				case aiAnimBehaviour_LINEAR:{
					l_T l_NewKey;
					unsigned int l_Time0 = l_Keys[l_Keys.size() - 2].first;
					l_T l_Key0 = l_Keys[l_Keys.size() - 2].second;
					unsigned int l_Time1 = l_Keys[l_Keys.size() - 1].first;
					l_T l_Key1 = l_Keys[l_Keys.size() - 1].second;
					for( unsigned int i=0 ; i<((unsigned int)l_NewKey.length()) ; i++ )
					{
						l_NewKey[i] = (l_Key1[i] - l_Key0[i]) / float(l_Time1 - l_Time0) * (l_LastKey - l_Time1);
						l_NewKey[i] = l_Key1[i] + l_NewKey[i];
					}
					l_Keys.push_back(std::make_pair(l_LastKey, l_NewKey));
					}break;

				case aiAnimBehaviour_REPEAT:{
					l_Keys.push_back(std::make_pair(l_LastKey, l_Keys.back().second));
					}break;
			}
		}
	}

	int l_CurrIdx = 0;
	for( unsigned int i=0 ; i<l_LastKey ; i++ )
	{
		if( i == l_Keys[l_CurrIdx].first )
			l_CurrIdx = std::min<unsigned int>(l_CurrIdx + 1, l_Keys.size() - 1);
		l_Maps.push_back(l_CurrIdx);
	}
}

//
// ExternModelNode
//
ExternModelNode::ExternModelNode()
	: m_Name(wxT(""))
	, m_Translate(1.0f)
	, m_Parent(-1)
{
}

ExternModelNode::~ExternModelNode()
{
}

#pragma region ExternModelData
//
// ExternModelData
//
ExternModelData::ExternModelData()
	: m_pVertex(nullptr)
	, m_pIndex(nullptr)
	, m_bValid(false)
{
}

ExternModelData::~ExternModelData()
{
	SAFE_DELETE(m_pIndex);
	SAFE_DELETE(m_pVertex);

	for( unsigned int i=0 ; i<m_Meshes.size() ; i++ ) delete m_Meshes[i];
	m_Meshes.clear();

	for( unsigned int i=0 ; i<m_Nodes.size() ; i++ ) delete m_Nodes[i];
	m_Nodes.clear();

	for( unsigned int i=0 ; i<m_Animes.size() ; i++ ) delete m_Animes[i];
	m_Animes.clear();
}

bool ExternModelData::loadMeshFile(wxString a_File, wxString &a_ErrorMsg)
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
    bool l_bSuccess = l_pImporter->Initialize(a_File.c_str(), -1, l_pIOCfg);

	if( !l_bSuccess )
	{
		a_ErrorMsg += wxString::Format(wxT("Load file error : %s"), a_File.c_str());
		return false;
	}
	
	FbxScene* l_pSceneData = FbxScene::Create(l_pSdkManager, "SourceScene");
    if( !l_pImporter->Import(l_pSceneData) )
	{
		a_ErrorMsg += wxString::Format(wxT("Scene import error : %s"), a_File.c_str());
		l_pImporter->Destroy();
		return false;	
	}
	l_pImporter->Destroy();

	std::map<FbxNode *, unsigned int> l_NodeIdxMap;
    std::map<FbxMesh *, unsigned int> l_MeshMap;
    {
        m_Nodes.resize(l_pSceneData->GetNodeCount(), nullptr);
        for( unsigned int i=0 ; i<m_Nodes.size() ; ++i )
        {
            m_Nodes[i] = new ExternModelNode();
            FbxNode *l_pCurrNode = l_pSceneData->GetNode(i);
            l_NodeIdxMap[l_pCurrNode] = i;

            FbxAMatrix l_FbxMat = l_pCurrNode->EvaluateLocalTransform();

            m_Nodes[i]->m_Name = l_pCurrNode->GetName();
            l_NodeIdxMap[m_Nodes[i]->m_Name] = i;
            for( unsigned int j=0 ; j<4 ; ++j )
            {
                FbxVector4 l_Data = l_FbxMat.GetColumn(j);
                m_Nodes[i]->m_Origin[j] = glm::vec4(l_Data[0], l_Data[1], l_Data[2], l_Data[3]);
            }

            FbxNodeAttribute *l_pNodeAttr = l_pCurrNode->GetNodeAttribute();
            if( nullptr != l_pNodeAttr )
            {
                switch( l_pNodeAttr->GetAttributeType() )
                {
                    case FbxNodeAttribute::eMesh:{
                        FbxMesh *l_pMesh = (FbxMesh *)l_pNodeAttr;

                        unsigned int l_MeshIdx = 0;
                        auto l_MeshIt = l_MeshMap.find(l_pMesh);
                        if( l_MeshMap.find(l_pMesh) == l_MeshMap.end() )
                        {
                            l_MeshIdx = l_MeshMap.size();
                            l_MeshMap.insert(std::make_pair(l_pMesh, l_MeshIdx));
                        }
                        else l_MeshIdx = l_MeshIt->second;
                        m_Nodes[i]->m_RefMesh.push_back(l_MeshIdx);
                        }break;

                    default:break;
                }
            }
        }

        for( unsigned int i=0 ; i<m_Nodes.size() ; ++i )
        {
            FbxNode *l_pCurrNode = l_pSceneData->GetNode(i);
            if( nullptr != l_pCurrNode->GetParent() ) m_Nodes[i]->m_Parent = l_NodeIdxMap[l_pCurrNode->GetParent()];
            for( unsigned int j=0 ; j<l_pCurrNode->GetChildCount() ; ++j )
            {
                unsigned int l_ChildIdx = l_NodeIdxMap[l_pCurrNode->GetChild(j)];
                m_Nodes[i]->m_Children.push_back(l_ChildIdx);
            }
        }
    }

	m_Meshes.resize(l_MeshMap.size(), nullptr);
	for( auto it = l_MeshMap.begin() ; it != l_MeshMap.end() ; ++it )
	{
		FbxMesh *l_pSrcMesh = it->first;
        unsigned int l_Idx = it->second;
        m_Meshes[l_Idx] = new ExternModelMesh();
    }

    l_pSceneData->Clear();
}

void ExternModelData::initDrawBuffer()
{
	m_pVertex = VertexBuffer::create(&(m_TempVertex.front()), m_TempVertex.size());
	m_pIndex = IndexBuffer::create(&(m_TempIndex.front()), m_TempIndex.size());
	m_TempVertex.clear();
	m_TempIndex.clear();
	
	m_bValid = true;
}
#pragma endregion

#pragma region ExternModelFileManager
//
// ExternModelFileManager
//
static ExternModelFileManager *m_pInstance = nullptr;
ExternModelFileManager& ExternModelFileManager::singleton()
{
	if( nullptr == m_pInstance ) m_pInstance = new ExternModelFileManager();
	return *m_pInstance;
}

void ExternModelFileManager::purge()
{
	SAFE_DELETE(m_pInstance);
}

ExternModelFileManager::ExternModelFileManager()
{

}

ExternModelFileManager::~ExternModelFileManager()
{
	clearCache();
}

void ExternModelFileManager::clearCache()
{
	for( auto it = m_ModelCache.begin() ; it != m_ModelCache.end() ; ++it ) delete it->second;
	m_ModelCache.clear();
}

ExternModelData* ExternModelFileManager::getModelData(wxString l_Filename, bool l_bImmediate)
{
	auto it = m_ModelCache.find(l_Filename);
	if( m_ModelCache.end() != it ) return it->second;
	
	ExternModelData *l_pNewFile = (m_ModelCache[l_Filename] = new ExternModelData());
	if( l_bImmediate )
	{
		l_pNewFile->loadMeshFile(l_Filename);
		l_pNewFile->initDrawBuffer();
	}
	
	return l_pNewFile;
}

}