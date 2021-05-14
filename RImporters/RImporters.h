// RImporters.h
//
// 2017/06/02 Ian Wu/Real0000
//
#ifndef _R_IMPORTERS_H_
#define _R_IMPORTERS_H_

#include "CommonUtil.h"
#include "Model/AnimationImporter.h"
#include "Model/ModelImporter.h"
#include "Image/ImageImporter.h"

#define FBXMAT_TO_GLMMAT(src, dst) \
	for( unsigned int r=0 ; r<4 ; ++r ){ \
		for( unsigned int c=0 ; c<4 ; ++c ) dst[r][c] = src[r][c];}

#endif