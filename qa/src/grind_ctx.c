/*
 * Repeatedly call pmNewContext ... then pmDestroyContext ...
 *
 * Looking for memory leaks, malloc botches etc, especially in the
 * presence of derived metrics.
 *
 * Copyright (c) 1997-2001 Silicon Graphics, Inc.  All Rights Reserved.
 * (largely borrowed from chkctx2.c)
 */

#define SOURCE handle == 0 ? "host" : ( type == PM_CONTEXT_ARCHIVE ? "archive" : "host" )
#define HOST handle == 0 ? "local:" : host

#include <pcp/pmapi.h>

#define NUMCTX 10

static char	*dm_config = NULL;	/* per-context derived metrics */

extern int add_ctx_dm(char *);

static void
grind(int type, char *host)
{
    int		ctx[NUMCTX];
    int		i;
    int		sts;

    for (i = 0; i < NUMCTX; i++) {
	if (type == PM_CONTEXT_HOST) {
	    if ((sts = pmNewContext(PM_CONTEXT_HOST, host)) < 0) {
		fprintf(stderr, "pmNewContext(host=%s): %s\n", host, pmErrStr(sts));
		exit(1);
	    }
	}
	else if (type == PM_CONTEXT_LOCAL) {
	    if ((sts = pmNewContext(PM_CONTEXT_LOCAL, host)) < 0) {
		fprintf(stderr, "pmNewContext(local): %s\n", pmErrStr(sts));
		exit(1);
	    }
	}
	else {
	    if ((sts = pmNewContext(PM_CONTEXT_ARCHIVE, host)) < 0) {
		fprintf(stderr, "pmNewContext(archive=%s): %s\n", host, pmErrStr(sts));
		exit(1);
	    }
	}
	ctx[i] = sts;
	if (dm_config != NULL) {
	    if (add_ctx_dm(dm_config) < 0) {
		/* fatal error reported earlier */
		exit(1);
	    }
	}
    }

    for (i = NUMCTX-1; i >=0; i--) {
	if ((sts = pmDestroyContext(ctx[i])) < 0) {
	    fprintf(stderr, "pmDestroyContext(%d): %s\n", ctx[i], pmErrStr(sts));
	}
    }
}

int
main(int argc, char **argv)
{
    int		c;
    int		sts;
    int		errflag = 0;
    int		type = 0;
    int		iter = 5;
    char	*host = "local:";
    char	*endnum;
    static char	*usage = "[-a archive] [-c dmfile] [-C dmfile] [-D debugspec] [-h hostname] [-L] [-n namespace] [-s iterations]";

    pmSetProgname(argv[0]);

    while ((c = getopt(argc, argv, "a:c:C:D:h:Ln:s:")) != EOF) {
	switch (c) {

	case 'a':	/* archive name */
	    if (type != 0) {
		fprintf(stderr, "%s: at most one of -a, -h and -L allowed\n", pmGetProgname());
		errflag++;
	    }
	    type = PM_CONTEXT_ARCHIVE;
	    host = optarg;
	    break;

	case 'c':	/* global derived metrics config file */
	    sts = pmLoadDerivedConfig(optarg);
	    if (sts < 0) {
		fprintf(stderr, "%s: -c error: %s\n", pmGetProgname(), pmErrStr(sts));
		exit(1);
	    }
	    break;

	case 'C':	/* per-context derived metrics config file */
	    dm_config = optarg;
	    break;

	case 'D':	/* debug options */
	    sts = pmSetDebug(optarg);
	    if (sts < 0) {
		fprintf(stderr, "%s: unrecognized debug options specification (%s)\n",
		    pmGetProgname(), optarg);
		errflag++;
	    }
	    break;

	case 'h':	/* hostname for PMCD to contact */
	    if (type != 0) {
		fprintf(stderr, "%s: at most one of -a, -h and -L allowed\n", pmGetProgname());
		errflag++;
	    }
	    host = optarg;
	    type = PM_CONTEXT_HOST;
	    break;

	case 'L':	/* local mode, no PMCD */
	    if (type != 0) {
		fprintf(stderr, "%s: at most one of -a, -h and -L allowed\n", pmGetProgname());
		errflag++;
	    }
	    host = NULL;
	    type = PM_CONTEXT_LOCAL;
	    break;

	case 'n':	/* alternative name space file */
	    if ((sts = pmLoadASCIINameSpace(optarg, 1)) < 0) {
		fprintf(stderr, "%s: cannot load namespace from \"%s\": %s\n", pmGetProgname(), optarg, pmErrStr(sts));
		exit(1);
	    }
	    break;

	case 's':	/* iterations */
	    iter = (int)strtol(optarg, &endnum, 10);
	    if (*endnum != '\0' || iter < 0) {
		fprintf(stderr, "%s: -s requires positive numeric argument\n", pmGetProgname());
		errflag++;
	    }
	    break;

	case '?':
	default:
	    errflag++;
	    break;
	}
    }

    if (errflag) {
	fprintf(stderr, "Usage: %s %s\n", pmGetProgname(), usage);
	exit(1);
    }

    if (type == 0)
	type = PM_CONTEXT_HOST;		/* default */

    while (iter-- > 0) {
	printf("Iteration %d\n", iter);
	grind(type, host);
    }

    exit(0);
}
