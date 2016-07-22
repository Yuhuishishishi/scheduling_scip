#include "pricer_tp3s.h"
#include "probdata_tp3s.h"
#include "vardata_tp3s.h"

#include <assert.h>

#define PRICER_NAME            "tp3s pricer"
#define PRICER_DESC            "pricer for tp3s columns"
#define PRICER_PRIORITY        0
#define PRICER_DELAY           TRUE     /* only call pricer if all problem variables have non-negative reduced costs */

struct SCIP_PricerData
{
	SCIP_CONSHDLR*				sameDiffhdlr;
	SCIP_CONSHDLR*				testOnVehiclehdlr;
	SCIP_CONSHDLR*				testOrderhdlr;
	SCIP_CONS**					testConss;
	SCIP_CONS**					vehicleConss;

	int							numTests;
	int							numVehicles;

	TEST*						testArr;
	VEHICLE*					vehicleArr;
	int**						rehitRules;
};

static
SCIP_RETCODE initPricing(
	SCIP*					scip,
	SCIP_PRICERDATA*		pricerdata
	) 
{
	assert(pricerdata != NULL);

	int numTests = pricerdata->numTests;
	int numVehicles = pricerdata->numVehicles;
	SCIP_CONS** testConss = pricerdata->testConss;
	SCIP_CONS* vehicleConss = pricerdata->vehicleConss;

	TEST* testArr = pricerdata->testArr;
	VEHICLE* vehicleArr = pricerdata->vehicleArr;

	int** rehitRules = pricerdata->rehitRules;


}

/** destructor of variable pricer to free user data (called when SCIP is exiting) */
static
SCIP_DECL_PRICERFREE(pricerFreeTP3S)
{
   SCIP_PRICERDATA* pricerdata;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);

   if( pricerdata != NULL)
   {
      /* free memory */
      SCIPfreeMemoryArrayNull(scip, &pricerdata->testConss);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->vehicleConss);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->testArr);
      SCIPfreeMemoryArrayNull(scip, &pricerdata->vehicleArr);

      for (int i = 0; i < &pricerdata->numTests; ++i)
      {
      	SCIPfreeMemoryArrayNull(scip, &pricerdata->rehitRules[i]);
      }
      SCIPfreeMemoryArrayNull(scip, &pricerdata->rehitRules);

      SCIPfreeMemory(scip, &pricerdata);
   }

   return SCIP_OKAY;
}

/** initialization method of variable pricer (called after problem was transformed) */
static
SCIP_DECL_PRICERINIT(pricerInitTP3S)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;
   SCIP_CONS* cons;
   int c;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   /* get transformed constraints */
   for( c = 0; c < pricerdata->numTests; ++c )
   {
      cons = pricerdata->testConss[c];

      /* release original constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &pricerdata->testConss[c]) );

      /* get transformed constraint */
      SCIP_CALL( SCIPgetTransformedCons(scip, cons, &pricerdata->testConss[c]) );

      /* capture transformed constraint */
      SCIP_CALL( SCIPcaptureCons(scip, pricerdata->testConss[c]) );
   }

      /* get transformed constraints */
   for( c = 0; c < pricerdata->numVehicles; ++c )
   {
      cons = pricerdata->vehicleConss[c];

      /* release original constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &pricerdata->vehicleConss[c]) );

      /* get transformed constraint */
      SCIP_CALL( SCIPgetTransformedCons(scip, cons, &pricerdata->vehicleConss[c]) );

      /* capture transformed constraint */
      SCIP_CALL( SCIPcaptureCons(scip, pricerdata->vehicleConss[c]) );
   }

   return SCIP_OKAY;
}

/** solving process deinitialization method of variable pricer (called before branch and bound process data is freed) */
static
SCIP_DECL_PRICEREXITSOL(pricerExitsolTP3S)
{
   SCIP_PRICERDATA* pricerdata;
   int c;

   assert(scip != NULL);
   assert(pricer != NULL);

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);

   /* get release constraints */
   for( c = 0; c < pricerdata->numTests; ++c )
   {
      /* release constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &(pricerdata->testConss[c])) );
   }

      /* get release constraints */
   for( c = 0; c < pricerdata->numVehicles; ++c )
   {
      /* release constraint */
      SCIP_CALL( SCIPreleaseCons(scip, &(pricerdata->vehicleConss[c])) );
   }

   return SCIP_OKAY;
}

static
SCIP_DECL_PRICERFARKAS(pricerFarkasTP3S)
{  /*lint --e{715}*/

   /** @note In case of this binpacking example, the master LP should not get infeasible after branching, because of the
    *        way branching is performed. Therefore, the Farkas pricing is not implemented.
    *        1. In case of Ryan/Foster branching, the two items are selected in a way such that the sum of the LP values
    *           of all columns/packings containing both items is fractional. Hence, it exists at least one
    *           column/packing which contains both items and also at least one column/packing for each item containing
    *           this but not the other item. That means, branching in the "same" direction stays LP feasible since there
    *           exists at least one column/packing with both items and branching in the "differ" direction stays LP
    *           feasible since there exists at least one column/packing containing one item, but not the other.
    *        2. In case of variable branching, we only branch on fractional variables. If a variable is fixed to one,
    *           there is no issue.  If a variable is fixed to zero, then we know that for each item which is part of
    *           that column/packing, there exists at least one other column/packing containing this particular item due
    *           to the covering constraints.
    */
   SCIPwarningMessage(scip, "Current master LP is infeasible, but Farkas pricing was not implemented\n");
   SCIPABORT();

   return SCIP_OKAY; /*lint !e527*/
}
