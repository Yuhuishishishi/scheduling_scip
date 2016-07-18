#include "scip/scip.h"
#include "probdata_tp3s.h"



#define EVENTHDLR_NAME         "addedvar"
#define EVENTHDLR_DESC         "event handler for catching added variables"

struct  SCIP_ProbData
{
	SCIP_VAR**			vars;
	SCIP_CONS** 		testConss;
	SCIP_CONS**			vehicleConss;

	TEST*				tests;
	VEHICLE*			vehicles;
	int**				rehits;
	int					numTests;
	int 				numVehicles;

	int 				nvars;
	int 				varssize;
};


/** execution method of event handler */
static
SCIP_DECL_EVENTEXEC(eventExecAddedVar)
{  /*lint --e{715}*/
   assert(eventhdlr != NULL);
   assert(strcmp(SCIPeventhdlrGetName(eventhdlr), EVENTHDLR_NAME) == 0);
   assert(event != NULL);
   assert(SCIPeventGetType(event) == SCIP_EVENTTYPE_VARADDED);

   SCIPdebugMessage("exec method of event handler for added variable to probdata\n");

   /* add new variable to probdata */
   SCIP_CALL( SCIPprobdataAddVar(scip, SCIPgetProbData(scip), SCIPeventGetVar(event)) );
}


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
	(*probdata)->varssize = nvars;

	return SCIP_OKAY;
}


/** frees the memory of the given problem data */
static
SCIP_RETCODE probdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA**       probdata            /**< pointer to problem data */
   )
{
   assert(scip != NULL);
   assert(probdata != NULL);

   /* release all variables */
   for(int i = 0; i < (*probdata)->nvars; ++i )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &(*probdata)->vars[i]) );
   }

   /* release all constraints */
   for(int i = 0; i < (*probdata)->numTests; ++i )
   {
      SCIP_CALL( SCIPreleaseCons(scip, &(*probdata)->testConss[i]) );
   }

   for (int i = 0; i < (*probdata)->numVehicles; ++i)
   {
      SCIP_CALL( SCIPreleaseCons(scip, &(*probdata)->vehicleConss[i]));
   }

   /* free memory of arrays */
   SCIPfreeMemoryArray(scip, &(*probdata)->vars);
   SCIPfreeMemoryArray(scip, &(*probdata)->testConss);
   SCIPfreeMemoryArray(scip, &(*probdata)->vehicleConss);
   
   SCIPfreeMemoryArray(scip, &(*probdata)->tests);
   SCIPfreeMemoryArray(scip, &(*probdata)->vehicles);
   for (int i = 0; i < (*probdata)->numTests; ++i)
   {
   		SCIPfreeMemoryArray(scip, &(*probdata)->rehits[i]);
   }
   SCIPfreeMemoryArray(scip, &(*probdata)->rehits);

   /* free probdata */
   SCIPfreeMemory(scip, probdata);
}

