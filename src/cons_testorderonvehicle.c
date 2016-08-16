#include <assert.h>
#include <string.h>

#include "cons_testorderonvehicle.h"
#include "probdata_tp3s.h"
#include "vardata_tp3s.h"

#define CONSHDLR_NAME          "testorderonvehicle"
#define CONSHDLR_DESC          "stores the local branching decisions two tests should be assigned in order to a vehicle "
#define CONSHDLR_ENFOPRIORITY         0 /**< priority of the constraint handler for constraint enforcing */
#define CONSHDLR_CHECKPRIORITY  9999999 /**< priority of the constraint handler for checking feasibility */
#define CONSHDLR_PROPFREQ             1 /**< frequency for propagating domains; zero means only preprocessing propagation */
#define CONSHDLR_EAGERFREQ            1 /**< frequency for using all instead of only the useful constraints in separation,
                                         *   propagation and enforcement, -1 for no eager evaluations, 0 for first only */
#define CONSHDLR_DELAYPROP        FALSE /**< should propagation method be delayed, if other propagators found reductions? */
#define CONSHDLR_NEEDSCONS         TRUE /**< should the constraint handler be skipped, if no constraints are available? */

#define CONSHDLR_PROP_TIMING       SCIP_PROPTIMING_BEFORELP


struct SCIP_ConsData
{
   int                   tid1;            /**< item id one */
   int 					         tid2	
   int                   vid;            /**< item id two */
   CONSTYPE              type;               /**< stores whether the items have to be in the SAME or DIFFER packing */
   int                   npropagatedvars;    /**< number of variables that existed, the last time, the related node was
                                              *   propagated, used to determine whether the constraint should be
                                              *   repropagated*/
   int                   npropagations;      /**< stores the number propagations runs of this constraint */
   unsigned int          propagated:1;       /**< is constraint already propagated? */
   SCIP_NODE*            node;               /**< the node in the B&B-tree at which the cons is sticking */
};

static
SCIP_RETCODE consdataCreate(
  SCIP*                   scip,               /**< SCIP data structure */
    SCIP_CONSDATA**       consdata,               /**< pointer to hold the created constraint */
    int                   tid1,
    int                   tid2,               /**< item id one */
    int                   vid,         /**< vehicle id */
    CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
    SCIP_NODE*            node              /**< the node in the B&B-tree at which the cons is sticking */
  )
{
  assert( scip != NULL);
  assert( consdata != NULL);
  assert( tid1 >= 0);
  assert( tid2 >= 0);
  assert( vid >= 0);
  assert( type == ENFORCE || type == FORBID);
    
    SCIP_CALL( SCIPallocBlockMemory(scip, consdata) );

    (*consdata)->tid1 = tid1;
    (*consdata)->tid2 = tid2;
    (*consdata)->vid = vid;
    (*consdata)->type = type;
    (*consdata)->npropagatedvars = 0;
    (*consdata)->npropagations = 0;
    (*consdata)->propagated = FALSE;
    (*consdata)->node = node;

    return SCIP_OKAY;
}

/** display constraints */
static 
void consdataPrint(
  SCIP*       scip, 
  SCIP_CONSDATA*    consdata,
  FILE*       file
  )
{
  SCIPinfoMessage(scip, file, "%s(%d,%d on vehicle %d) at node %d\n",
    consdata->type == ENFORCE? "enforce" : "forbid",
    consdata->tid1, consdata->tid2, consdata->vid, SCIPnodeGetNumber(consdata->node) );
}

/** fixes a variable to zero if the corresponding packings are not valid for this constraint/node (due to branching) */
static
SCIP_RETCODE checkVariable(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR*             var,                /**< variables to check  */
   int*                  nfixedvars,         /**< pointer to store the number of fixed variables */
   SCIP_Bool*            cutoff              /**< pointer to store if a cutoff was detected */
   )

{

    /** need to consider the different column cost thing */
    SCIP_VARDATA* vardata;
    int* testConsids;
    int nconsids;
    int vehicleIds;  

    SCIP_Bool existid1;
    SCIP_Bool existid2;
    CONSTYPE type;

    SCIP_Bool fixed;
    SCIP_Bool infeasible;

    int pos;

    assert(scip != NULL);
    assert(consdata != NULL);
    assert(var != NULL);
    assert(nfixedvars != NULL);
    assert(cutoff != NULL);

    /* if variables is locally fixed to zero continue */
    if( SCIPvarGetUbLocal(var) < 0.5 )
      return SCIP_OKAY;


    /* check if the packing which corresponds to the variable feasible for this constraint */
    vardata = SCIPvarGetData(var);

    testConsids = SCIPvardataGetConsids(vardata);
    nconsids = SCIPvardataGetNConsids(vardata);
    vehicleIds = SCIPvardataGetVehicleConsids(vardata);

    existid1 = SCIPsortedvecFindInt(testConsids, consdata->tid1, nconsids, &pos);
    existid2 = SCIPsortedvecFindInt(testConsids, consdata->tid2, nconsids, &pos);
    type = consdata->type;

    /** situations the ub needs to be 0:
    ENFORCE the constraint, col contains either tests but assign to different vehicle
    ENFORCE the constraint, col contains single test 
    FORBID the constraint, col contains both tests and assign to the vehicle */
    if ((type==ENFORCE && existid1 && existid2 && vehicleIds!=consdata->vid)
        || (type==ENFORCE && existid1 && !existid2)
        || (type==ENFORCE && existid2 && !existid1)
        || (type==FORBID && existid1 && existid2 && vehicleIds==consdata->vid))
    {       
        SCIP_CALL( SCIPfixVar(scip, var, 0.0, &infeasible, &fixed) );
        if( infeasible )
        {
          assert( SCIPvarGetLbLocal(var) > 0.5 );
          SCIPdebugMessage("-> cutoff\n");
          (*cutoff) = TRUE;
        }
        else
        {
          assert(fixed);
          (*nfixedvars)++;
        }
    }

    return SCIP_OKAY;

}


int SCIPgetTid1TestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->tid1;
}

int SCIPgetTid2TestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->tid2;
}

int SCIPgetVidTestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->vid;
}

int SCIPgetTypeTestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   )
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->type;
}