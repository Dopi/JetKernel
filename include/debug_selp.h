
#if 0
//-----------------------------------------------------------------------
// DEBUG
//
// DEBUG_LEVEL 	0 : no debug message
// 				1 : show debug message
#define DEBUG_LEVEL 	 1
#define MID "[ name ] "
#include <debug_selp.h>
#define DFLAG		XXX
//-----------------------------------------------------------------------
#endif


#include "dprintk.h"
//-----------------------------------------------------------------------
// DEBUG_ALL 	0 : no debug message
// 				1 : show debug message
#define DEBUG_ALL 	1


#if ( DEBUG_LEVEL > 0) && (DEBUG_ALL > 0)
#define DL1 "%s "
#define DR1 ,__FUNCTION__
#define DL2 "[%d] "
#define DR2 ,__LINE__

#define D(format,...)\
	dprintk (DFLAG,MID DL1 DL2 format "\n" DR1 DR2, ## __VA_ARGS__)

#define M(format,...)\
	dprintk (DFLAG,MID "[MSG] " DL2 format "\n" DR2, ## __VA_ARGS__)

#define I(format,...)\
	dprintk (DFLAG,MID DL1 DL2 format "\n" DR1 DR2, ## __VA_ARGS__)

#define W(format,...)\
	dprintk (DFLAG,MID "[MSG] " DL2 format "\n" DR2, ## __VA_ARGS__)

#define FN_IN(ARG)		dprintk (DFLAG,MID "[IN] %s (%d)\n",__FUNCTION__,ARG)
#define FN_OUT(ARG) 	dprintk (DFLAG,MID "[OUT] %s[%d] (%d)\n",__FUNCTION__,__LINE__,ARG)
//---------------------------------------------------------------------
#else
#define D(format,...)
#define M(format,...)
#define I(format,...)
#define W(format,...)
#define FN_IN(ARG)
#define FN_OUT(ARG)
#endif
//-----------------------------------------------------------------------



