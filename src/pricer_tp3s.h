#ifndef _SCIP_PRICER_TP3S_H_
#define _SCIP_PRICER_TP3S_H_ 

#include "scip/scip.h"

/** creates the binpacking variable pricer and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludePricerTP3S(
   SCIP*                 scip                /**< SCIP data structure */
   );



/** added problem specific data to pricer and activates pricer */
extern
SCIP_RETCODE SCIPpricerTP3SActivate(
   SCIP* 					scip,
   SCIP** 					testConss,
   SCIP**					vehicleConss,
   TETS*					testArr,
   VEHICLE*					vehicleArr,
   int**					rehitRules,
   int 						numTests,
   int 						numVehicles
   );


#endif