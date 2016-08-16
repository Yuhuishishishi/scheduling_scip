#include <assert.h>
#include <string.h>

#include "cons_testonvehicle.h"
#include "probdata_tp3s.h"
#include "vardata_tp3s.h"

#define CONSHDLR_NAME          "testonvehicle"
#define CONSHDLR_DESC          "stores the local branching decisions a test should be assigned to one vehicle"
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
   int                   tid;            /**< item id one */
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
	SCIP*                 scip,               /**< SCIP data structure */
   	SCIP_CONSDATA**           consdata,               /**< pointer to hold the created constraint */
   	int                   tid,	             /**< item id one */
   	int 				  vid,				 /**< vehicle id */
   	CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   	SCIP_NODE*            node              /**< the node in the B&B-tree at which the cons is sticking */
	)
{
	assert( scip != NULL);
	assert( consdata != NULL);
	assert( tid >= 0);
	assert( vid >= 0);
	assert( type == ENFORCE || type == FORBID);
   	
   	SCIP_CALL( SCIPallocBlockMemory(scip, consdata) );

   	(*consdata)->tid = tid;
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
	SCIP*				scip, 
	SCIP_CONSDATA*		consdata,
	FILE*				file
	)
{
	SCIPinfoMessage(scip, file, "%s(%d,%d) at node %d\n",
		consdata->type == ENFORCE? "enforce" : "forbid",
		consdata->tid, consdata->vid, SCIPnodeGetNumber(consdata->node) );
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
	SCIP_VARDATA* vardata;
	int* testConsids;
	int nconsids;
	int vehicleIds;

	SCIP_Bool existid;
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

   	existid = SCIPsortedvecFindInt(testConsids, consdata->tid, nconsids, &pos);
   	type = consdata->type;

   	/** situations the ub needs to be 0:
   		ENFORCE the test to be assigned to the test, but the column assigns the test to other vehicle
   		FORBID such assignment, but the column makes the assignment */
   	if ((type == ENFORCE && existid && (consdata->vid != vehicleIds))
   			|| (type == FORBID && existid && (consdata->vid = vehicleIds)))
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

/** fixes variables to zero if the corresponding packings are not valid for this sonstraint/node (due to branching) */
static
SCIP_RETCODE consdataFixVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA*        consdata,           /**< constraint data */
   SCIP_VAR**            vars,               /**< generated variables */
   int                   nvars,              /**< number of generated variables */
   SCIP_RESULT*          result              /**< pointer to store the result of the fixing */
   )
{
	int nfixedvars;
   	int v;
   	SCIP_Bool cutoff;

   	nfixedvars = 0;
   	cutoff = FALSE;

   	SCIPdebugMessage("check variables %d to %d\n", consdata->npropagatedvars, nvars);

   	for( v = consdata->npropagatedvars; v < nvars && !cutoff; ++v )
   	{
      	SCIP_CALL( checkVariable(scip, consdata, vars[v], &nfixedvars, &cutoff) );
   	}

   	SCIPdebugMessage("fixed %d variables locally\n", nfixedvars);

   	if( cutoff )
      	*result = SCIP_CUTOFF;
   	else if( nfixedvars > 0 )
      	*result = SCIP_REDUCEDDOM;

   	return SCIP_OKAY;	
}

