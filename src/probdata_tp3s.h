#ifndef _SCIP_PROBDATA_TP3S_H_
#define _SCIP_PROBDATA_TP3S_H_

#include "scip/scip.h"
#include "vardata_tp3s.h"
#include "data_structure.h"

extern 
SCIP_RETCODE SCIPprobdataCreate(
	SCIP*			scip,
	const char*		probname,
	TEST*			tests,
	VEHICLE*		vehicles,
	int 			numTests,
	int 			numVehicles,
	int**			rehits);

extern 
TEST* SCIPprobdataGetTests(
	SCIP_PROBDATA*	probdata);

extern
VEHICLE* SCIPprobdataGetVehicles(
	SCIP_PROBDATA* 	probdata);

extern
int** SCIPprobdataGetRehitRules(
	SCIP_PROBDATA* 	probdata);

extern
int SCIPprobdataGetNumTests(
	SCIP_PROBDATA* 	probdata);

extern
int SCIPprobdataGetNumVehicles(
	SCIP_PROBDATA* 	probdata);

/** returns array of all variables ordered in the way they got generated */
extern
SCIP_VAR** SCIPprobdataGetVars(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns number of variables */
extern
int SCIPprobdataGetNVars(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   );

/** returns array of set partitioning constrains */
extern
SCIP_CONS** SCIPprobdataGetTestConss(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   );

extern
SCIP_CONS** SCIPprobdataGetVehicleConss(
	SCIP_PROBDATA*		probdata
	);


/** adds given variable to the problem data */
extern
SCIP_RETCODE SCIPprobdataAddVar(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata,           /**< problem data */
   SCIP_VAR*             var                 /**< variables to add */
   );


#endif