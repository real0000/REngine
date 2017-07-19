// AnimationImporter.h
//
// 2017/06/20 Ian Wu/Real0000
//
#ifndef _ANIMATION_IMPORTER_H_
#define _ANIMATION_IMPORTER_H_

namespace R
{

class ModelNode;

class AnimationData
{
public:
	struct AnimNode
	{
		std::map<unsigned int, glm::vec3> m_Trans;
		std::map<unsigned int, glm::vec3> m_Scale;
		std::map<unsigned int, glm::quat> m_Rot;
	};
	struct Animation
	{
		int m_Duration;
		float m_FramePerSecond;
		std::map<wxString, AnimNode *> m_NodeList;
	};
public:
	AnimationData();
	virtual ~AnimationData();
	
	void init(wxString a_Filepath);

private:
	std::map<wxString, Animation *> m_Animations;
};

class AnimationManager : public SearchPathSystem<AnimationData>
{
public:
	static AnimationManager& singleton();

private:
	AnimationManager();
	virtual ~AnimationManager();

	void loadFile(std::shared_ptr<AnimationData> a_pInst, wxString a_Filepath);
};

}

#endif