#ifndef NDEBUG
static 
SCIP_Bool consdataCheck(
	SCIP*					scip, 
	SCIP_PROBDATA*			probdata,
	SCIP_CONSDATA* 			consdata,
	SCIP_Bool				beforeprop
	)
{
	SCIP_VAR** vars;
	int nvars;

	SCIP_VARDATA* vardata;
	SCIP_VAR* var;

	int* testConsids;
	int nconsids;
	int vehicleIds;
	SCIP_Bool existid;
	CONSTYPE type;

	int pos;
	int v;

   	vars = SCIPprobdataGetVars(probdata);
   	nvars = (beforeprop ? consdata->npropagatedvars : SCIPprobdataGetNVars(probdata));
   	assert(nvars <= SCIPprobdataGetNVars(probdata));

   	for (v=0; v<nvars; v++)
   	{
   		var = vars[v];
      	/* if variables is locally fixed to zero continue */
      	if( SCIPvarGetLbLocal(var) < 0.5 )
         	continue;
      	/* check if the packing which corresponds to the variable is feasible for this constraint */
      	vardata = SCIPvarGetData(var);

      	testConsids = SCIPvardataGetConsids(vardata);
      	nconsids = SCIPvardataGetNConsids(vardata);
		existid = SCIPsortedvecFindInt(testConsids, consdata->tid, nconsids, &pos);
   		type = consdata->type;

   		/** situations the ub needs to be 0:
   			ENFORCE the test to be assigned to the test, but the column assigns the test to other vehicle
   			FORBID such assignment, but the column makes the assignment */
   		if (type == ENFORCE && existid && (consdata->vid != vehicleIds)
   			|| type == FORBID && existid && (consdata->vid = vehicleIds))
   		{
   			SCIPdebug( SCIPvardataPrint(scip, vardata, NULL) );
         	SCIPdebug( consdataPrint(scip, consdata, NULL) );
         	SCIPdebug( SCIPprintVar(scip, var, NULL) );
         	return FALSE;
   		}
   	}
   	return TRUE;
}

#endif

/** frees test on vehicle constraint data */
static
SCIP_RETCODE consdataFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONSDATA**       consdata            /**< pointer to the constraint data */
   )
{
	assert(consdata != NULL);
 	assert(*consdata != NULL);

   	SCIPfreeBlockMemory(scip, consdata);

   	return SCIP_OKAY;
}

/** frees specific constraint data */
static
SCIP_DECL_CONSDELETE(consDeleteTestOnVehicle)
{  /*lint --e{715}*/
   assert(conshdlr != NULL);
   assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   assert(consdata != NULL);
   assert(*consdata != NULL);

   /* free samediff constraint */
   SCIP_CALL( consdataFree(scip, consdata) );

   return SCIP_OKAY;
}

/** transforms constraint data into data belonging to the transformed problem */
static
SCIP_DECL_CONSTRANS(consTransTestOnVehicle)
{
	SCIP_CONSDATA* sourcedata;
   	SCIP_CONSDATA* targetdata;

   	assert(conshdlr != NULL);
   	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   	assert(SCIPgetStage(scip) == SCIP_STAGE_TRANSFORMING);
   	assert(sourcecons != NULL);
   	assert(targetcons != NULL);

   	sourcedata = SCIPconsGetData(sourcecons);
   	assert(sourcedata != NULL);

   	/* create constraint data for target constraint */
   	SCIP_CALL( consdataCreate(scip, &targetdata, 
   		sourcedata->tid, sourcedata->vid, sourcedata->type, sourcedata->node));

   	/* create target constraint */
   	SCIP_CALL( SCIPcreateCons(scip, targetcons, SCIPconsGetName(sourcecons), conshdlr, targetdata,
         SCIPconsIsInitial(sourcecons), SCIPconsIsSeparated(sourcecons), SCIPconsIsEnforced(sourcecons),
         SCIPconsIsChecked(sourcecons), SCIPconsIsPropagated(sourcecons),
         SCIPconsIsLocal(sourcecons), SCIPconsIsModifiable(sourcecons),
         SCIPconsIsDynamic(sourcecons), SCIPconsIsRemovable(sourcecons), SCIPconsIsStickingAtNode(sourcecons)) );

   	return SCIP_OKAY;
}

/** constraint enforcing method of constraint handler for LP solutions */
#define consEnfolpTestOnVehicle NULL

/** constraint enforcing method of constraint handler for pseudo solutions */
#define consEnfopsTestOnVehicle NULL

/** feasibility check method of constraint handler for integral solutions */
#define consCheckTestOnVehicle NULL