static
SCIP_RETCODE createInitialColumns(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
	SCIP_CONS** testConss;
	SCIP_CONS** vehicleConss;
	SCIP_VAR* var;

	char name[SCIP_MAXSTRLEN];

	int numTests, numVehicles;
	TEST* tests;
	VEHICLE* vehicles;
	int** rehits;

	testConss = probdata->testConss;
	vehicleConss = probdata->vehicleConss;

	numTests = probdata->numTests;
	numVehicles = probdata->numVehicles;

	tests = probdata->tests;
	vehicles = probdata->vehicles;
	rehits = probdata->rehits;

	/* columns contains single test */
	for (int i = 0; i < numTests; ++i)
	{

		for (int v = 0; v < numVehicles; ++v)
		{
         int vehicleRelease;
         int testDur, testDeadline, testRelease;
         int realRelease;
         int cost;
         int a;

			(void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "item_%d_on_vehicle_%d", i,v);

         vehicleRelease = vehicles[v].release;
         testDur = tests[i].dur;
         testDeadline = tests[i].deadline;
         testRelease = tests[i].release;

			/* compute the cost of assignment */
         realRelease = vehicleRelease < testRelease ? vehicleRelease : testRelease;
         cost = realRelease + testDur - testDeadline;
         cost = cost > 0 ? cost : 0;
         cost += 50;

			/* create variable */
         SCIP_CALL( SCIPcreateVarTP3S(scip, &var, name, cost, TRUE, TRUE, NULL));

			/* add variable to the problem */
      	SCIP_CALL( SCIPaddVar(scip, var) );

      	/* store variable in the problme data */
      	SCIP_CALL( SCIPprobdataAddVar(scip, probdata, var) );

      	/* add variable to corresponding set covering constraint */
      	SCIP_CALL( SCIPaddCoefSetppc(scip, testConss[i], var) );
      	SCIP_CALL( SCIPaddCoefSetppc(scip, vehicleConss[v], var));


      	/* create the variable data for the variable; the variable data contains the information in which constraints the
       	* variable appears */
         a = i;
         SCIP_CALL( SCIPvardataCreateTP3S(scip, &vardata, &a, 1, v));

       	/* add the variable data to the variable */
      	SCIPvarSetData(var, vardata);


      	/* change the upper bound of the binary variable to lazy since the upper bound is already enforced
       		* due to the objective function the set covering constraint;
       		* The reason for doing is that, is to avoid the bound of x <= 1 in the LP relaxation since this bound
       		* constraint would produce a dual variable which might have a positive reduced cost
       		*/
      	SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

      	/* release variable */
      	SCIP_CALL( SCIPreleaseVar(scip, &var) );
		}
	}

	/* columns that contains two tests */
	for (int i = 0; i < numTests; ++i)
	{
		for (int j = 0; j < numTests; ++j)
		{
			/* if self or not allowed by compatibility, then continue */
			if (i==j || !rehits[i][j])
				continue;
			for (int v = 0; v < numVehicles; ++v)
			{

            int vehicleRelease;
            int realRelease;
            int cost = 0;
            int test1Dur, test1Release, test1Deadline;
            int test2Dur, test2Release, test2Deadline;
            int* consids;

				(void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "item_%d,%d_on_vehicle_%d", i,j,v);

            vehicleRelease = vehicles[v].release;
            
            test1Dur = tests[i].dur;
            test1Deadline = tests[i].deadline;
            test1Release = tests[i].release;

            test2dur = tests[j].dur;
            test2Deadline = tests[j].deadline;
            test2Release = tests[j].release;

				/* compute the cost of assignment */
            realRelease = vehicleRelease < test1Release ? vehicleRelease : test1Release;
            realRelease += test1Dur;
            cost = realRelease - test1Deadline;
            cost = cost > 0 ? cost : 0;

            realRelease = realRelease < test2Release ? realRelease : test2Release;
            realRelease += test2Dur;
            cost += (realRelease - test2Deadline > 0) ? (realRelease - test2Deadline) : 0;

				/* create variable */
            SCIP_CALL( SCIPcreateVarTP3S(scip, &var, name, cost, TRUE, TRUE, NULL));


				/* add variable to the problem */
	      	SCIP_CALL( SCIPaddVar(scip, var) );

	      	/* store variable in the problme data */
	      	SCIP_CALL( SCIPprobdataAddVar(scip, probdata, var) );

	      	/* add variable to corresponding set covering constraint */
	      	SCIP_CALL( SCIPaddCoefSetppc(scip, testConss[i], var) );
	      	SCIP_CALL( SCIPaddCoefSetppc(scip, testConss[j], var) );
	      	SCIP_CALL( SCIPaddCoefSetppc(scip, vehicleConss[v], var));


	      	/* create the variable data for the variable; the variable data contains the information in which constraints the
	       		* variable appears */
            SCIP_CALL( SCIPallocBufferArray(scip, &consids, 2));
            consids[0] = i;
            consids[1] = j;
            SCIP_CALL( SCIPvardataCreateTP3S(scip, &vardata, consids, 2, v));

	       	/* add the variable data to the variable */
	      	SCIPvarSetData(var, vardata);


	      	/* change the upper bound of the binary variable to lazy since the upper bound is already enforced
	       		* due to the objective function the set covering constraint;
	       		* The reason for doing is that, is to avoid the bound of x <= 1 in the LP relaxation since this bound
	       		* constraint would produce a dual variable which might have a positive reduced cost
	       		*/
	      	SCIP_CALL( SCIPchgVarUbLazy(scip, var, 1.0) );

	      	/* release variable */
	      	SCIP_CALL( SCIPreleaseVar(scip, &var) );

            SCIPfreeBufferArray(scip, &consids);
			
			}
		}
	}

	return SCIP_OKAY;

}


