
#include "class_name.h"

namespace nmsp {

ClassName::ClassName()
{
	// ...
}

ClassName::~ClassName()
{
	// ...
}

}

NMSP_ClassName *NMSP_ClassName_create()
{
	return new NMSP_ClassName();
}

void NMSP_ClassName_destroy(NMSP_ClassName *className)
{
	delete className;
}

/* end of file */
