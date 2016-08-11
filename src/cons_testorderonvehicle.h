#ifndef _SCIP_CONS_TESTORDERONVEHICLE_H_
#define _SCIP_CONS_TESTORDERONVEHICLE_H_ 

#include "scip/scip.h"

enum ConsType 
{
	ALLOW = 1,
	FORBID = 0
}
typedef enum ConsType CONSTYPE;

/** creates the handler for element constraints and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeConshdlrTestOrderOnVehicle(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** creates and captures a testOnVehicle constraint */
extern
SCIP_RETCODE SCIPcreateConsTestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   tid1,	               /**< item id one */
   int                   tid2,
   int 					    vid,				         /**< vehicle id */
   CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   );

/** returns test id 1 */
extern
int SCIPgetTid1TestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );

/** returns test id 2 */
extern
int SCIPgetTid2TestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );

/** returns vehicle id */
extern
int SCIPgetVidTestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );


/** return constraint type ALLOW or FORBID */
extern
CONSTYPE SCIPgetTypeTestOrderOnVehicle(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );
#endif