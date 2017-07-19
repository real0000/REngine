// AnimationImporter.cpp
//
// 2017/06/20 Ian Wu/Real0000
//
#include "RImporters.h"
#include "fbxsdk.h"

namespace R
{

#pragma region AnimationData
//
// AnimationData
//
AnimationData::AnimationData()
{
}

AnimationData::~AnimationData()
{
}

void AnimationData::init(wxString a_Filepath)
{
	FbxManager *l_pSdkManager = FbxManager::Create();
	FbxIOSettings *l_pIOCfg = FbxIOSettings::Create(l_pSdkManager, IOSROOT);
    l_pIOCfg->SetBoolProp(IMP_FBX_MATERIAL, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_TEXTURE, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_LINK, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_SHAPE, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_GOBO, false);
    l_pIOCfg->SetBoolProp(IMP_FBX_ANIMATION, true);
    l_pIOCfg->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    l_pSdkManager->SetIOSettings(l_pIOCfg);

    FbxImporter* l_pImporter = FbxImporter::Create(l_pSdkManager, "");
    bool l_bSuccess = l_pImporter->Initialize(a_Filepath.c_str(), -1, l_pIOCfg);
    assert(l_bSuccess);

    FbxScene* l_pScene = FbxScene::Create(l_pSdkManager, "SourceScene");
    l_pImporter->Import(l_pScene);
    l_pImporter->Destroy();

	std::vector<FbxNode *> l_NodeList(1, l_pScene->GetRootNode());
	for( unsigned int i=0 ; i<l_NodeList.size() ; ++i )
	{
		FbxNode *l_pCurrNode = l_NodeList[i];
		for( int j=0 ; j<l_pCurrNode->GetChildCount() ; ++j ) l_NodeList.push_back(l_pCurrNode->GetChild(j));
	}

	int l_NumAnimation = l_pScene->GetSrcObjectCount<FbxAnimStack>();
	for( int i=0 ; i<l_NumAnimation ; ++i )
	{
		FbxAnimStack *l_pSrcAnimStack = l_pScene->GetSrcObject<FbxAnimStack>(i);
		if( 0 == l_pSrcAnimStack->GetMemberCount<FbxAnimLayer>() ) continue;
		
		Animation *l_pDstAnim = m_Animations[l_pSrcAnimStack->GetName()] = new Animation();
		FbxTime l_TimeInfo = l_pSrcAnimStack->GetLocalTimeSpan().GetDuration();
		l_pDstAnim->m_Duration = l_TimeInfo.GetFrameCount();
		l_pDstAnim->m_FramePerSecond = l_pDstAnim->m_Duration / l_TimeInfo.GetSecondDouble();
		
		// no blending here
		FbxAnimLayer *l_pSrcAnimLayer = l_pSrcAnimStack->GetMember<FbxAnimLayer>();

#define ASSIGN_POS(idx) l_pNodeAnimData->m_Trans[l_TimeInfo.Get()][idx] = l_Val;
#define ASSIGN_ROTATE(idx) l_pNodeAnimData->m_Rot[l_TimeInfo.Get()][idx] = glm::radians(l_Val) * 0.5f;
#define ASSIGN_SCALE(idx) l_pNodeAnimData->m_Scale[l_TimeInfo.Get()][idx] = l_Val;
#define ASSIGN_COMPONENT(component, member, func) \
		l_pAnimCurve = l_pCurrNode->##member##.GetCurve(l_pSrcAnimLayer, component); \
		if( nullptr != l_pAnimCurve ) { \
				if( nullptr == l_pNodeAnimData ) l_pNodeAnimData = l_pDstAnim->m_NodeList[l_pCurrNode->GetName()] = new AnimNode(); \
				for( int k=0 ; k<l_pAnimCurve->KeyGetCount() ; ++k ) { \
					float l_Val = l_pAnimCurve->KeyGetValue(k); \
					l_TimeInfo = l_pAnimCurve->KeyGetTime(k); \
					func } }
		
		for( unsigned int j=0 ; j<l_NodeList.size() ; ++j )
		{
			AnimNode *l_pNodeAnimData = nullptr;
			FbxNode *l_pCurrNode = l_NodeList[j];
			FbxAnimCurve *l_pAnimCurve = nullptr;
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_X, LclTranslation, ASSIGN_POS(0))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Y, LclTranslation, ASSIGN_POS(1))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Z, LclTranslation, ASSIGN_POS(2))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_X, LclRotation, ASSIGN_ROTATE(0))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Y, LclRotation, ASSIGN_ROTATE(1))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Z, LclRotation, ASSIGN_ROTATE(2))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_X, LclScaling, ASSIGN_SCALE(0))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Y, LclScaling, ASSIGN_SCALE(1))
			ASSIGN_COMPONENT(FBXSDK_CURVENODE_COMPONENT_Z, LclScaling, ASSIGN_SCALE(2))

			if( nullptr != l_pNodeAnimData )
			{
				for( auto it = l_pNodeAnimData->m_Rot.begin() ; it != l_pNodeAnimData->m_Rot.end() ; ++it )
				{
					float t0 = std::cos(it->second.z);
					float t1 = std::sin(it->second.z);
					float t2 = std::cos(it->second.x);
					float t3 = std::sin(it->second.x);
					float t4 = std::cos(it->second.y);
					float t5 = std::sin(it->second.y);
					it->second.w = t0 * t2 * t4 + t1 * t3 * t5;
					it->second.x = t0 * t3 * t4 - t1 * t2 * t5;
					it->second.y = t0 * t2 * t5 + t1 * t3 * t4;
					it->second.z = t1 * t2 * t4 - t0 * t3 * t5;
				}
			}
		}
	}

	l_pScene->Clear();
	l_pScene->Destroy();
	l_pIOCfg->Destroy();
	l_pSdkManager->Destroy();
}
#pragma endregion

#pragma region AnimationManager
//
// AnimationManager
//
AnimationManager& AnimationManager::singleton()
{
	static AnimationManager s_Inst;
	return s_Inst;
}

AnimationManager::AnimationManager()
	: SearchPathSystem(std::bind(&AnimationManager::loadFile, this, std::placeholders::_1, std::placeholders::_2), std::bind(&defaultNewFunc<AnimationData>))
{
}

AnimationManager::~AnimationManager()
{
}

void AnimationManager::loadFile(std::shared_ptr<AnimationData> a_pInst, wxString a_Filepath)
{
	a_pInst->init(a_Filepath);
}
#pragma endregion

}