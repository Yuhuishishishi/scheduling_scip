#ifndef _SCIP_CONS_TESTONVEHICLE_H
#define _SCIP_CONS_TESTONVEHICLE_H 

#include "scip/scip.h"

enum ConsType 
{
	ENFORCE = 1,
	FORBID = 0
}
typedef enum ConsType CONSTYPE;

/** creates the handler for element constraints and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeConshdlrTestOnVehicle(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** creates and captures a testOnVehicle constraint */
extern
SCIP_RETCODE SCIPcreateConsTestOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   tid,	             /**< item id one */
   int 					    vid,				 /**< vehicle id */
   CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   );

/** returns test id */
extern
int SCIPgetTidTestOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );

/** returns vehicle id */
extern
int SCIPgetVidTestOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );


/** return constraint type ALLOW or FORBID */
extern
CONSTYPE SCIPgetTypeTestOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );
#endif