/** frees user data of original problem (called when the original problem is freed) */
static
SCIP_DECL_PROBDELORIG(probdelorigTP3S)
{
   SCIPdebugMessage("free original problem data\n");

   SCIP_CALL( probdataFree(scip, probdata) );

   return SCIP_OKAY;
}

/** creates user data of transformed problem by transforming the original user problem data
 *  (called after problem was transformed) */
static
SCIP_DECL_PROBTRANS(probtransTP3S)
{
   /* create transform probdata */
   SCIP_CALL( probdataCreate(scip, targetdata, sourcedata->vars, 
   		sourcedata->testConss, sourcedata->vehicleConss,
        sourcedata->nvars, sourcedata->numTests, sourcedata->numVehicles,
        sourcedata->tests, sourcedata->vehicles, sourcedata->rehits) );

   /* transform all constraints */
   SCIP_CALL( SCIPtransformConss(scip, (*targetdata)->numTests, (*targetdata)->testConss, (*targetdata)->testConss) );
   SCIP_CALL( SCIPtransformConss(scip, (*targetdata)->numVehicles, (*targetdata)->vehicleConss, (*targetdata)->vehicleConss) );

   /* transform all variables */
   SCIP_CALL( SCIPtransformVars(scip, (*targetdata)->nvars, (*targetdata)->vars, (*targetdata)->vars) );

   return SCIP_OKAY;
}

/** frees user data of transformed problem (called when the transformed problem is freed) */
static
SCIP_DECL_PROBDELTRANS(probdeltransTP3S)
{
   SCIPdebugMessage("free transformed problem data\n");

   SCIP_CALL( probdataFree(scip, probdata) );

   return SCIP_OKAY;
}

/** solving process initialization method of transformed data (called before the branch and bound process begins) */
static
SCIP_DECL_PROBINITSOL(probinitsolTP3S)
{
   SCIP_EVENTHDLR* eventhdlr;

   assert(probdata != NULL);

   /* catch variable added event */
   eventhdlr = SCIPfindEventhdlr(scip, "addedvar");
   assert(eventhdlr != NULL);

   SCIP_CALL( SCIPcatchEvent(scip, SCIP_EVENTTYPE_VARADDED, eventhdlr, NULL, NULL) );

   return SCIP_OKAY;
}

/** solving process deinitialization method of transformed data (called before the branch and bound data is freed) */
static
SCIP_DECL_PROBEXITSOL(probexitsolTP3S)
{
   SCIP_EVENTHDLR* eventhdlr;

   assert(probdata != NULL);

   /* drop variable added event */
   eventhdlr = SCIPfindEventhdlr(scip, "addedvar");
   assert(eventhdlr != NULL);

   SCIP_CALL( SCIPdropEvent(scip, SCIP_EVENTTYPE_VARADDED, eventhdlr, NULL, -1) );


   return SCIP_OKAY;
}