/** domain propagation method of constraint handler */
static
SCIP_DECL_CONSPROP(consPropTestOnVehicle)
{
	SCIP_PROBDATA* probdata;
	SCIP_CONSDATA* consdata;

	SCIP_VAR** vars;
	int nvars;
	int c;

	assert(scip != NULL);
	assert(scip != NULL);
   	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   	assert(result != NULL);

   	SCIPdebugMessage("propagation constraints of constraint handler <"CONSHDLR_NAME">\n");

   	probdata = SCIPgetProbData(scip);
   	assert( probdata != NULL);

   	vars = SCIPprobdataGetVars(probdata);
   	nvars = SCIPprobdataGetNVars(probdata);

   	*result = SCIP_DIDNOTFIND;

   	for (c=0; c<nconss; ++c)
   	{
   		consdata = SCIPconsGetData(conss[c]);
#ifndef NDEBUG
   		{
   		    /* check if there are no equal consdatas */
   		    SCIP_CONSDATA* consdata2;
   		    int i;

   		    for (i=c+1; i<nconss; ++i)
   		    {
   		    	consdata2 = SCIPconsGetData(conss[i]);
   		    	assert( !(consdata->tid == consdata2->tid 
   		    		&& consdata->vid == consdata2->vid));
   		    }
	
   		}
#endif
   		if (!consdata->propagated)
   		{
   			SCIPdebugMessage("propagate constraint <%s> ", SCIPconsGetName(conss[c]));
   			SCIPdebug( consdataPrint(scip, consdata, NULL) );

         	SCIP_CALL( consdataFixVariables(scip, consdata, vars, nvars, result) );
         	consdata->npropagations++;

         	if (*result != SCIP_CUTOFF)
         	{
         		consdata->propagated = TRUE;
         		consdata->npropagatedvars = nvars;
         	}
         	else
         		break;
   		}

   		assert( consdataCheck(scip, probdata, consdata, FALSE));
   	}

   	return SCIP_OKAY;
}


/** variable rounding lock method of constraint handler */
#define consLockTestOnVehicle NULL

/** constraint activation notification method of constraint handler */
static 
SCIP_DECL_CONSACTIVE(consActiveTestOnVehicle)
{
	SCIP_CONSDATA* consdata;
   	SCIP_PROBDATA* probdata;

   	assert(scip != NULL);
   	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   	assert(cons != NULL);

   	probdata = SCIPgetProbData(scip);
   	assert(probdata != NULL);

   	consdata = SCIPconsGetData(cons);
   	assert(consdata != NULL);
   	assert(consdata->npropagatedvars <= SCIPprobdataGetNVars(probdata));

   	SCIPdebugMessage("activate constraint <%s> at node <%"SCIP_LONGINT_FORMAT"> in depth <%d>: ",
    SCIPconsGetName(cons), SCIPnodeGetNumber(consdata->node), SCIPnodeGetDepth(consdata->node));
   	SCIPdebug( consdataPrint(scip, consdata, NULL) );

   	if( consdata->npropagatedvars != SCIPprobdataGetNVars(probdata) )
   	{
      	SCIPdebugMessage("-> mark constraint to be repropagated\n");
      	consdata->propagated = FALSE;
      	SCIP_CALL( SCIPrepropagateNode(scip, consdata->node) );
   	}

   	/* check if all previously generated variables are valid for this constraint */
   	assert( consdataCheck(scip, probdata, consdata, TRUE) );

   	return SCIP_OKAY;
}

/** constraint deactivation notification method of constraint handler */
static 
SCIP_DECL_CONSDEACTIVE(consDeactiveTestOnVehicle)
{
   	SCIP_CONSDATA* consdata;
   	SCIP_PROBDATA* probdata;

   	assert(scip != NULL);
   	assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
   	assert(cons != NULL);

   	consdata = SCIPconsGetData(cons);
   	assert(consdata != NULL);
   	assert(consdata->propagated || SCIPgetNChildren(scip) == 0);

   	probdata = SCIPgetProbData(scip);
   	assert(probdata != NULL);

   	SCIPdebugMessage("deactivate constraint <%s> at node <%"SCIP_LONGINT_FORMAT"> in depth <%d>: ",
    	  SCIPconsGetName(cons), SCIPnodeGetNumber(consdata->node), SCIPnodeGetDepth(consdata->node));
   	SCIPdebug( consdataPrint(scip, consdata, NULL) );

   	/* set the number of propagated variables to current number of variables is SCIP */
   	consdata->npropagatedvars = SCIPprobdataGetNVars(probdata);

   	return SCIP_OKAY;	
}

