This code is for testing drm ioctl in Genbu02 driver, 
if you want to used it,please copy the gb_uk.h to this directory.

Compile step:

1. cp ../../common/gb_uk.h ./

2. change the "#include <uapi/drm/drm.h>" to "#include "drm.h"" in gb_uk.h

3. add "#include <stdlib.h>" to gb_uk.h

4. make
