#include <assert.h>
#include <string.h>

#include "cons_samediff.h"
#include "probdata_tp3s.h"
#include "vardata_tp3s.h"

#define CONSHDLR_NAME          "samediff"
#define CONSHDLR_DESC          "stores the local branching decisions if two tests should come together"
#define CONSHDLR_ENFOPRIORITY         0 /**< priority of the constraint handler for constraint enforcing */
#define CONSHDLR_CHECKPRIORITY  9999999 /**< priority of the constraint handler for checking feasibility */
#define CONSHDLR_PROPFREQ             1 /**< frequency for propagating domains; zero means only preprocessing propagation */
#define CONSHDLR_EAGERFREQ            1 /**< frequency for using all instead of only the useful constraints in separation,
                                         *   propagation and enforcement, -1 for no eager evaluations, 0 for first only */
#define CONSHDLR_DELAYPROP        FALSE /**< should propagation method be delayed, if other propagators found reductions? */
#define CONSHDLR_NEEDSCONS         TRUE /**< should the constraint handler be skipped, if no constraints are available? */

#define CONSHDLR_PROP_TIMING       SCIP_PROPTIMING_BEFORELP

/** Constraint data for  \ref cons_samediff.c "SameDiff" constraints */
struct SCIP_ConsData
{
   int                   tid1;            /**< item id one */
   int                   tid2;            /**< item id two */
   CONSTYPE              type;               /**< stores whether the items have to be in the SAME or DIFFER packing */
   int                   npropagatedvars;    /**< number of variables that existed, the last time, the related node was
                                              *   propagated, used to determine whether the constraint should be
                                              *   repropagated*/
   int                   npropagations;      /**< stores the number propagations runs of this constraint */
   unsigned int          propagated:1;       /**< is constraint already propagated? */
   SCIP_NODE*            node;               /**< the node in the B&B-tree at which the cons is sticking */
};



/* create constraint data */
static 
SCIP_RETCODE consdataCreate(
    SCIP*             scip,
    SCIP_CONSDATA**    consdata,
    int               tid1,
    int               tid2,
    CONSTYPE          type,
    SCIP_NODE*        node)
{
    assert( scip != NULL);
    assert( consdata != NULL);
    assert( tid1 >= 0);
    assert( tid2 >= 0);
    assert( type == SAME || type == DIFFER);

    SCIP_CALL( SCIPallocBlockMemory(scip, consdata));

    (*consdata)->tid1 = tid1;
    (*consdata)->tid2 = tid2;
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
    SCIP*             scip,
    SCIP_CONSDATA*    consdata,
    FILE*             file
    )
{
    SCIPinfoMessage(scip, file, "%s(%d,%d) at node %d\n",
        consdata->type == SAME ? "same" : "diff",
        consdata->tid1, consdata->tid2, SCIPnodeGetNumber(consdata->node));

}

