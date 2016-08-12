#include "probdata_tp3s.h"
#include "vardata_tp3s.h"

struct SCIP_VarData
{
	int*			testConsids;
	int				nconsids;
	int				vehicleConsid;
};

static
SCIP_RETCODE vardataCreate(
	SCIP*					scip,
	SCIP_VARDATA**			vardata,
	int*					testConsids,
	int						nconsids,
	int						vehicleConsid
	)
{
	SCIP_CALL( SCIPallocBlockMemory(scip, vardata) );
	SCIP_CALL( SCIPduplicateBlockMemoryArray(scip, &(*vardata)->testConsids, testConsids, nconsids) );

   	SCIPsortInt((*vardata)->testConsids, nconsids);

   	(*vardata)->nconsids = nconsids;
   	(*vardata)->testConsids = testConsids;

   	return SCIP_OKAY;
}

/** frees user data of variable */
static
SCIP_RETCODE vardataDelete(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata             /**< vardata to delete */
   )
{
   SCIPfreeBlockMemoryArray(scip, &(*vardata)->testConsids, (*vardata)->nconsids);
   SCIPfreeBlockMemory(scip, vardata);

   return SCIP_OKAY;
}

/** frees user data of transformed variable (called when the transformed variable is freed) */
static
SCIP_DECL_VARDELTRANS(vardataDelTrans)
{
   SCIP_CALL( vardataDelete(scip, vardata) );

   return SCIP_OKAY;
}

/** create variable data */
SCIP_RETCODE SCIPvardataCreateTP3S(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA**        vardata,            /**< pointer to vardata */
   int*                  testConsids,            /**< array of constraints ids */
   int                   nconsids,            /**< number of constraints */
   int 					 vehicleConsid)
{
   SCIP_CALL( vardataCreate(scip, vardata, testConsids, nconsids, vehicleConsid) );

   return SCIP_OKAY;
}

/** get number of test constraints */
int SCIPvardataGetNConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   )
{
   return vardata->nconsids;
}


/** returns sorted constraint id array */
int* SCIPvardataGetConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   )
{
   /* check if the consids are sorted */
#ifndef NDEBUG
   {
      int i;

      for( i = 1; i < vardata->testConsids; ++i )
         assert( vardata->testConsids[i-1] < vardata->testConsids[i]);
   }
#endif

   return vardata->testConsids;
}

int SCIPvardataGetVehicleConsids(
   SCIP_VARDATA*        vardata)
{
   return vardata->vehicleConsid;
}

/** creates variable */
SCIP_RETCODE SCIPcreateVarTP3S(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR**            var,                /**< pointer to variable object */
   const char*           name,               /**< name of variable, or NULL for automatic name creation */
   SCIP_Real             obj,                /**< objective function value */
   SCIP_Bool             initial,            /**< should var's column be present in the initial root LP? */
   SCIP_Bool             removable,          /**< is var's column removable from the LP (due to aging or cleanup)? */
   SCIP_VARDATA*         vardata             /**< user data for this specific variable */
   )
{
   assert(scip != NULL);
   assert(var != NULL);

   /* create a basic variable object */
   SCIP_CALL( SCIPcreateVarBasic(scip, var, name, 0.0, 1.0, obj, SCIP_VARTYPE_BINARY) );
   assert(*var != NULL);

   /* set callback functions */
   SCIPvarSetData(*var, vardata);
   SCIPvarSetDeltransData(*var, vardataDelTrans);

   /* set initial and removable flag */
   SCIP_CALL( SCIPvarSetInitial(*var, initial) );
   SCIP_CALL( SCIPvarSetRemovable(*var, removable) );

   SCIPvarMarkDeletable(*var);

   // SCIPdebug( SCIPprintVar(scip, *var, NULL) );

   return SCIP_OKAY;
}



/** prints vardata to file stream */
void SCIPvardataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA*         vardata,            /**< variable data */
   FILE*                 file                /**< the text file to store the information into */
   )
{
   // place holder, do nothing
}