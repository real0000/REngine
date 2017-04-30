// Entity.h
//
// 2014/06/16 Ian Wu/Real0000
//

#ifndef _ENTITY_H_
#define _ENTITY_H_

namespace R
{

// program and texture as key, world matrix as extern array buffer
class Entity
{
public:
	Entity();
	virtual ~Entity();

	void draw(glm::mat4x4 &_Transition);
	void draw(glm::vec3 &_Pos, glm::vec3 &_Scale, glm::quat &_Rotate);

private:
};

}

#endif