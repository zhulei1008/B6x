#include <stdio.h>
#include "svn_ver.h"

void drvs_build_info(void)
{
    printf("drvs build <Ver:%d> <Date:%s>\r\n", SVN_VER, __DATE__);
}
