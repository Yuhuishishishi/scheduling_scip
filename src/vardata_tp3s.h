#ifndef _SCIP_VARDATA_TP3S_
#define _SCIP_VARDATA_TP3S_ 

#include "scip/scip.h"

extern 
SCIP_RETCODE SCIPvardataCreateTP3S(
	SCIP*			scip,
	SCIP_VARDATA**	vardata,
	int*			testConsids,
	int				nconsids,
	int				vehicleConsids
	);

/** get number of constraints */
extern
int SCIPvardataGetNConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   );

/** returns sorted constraint id array */
extern
int* SCIPvardataGetConsids(
   SCIP_VARDATA*         vardata             /**< variable data */
   );

/** return vehicle constraint id */
extern
int SCIPvardataGetVehicleConsids(
   SCIP_VARDATA*         vardata
   );


/** creates variable */
extern
SCIP_RETCODE SCIPcreateVarTP3S(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR**            var,                /**< pointer to variable object */
   const char*           name,               /**< name of variable, or NULL for automatic name creation */
   SCIP_Real             obj,                /**< objective function value */
   SCIP_Bool             initial,            /**< should var's column be present in the initial root LP? */
   SCIP_Bool             removable,          /**< is var's column removable from the LP (due to aging or cleanup)? */
   SCIP_VARDATA*         vardata             /**< user data for this specific variable */
   );


/** prints vardata to file stream */
extern
void SCIPvardataPrint(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VARDATA*         vardata,            /**< variable data */
   FILE*                 file                /**< the text file to store the information into */
   );

#endif