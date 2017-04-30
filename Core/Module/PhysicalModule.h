// PhysicalModule.h
//
// 2014/06/16 Ian Wu/Real0000
//

#ifndef _PHYSICAL_MODULE_H_
#define _PHYSICAL_MODULE_H_

namespace R
{

class PhysicalModule
{
public:
	static PhysicalModule& singleton(){ static PhysicalModule l_Inst; return l_Inst; }

	bool init();

private:
	PhysicalModule();
	virtual ~PhysicalModule();
};

}

#endif