#ifndef _SCIP_BRANCH_TP3S_H_
#define _SCIP_BRANCH_TP3S_H_ 

#include "scip/scip.h"

/** creates the ryanfoster branching rule and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeBranchrule(
   SCIP*                 scip                /**< SCIP data structure */
   );

#endif