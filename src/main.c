#include <stdio.h>

#include "scip/scip.h"
#include "scip/scipshell.h"
#include "scip/scipdefplugins.h"


static 
SCIP_RETCODE runShell(
	int			argc,
	char** 		argv,
	const char*	defaultsetname)
{
	SCIP* scip = NULL;

	/* initialize scip */
	SCIP_CALL( SCIPcreate(&scip));

	SCIPenableDebugSol(scip);

	/* include tp3s reader */
	SCIP_CALL( SCIPincludeReaderTP3S(scip));

	/*include tp3s branching and branching data */

	/* include tp3s pricer */

	/* include default plugins */
	SCIP_CALL(SCIPincludeDefaultPlugins(scip));

	/* disable restarts */
	SCIP_CALL( SCIPsetIntParam(scip, "presolving/maxrestarts", 0));

	/* turn off all separation algorithms */
	SCIP_CALL( SCIPsetSeparating(scip, SCIP_PARAMSETTING_OFF, TRUE));

	/**********************************
    * Process command line arguments *
    **********************************/
   	SCIP_CALL( SCIPprocessShellArguments(scip, argc, argv, defaultsetname) );

   /********************
    * Deinitialization *
    ********************/

   	SCIP_CALL( SCIPfree(&scip) );

   	BMScheckEmptyMemory();

   	return SCIP_OKAY;
}




int main(
	int		argc,
	char**	argv)
{
	SCIP_RETCODE retcode;

	retcode = runShell(argc, argv, "scip.set");
	if (retcode != SCIP_OKAY)
	{
		SCIPprintError(retcode);
		return -1;
	}

	return 0;
}