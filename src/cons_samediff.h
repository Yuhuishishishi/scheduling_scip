##ifndef _SCIP_CONS_SAMEDIFF_H
#define _SCIP_CONS_SAMEDIFF_H 

#include "scip/scip.h"

enum ConsType
{
	SAME = 1,
	DIFFER = 0
};
typedef enum ConsType CONSTYPE;

/** creates the handler for element constraints and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeConshdlrSamediff(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** creates and captures a samediff constraint */
extern
SCIP_RETCODE SCIPcreateConsSamediff(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   tid1,            /**< item id one */
   int                   tid2,            /**< item id two */
   CONSTYPE              type,               /**< stores whether the items have to be in the SAME or DIFFER packing */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   );

/** returns item id one */
extern
int SCIPgetTid1Samediff(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );

/** returns item id two */
extern
int SCIPgetTid2Samediff(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );

/** return constraint type SAME or DIFFER */
extern
CONSTYPE SCIPgetTypeSamediff(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< samediff constraint */
   );



#endif