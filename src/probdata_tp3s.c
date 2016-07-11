#include "scip/scip.h"
#include "scip/cons_setppc.h"


#define EVENTHDLR_NAME         "addedvar"
#define EVENTHDLR_DESC         "event handler for catching added variables"

struct  SCIP_ProbData
{
	SCIP_VAR**			var;
	SCIP_CONS** 		testConss;
	SCIP_CONS**			vehicleConss;

	TEST*				test;
	VEHICLE*			vehicles;
	int**				rehits;
	int					numTests;
	int 				numVehicles;
};


static
SCIP_RETCODE probdataCreate(
	SCIP*			scip,
	SCIP_PROBDATA**	probdata,
	SCIP_VAR**		vars,
	SCIP_CONS**		testConss,
	SCIP_CONS**		vehicleConss,
	int  			nvars,
	int 			numTests,
	int 			numVehicles,
	TEST*			tests,
	VEHICLE*		vehicles,
	int**			rehits
	)
{
	assert(scip != NULL);
	assert(probdata != NULL);

	SCIP_CALL( SCIPallocMemory(scip, probdata));

	if (nvars > 0)
	{
		SCIP_CALL( SCIPduplicateMemoryArray(scip, &(*probdata)->vars, vars, nvars));
	} 
	else 
		(*probdata)->vars = NULL;

	SCIP_CALL(SCIPduplicateMemoryArray(scip, &(*probdata)->tests, tests, numTests));
	SCIP_CALL(SCIPduplicateMemoryArray(scip, &(*probdata)->vehicles, vehicles, numVehicles));
	SCIP_CALL(SCIPduplicateMemoryArray(scip, &(*probdata)->rehits, rehits, numTests));
	for (int i = 0; i < numTests; ++i)
	{
		SCIP_CALL(SCIPduplicateMemoryArray(scip, &(*probdata)->rehits[i], rehits[i], numTests));
	}

	(*probdata)->nvars = nvars;
	(*probdata)->numTests = numTests;
	(*probdata)->numVehicles = numVehicles;

	return SCIP_OKAY;
}

SCIP_RETCODE SCIPprobdataCreate(
	SCIP*			scip,
	const char*		probname,
	TEST*			tests,
	int 			numTests,
	VEHICLE*		vehicles,
	int 			numVehicles,
	int**			rehits)
{
	SCIP_PROBDATA* probdata;
	SCIP_CONS** testConss;
	SCIP_CONS** vehicleConss;
	char name[SCIP_MAXSTRLEN];

	assert(scip != NULL);

	/* if cannot find event handler, create the handler */

	/* set callbacks of problem data processing */

	/* set objective sense */
   	SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE) );

   	/* tell SCIP that the objective will be always integral */
   	SCIP_CALL( SCIPsetObjIntegral(scip) );

   	SCIP_CALL( SCIPallocBufferArray(scip, &testConss, numTests));
   	SCIP_CALL( SCIPallocBufferArray(scip, &vehicleConss, numVehicles));

   	/* create set covering constraint for each item */
   	for (int i = 0; i < numTests; ++i)
   	{
   		(void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "test_%d", tests[i].tid);
   		SCIP_CALL( SCIPcreateConsBasicSetcover(scip, &testConss[i], name, 0, NULL) );
   		SCIP_CALL( SCIPaddCons(scip, testConss[i]) );   
   	}

   	/* create the set packing constraint for each vehicle */
   	for (int i = 0; i < numVehicles; ++i)
   	{
   		(void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "vehicle_%d", tests[i].tid);
   		SCIP_CALL( SCIPcreateConsBasicSetpack(scip, &vehicleConss[i], name, 0, NULL));
   		SCIP_CALL( SCIPaddCons(scip, vehicleConss[i]));   
   	}

   	/* create problem data */

}