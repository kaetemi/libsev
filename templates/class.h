
#pragma once
#ifndef NMSP_CLASSNAME_H
#define NMSP_CLASSNAME_H

#include "platform.h"

#ifdef __cplusplus

namespace nmsp {

class ClassName
{
public:
	ClassName();
	~ClassName();
	
private:
	// ...
	
};

}

typedef nmsp::ClassName NMSP_ClassName;

#else

typedef void NMSP_ClassName;

#endif

#ifdef __cplusplus
extern "C" {
#endif

SEV_ClassName *NMSP_ClassName_create();
void NMSP_ClassName_destroy(NMSP_ClassName *className);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef NMSP_CLASSNAME_H */

/* end of file */