SCIP_RETCODE SCIPprobdataCreate(
	SCIP*			scip,
	const char*		probname,
	TEST*			tests,
	VEHICLE*		vehicles,
	int 			numTests,
	int 			numVehicles,
	int**			rehits)
{
	SCIP_PROBDATA* probdata;
	SCIP_CONS** testConss;
	SCIP_CONS** vehicleConss;
	char name[SCIP_MAXSTRLEN];

	assert(scip != NULL);

	/* if cannot find event handler, create the handler */
   	if( SCIPfindEventhdlr(scip, EVENTHDLR_NAME) == NULL )
   	{
      	SCIP_CALL( SCIPincludeEventhdlrBasic(scip, NULL, EVENTHDLR_NAME, EVENTHDLR_DESC, eventExecAddedVar, NULL) );
   	}
	/* set callbacks of problem data processing */
	SCIP_CALL( SCIPcreateProbBasic(scip, probname) );

	SCIP_CALL( SCIPsetProbDelorig(scip, probdelorigTP3S));
	SCIP_CALL(SCIPsetProbTrans(scip, probtransTP3S));
	SCIP_CALL( SCIPsetProbDeltrans(scip, probdeltransTP3S));
	SCIP_CALL(SCIPsetProbInitsol(scip, probinitsolTP3S));
	SCIP_CALL( SCIPsetProbExitsol(scip, probexitsolTP3S));

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
   	SCIP_CALL( probdataCreate(scip, &probdata, NULL, testConss, vehicleConss,
   	 	0, numTests, numVehicles, 
   	 	tests, vehicles, rehits) );

   	SCIP_CALL( createInitialColumns(scip, probdata) );

   	/* set user problem data */
   	SCIP_CALL( SCIPsetProbData(scip, probdata) );

   	/* set pricer */
   	// SCIP_CALL( SCIPpricerBinpackingActivate(scip, conss, weights, ids, nitems, capacity) );

   	/* free local buffer arrays */
   	SCIPfreeBufferArray(scip, &testConss);
   	SCIPfreeBufferArray(scip, &vehicleConss);

}

TEST* SCIPprobdataGetTests(
	SCIP_PROBDATA*	probdata)
{
	return probdata->tests;
}

VEHICLE* SCIPprobdataGetVehicles(
	SCIP_PROBDATA* 	probdata)
{
	return probdata->vehicles;
}

int** SCIPprobdataGetRehitRules(
	SCIP_PROBDATA* 	probdata)
{
	return probdata->rehits;
}

int SCIPprobdataGetNumTests(
	SCIP_PROBDATA* 	probdata)
{
	return probdata->numTests;
}

int SCIPprobdataGetNumVehicles(
	SCIP_PROBDATA* 	probdata)
{
	return probdata->numVehicles;
}

/** returns array of all variables ordered in the way they got generated */
SCIP_VAR** SCIPprobdataGetVars(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
	return probdata->vars;
}

/** returns number of variables */
int SCIPprobdataGetNVars(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
	return probdata->nvars;
}

/** returns array of set covering constrains */
SCIP_CONS** SCIPprobdataGetTestConss(
   	SCIP_PROBDATA*        probdata            /**< problem data */
   )
{
	return probdata->testConss;
}

SCIP_CONS** SCIPprobdataGetVehicleConss(
	SCIP_PROBDATA*		probdata
	)
{
	return probdata->vehicleConss;
}

SCIP_RETCODE SCIPprobdataAddVar(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROBDATA*        probdata,           /**< problem data */
   SCIP_VAR*             var                 /**< variables to add */
   )
{
   /* check if enough memory is left */
   if( probdata->varssize == probdata->nvars )
   {
      probdata->varssize = MAX(100, probdata->varssize * 2);
      SCIP_CALL( SCIPreallocMemoryArray(scip, &probdata->vars, probdata->varssize) );
   }

   /* caputure variables */
   SCIP_CALL( SCIPcaptureVar(scip, var) );

   probdata->vars[probdata->nvars] = var;
   probdata->nvars++;

   SCIPdebugMessage("added variable to probdata; nvars = %d\n", probdata->nvars);

   return SCIP_OKAY;
}

