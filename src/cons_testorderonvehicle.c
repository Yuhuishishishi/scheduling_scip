#include <assert.h>

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

struct SCIP_ConsData
{
   int                   tid1;            /**< item id one */
   int 					 tid2	
   int                   vid;            /**< item id two */
   CONSTYPE              type;               /**< stores whether the items have to be in the SAME or DIFFER packing */
   int                   npropagatedvars;    /**< number of variables that existed, the last time, the related node was
                                              *   propagated, used to determine whether the constraint should be
                                              *   repropagated*/
   int                   npropagations;      /**< stores the number propagations runs of this constraint */
   unsigned int          propagated:1;       /**< is constraint already propagated? */
   SCIP_NODE*            node;               /**< the node in the B&B-tree at which the cons is sticking */
};



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