/** constraint display method of constraint handler */
static
SCIP_DECL_CONSPRINT(consPrintTestOnVehicle)
{  /*lint --e{715}*/
   	SCIP_CONSDATA*  consdata;

   	consdata = SCIPconsGetData(cons);
   	assert(consdata != NULL);

   	consdataPrint(scip, consdata, file);

   	return SCIP_OKAY;
}

/** creates the handler for samediff constraints and includes it in SCIP */
SCIP_RETCODE SCIPincludeConshdlrTestOnVehicle(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
	SCIP_CONSHDLRDATA* conshdlrdata = NULL;
   	SCIP_CONSHDLR* conshdlr = NULL;

   	/* include constraint handler */
   	SCIP_CALL( SCIPincludeConshdlrBasic(scip, &conshdlr, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY, CONSHDLR_EAGERFREQ, CONSHDLR_NEEDSCONS,
         consEnfolpTestOnVehicle, consEnfopsTestOnVehicle, consCheckTestOnVehicle, consLockTestOnVehicle,
         conshdlrdata) );

   	assert(conshdlr != NULL);

   	SCIP_CALL( SCIPsetConshdlrDelete(scip, conshdlr, consDeleteTestOnVehicle) );
   	SCIP_CALL( SCIPsetConshdlrTrans(scip, conshdlr, consTransTestOnVehicle) );
   	SCIP_CALL( SCIPsetConshdlrProp(scip, conshdlr, consPropTestOnVehicle, CONSHDLR_PROPFREQ, CONSHDLR_DELAYPROP,
         CONSHDLR_PROP_TIMING) );
   	SCIP_CALL( SCIPsetConshdlrActive(scip, conshdlr, consActiveTestOnVehicle) );
   	SCIP_CALL( SCIPsetConshdlrDeactive(scip, conshdlr, consDeactiveTestOnVehicle) );
   	SCIP_CALL( SCIPsetConshdlrPrint(scip, conshdlr, consPrintTestOnVehicle) );

   	return SCIP_OKAY;
}

SCIP_RETCODE SCIPcreateConsTestOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   tid,	             /**< item id one */
   int 					    vid,				 /**< vehicle id */
   CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   )
{
	SCIP_CONSHDLR* conshdlr;
   	SCIP_CONSDATA* consdata;

   	/* find the samediff constraint handler */
   	conshdlr = SCIPfindConshdlr(scip, CONSHDLR_NAME);
   	if( conshdlr == NULL )
   	{
      	SCIPerrorMessage("test on vehicle constraint handler not found\n");
      	return SCIP_PLUGINNOTFOUND;
   	}

   	/* create the constraint specific data */
   	SCIP_CALL( consdataCreate(scip, &consdata, tid, vid, type, node) );

   /* create constraint */
   SCIP_CALL( SCIPcreateCons(scip, cons, name, conshdlr, consdata, FALSE, FALSE, FALSE, FALSE, TRUE,
         local, FALSE, FALSE, FALSE, TRUE) );

   SCIPdebugMessage("created constraint: ");
   SCIPdebug( consdataPrint(scip, consdata, NULL) );

   return SCIP_OKAY;

}


/** return test id */
int SCIPgetTidTestOnVehicle(
	SCIP*				scip,
	SCIP_CONS*			cons)
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->tid;
}

/** return vehicle id */
int SCIPgetVidTestOnVehicle(
	SCIP*				scip,
	SCIP_CONS*			cons)
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->vid;
}

/** return constraint type ENFORCE or FORBID */
CONSTYPE SCIPgetTypeTestOnVehicle(
	SCIP*				scip,
	SCIP_CONS*			cons)
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->type;
}