/** fixes a variable to zero if the corresponding packings are not valid for this constraint/node (due to branching) */
static 
SCIP_RETCODE checkVariable(
    SCIP*             scip,
    SCIP_CONSDATA*    consdata,
    SCIP_VAR*         var,
    int*              nfixedvars,
    SCIP_Bool*        cutoff)
{
    SCIP_VARDATA* vardata;
    int* testConsids;
    int nconsids;

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

    nconsids = SCIPvardataGetNConsids(vardata);
    testConsids = SCIPvardataGetConsids(vardata);

    existid1 = SCIPsortedvecFindInt(testConsids, consdata->tid1, nconsids, &pos);
    existid2 = SCIPsortedvecFindInt(testConsids, consdata->tid2, nconsids, &pos);
    type = consdata->type;

    if ( (type == SAME && existid1 != existid2) ||  (type == DIFFER && existid1 && existid2))
    {
        SCIP_CALL( SCIPfixVar(scip, var, 0.0, &infeasible, &fixed));

        if (infeasible)
        {
          assert( SCIPvarGetLbLocal(var) > 0.5);
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
    SCIP*                   scip,
    SCIP_CONSDATA*          consdata,
    SCIP_VAR**              vars,
    int                     nvars,
    SCIP_RESULT*            result)
{
    int nfixedvars;
    int v;
    SCIP_Bool cutoff;

    nfixedvars = 0;
    cutoff = FALSE;

    SCIPdebugMessage("check variables %d to %d\n", consdata->npropagatedvars, nvars);

    for (v=consdata->npropagatedvars; v < nvars && !cutoff; ++v)
    {
      SCIP_CALL(checkVariable(scip, consdata, vars[v], &nfixedvars, &cutoff));
    }

    SCIPdebugMessage("fixed %d variables locally\n", nfixedvars);

    if (cutoff)
      *result = SCIP_CUTOFF;
    else if (nfixedvars > 0)
      *result = SCIP_REDUCEDDOM;

    return SCIP_OKAY;
}

/** check if all variables are valid for the given consdata */
#ifndef NDEBUG
static 
SCIP_Bool consdataCheck(
    SCIP*                     scip,
    SCIP_PROBDATA*            probdata,
    SCIP_CONSDATA*            consdata,
    SCIP_Bool                 beforeprop
    )
{
    SCIP_VAR** vars;
    int nvars;

    SCIP_VARDATA* vardata;
    SCIP_VAR* var;

    int* testConsids;
    int nconsids;

    SCIP_Bool existid1;
    SCIP_Bool existid2;

    CONSTYPE type;

    int pos;
    int v;

    vars = SCIPprobdataGetVars(probdata);
    nvars = (beforeprop ? consdata->npropagatedvars : SCIPprobdataGetNVars(probdata) );
    assert(nvars <= SCIPprobdataGetNVars(probdata));

    for( v= 0; v<nvars; ++v)
    {
      var = vars[v];

      if (SCIPvarGetLbLocal(var) < 0.5)
        continue;

      vardata = SCIPvarGetData(var);

      testConsids = SCIPvardataGetConsids(vardata);
      nconsids = SCIPvardataGetNConsids(vardata);

      existid1 = SCIPsortedvecFindInt(testConsids, consdata->tid1, nconsids, &pos);
      existid2 = SCIPsortedvecFindInt(testConsids, consdata->tid2, nconsids, &pos);
      type = consdata->type;

      if( (type == SAME && existid1 != existid2) || (type == DIFFER && existid1 && existid2) )
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

/** frees samediff constraint data */
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
SCIP_DECL_CONSDELETE(consDeleteSamediff)
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
SCIP_DECL_CONSTRANS(consTransSamediff)
{  /*lint --e{715}*/
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
         sourcedata->tid1, sourcedata->tid2, sourcedata->type, sourcedata->node) );

   /* create target constraint */
   SCIP_CALL( SCIPcreateCons(scip, targetcons, SCIPconsGetName(sourcecons), conshdlr, targetdata,
         SCIPconsIsInitial(sourcecons), SCIPconsIsSeparated(sourcecons), SCIPconsIsEnforced(sourcecons),
         SCIPconsIsChecked(sourcecons), SCIPconsIsPropagated(sourcecons),
         SCIPconsIsLocal(sourcecons), SCIPconsIsModifiable(sourcecons),
         SCIPconsIsDynamic(sourcecons), SCIPconsIsRemovable(sourcecons), SCIPconsIsStickingAtNode(sourcecons)) );

   return SCIP_OKAY;
}

/** constraint enforcing method of constraint handler for LP solutions */
#define consEnfolpSamediff NULL

/** constraint enforcing method of constraint handler for pseudo solutions */
#define consEnfopsSamediff NULL

/** feasibility check method of constraint handler for integral solutions */
#define consCheckSamediff NULL

/** domain propagation method of constraint handler */
static
SCIP_DECL_CONSPROP(consPropSamediff)
{
    SCIP_PROBDATA* probdata;
    SCIP_CONSDATA* consdata;

    SCIP_VAR** vars;
    int nvars;
    int c;

    assert(scip != NULL);
    assert(strcmp(SCIPconshdlrGetName(conshdlr), CONSHDLR_NAME) == 0);
    assert(result != NULL);

    SCIPdebugMessage("propagation constraints of constraint handler <"CONSHDLR_NAME">\n");

    probdata = SCIPgetProbData(scip);
    assert(probdata != NULL);

    vars = SCIPprobdataGetVars(probdata);
    nvars = SCIPprobdataGetNVars(probdata);

    *result = SCIP_DIDNOTFIND;

    for (c=0; c<nconss; ++c)
    {
      consdata = SCIPconsGetData(conss[c]);
      #ifndef NDEBUG
      {
        SCIP_CONSDATA* consdata2;
        int i;

        for (i=c+1; i <nconss; i++)
        {
          consdata2 = SCIPconsGetData(cons[i]);
          assert(!(consdata->tid1 == consdata2->tid1 
              && consdata->tid2 == consdata2->tid2 
              && consdata->type == consdata2->type));
          assert(!(consdata->tid1 == consdata2->tid2 
              && consdata->tid2 == consdata2->tid1 
              && consdata->type == consdata2->type));

        }
      }
      #endif

      if (!consdata->propagated)
      {
        SCIPdebugMessage("propagate constraint <%s> ", SCIPconsGetName(conss[c]));

        SCIP_CALL( consdataFixVariables(scip, consdata, vars, nvars, result));
        consdata->npropagations++;

        if (*result != SCIP_CUTOFF)
        {
          consdata->propagated = TRUE;
          consdata->npropagatedvars = nvars;
        }
        else
          break;


      }
      assert( consdataCheck(scip, probdata, consdata, FALSE) );

    }

    return SCIP_OKAY;

}

/** variable rounding lock method of constraint handler */
#define consLockSamediff NULL

/** constraint activation notification method of constraint handler */
static
SCIP_DECL_CONSACTIVE(consActiveSamediff)
{  /*lint --e{715}*/
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
   // SCIPdebug( consdataPrint(scip, consdata, NULL) );

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
SCIP_DECL_CONSDEACTIVE(consDeactiveSamediff)
{  /*lint --e{715}*/
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
SCIP_DECL_CONSPRINT(consPrintSamediff)
{  /*lint --e{715}*/
   SCIP_CONSDATA*  consdata;

   consdata = SCIPconsGetData(cons);
   assert(consdata != NULL);

   // consdataPrint(scip, consdata, file);

   return SCIP_OKAY;
}

/** creates the handler for samediff constraints and includes it in SCIP */
SCIP_RETCODE SCIPincludeConshdlrSamediff(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_CONSHDLRDATA* conshdlrdata = NULL;
   SCIP_CONSHDLR* conshdlr = NULL;

   /* include constraint handler */
   SCIP_CALL( SCIPincludeConshdlrBasic(scip, &conshdlr, CONSHDLR_NAME, CONSHDLR_DESC,
         CONSHDLR_ENFOPRIORITY, CONSHDLR_CHECKPRIORITY, CONSHDLR_EAGERFREQ, CONSHDLR_NEEDSCONS,
         consEnfolpSamediff, consEnfopsSamediff, consCheckSamediff, consLockSamediff,
         conshdlrdata) );
   assert(conshdlr != NULL);

   SCIP_CALL( SCIPsetConshdlrDelete(scip, conshdlr, consDeleteSamediff) );
   SCIP_CALL( SCIPsetConshdlrTrans(scip, conshdlr, consTransSamediff) );
   SCIP_CALL( SCIPsetConshdlrProp(scip, conshdlr, consPropSamediff, CONSHDLR_PROPFREQ, CONSHDLR_DELAYPROP,
         CONSHDLR_PROP_TIMING) );
   SCIP_CALL( SCIPsetConshdlrActive(scip, conshdlr, consActiveSamediff) );
   SCIP_CALL( SCIPsetConshdlrDeactive(scip, conshdlr, consDeactiveSamediff) );
   SCIP_CALL( SCIPsetConshdlrPrint(scip, conshdlr, consPrintSamediff) );

   return SCIP_OKAY;
}

/** creates and captures a samediff constraint */
SCIP_RETCODE SCIPcreateConsSamediff(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   tid1,            /**< item id one */
   int                   tid2,            /**< item id two */
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
      SCIPerrorMessage("samediff constraint handler not found\n");
      return SCIP_PLUGINNOTFOUND;
   }

   /* create the constraint specific data */
   SCIP_CALL( consdataCreate(scip, &consdata, tid1, tid2, type, node) );

   /* create constraint */
   SCIP_CALL( SCIPcreateCons(scip, cons, name, conshdlr, consdata, FALSE, FALSE, FALSE, FALSE, TRUE,
         local, FALSE, FALSE, FALSE, TRUE) );

   SCIPdebugMessage("created constraint: ");
   // SCIPdebug( consdataPrint(scip, consdata, NULL) );

   return SCIP_OKAY;
}


int SCIPgetTid1Samediff(
	SCIP*			scip,
	SCIP_CONS*		cons)
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->tid1;
}

int SCIPgetTid2Samediff(
	SCIP*			scip,
	SCIP_CONS*		cons)
{
	SCIP_CONSDATA* consdata;

	assert(cons != NULL);

	consdata = SCIPconsGetData(cons);
	assert(consdata != NULL);

	return consdata->tid2;
}

/** return constraint type SAME or DIFFER */
CONSTYPE SCIPgetTypeSamediff(
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
