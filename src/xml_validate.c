/*  $Id: xml_validate.c,v 1.7 2002/12/05 22:19:33 mgrouch Exp $  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

/*
 *   TODO: Use cases
 *   1. find malfomed XML documents in a given set of XML files 
 *   2. find XML documents which do not match DTD/XSD in a given set of XML files
 */

static const char validate_usage_str[] =
"XMLStarlet Toolkit: Validate XML document(s)\n"
"Usage: xml val <options> [ <xml-file> ... ]\n"
"where <options>\n"
"   -d or --dtd <dtd-file>  - validate against DTD\n"
"   -s or --xsd <xsd-file>  - validate against schema\n"
"   -n or --line-num        - print line numbers for validation errors\n"
"   -x or --xml-out         - print result as xml\n"
"   -w or --well-formed     - check only if XML is well-formed\n\n";

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, validate_usage_str);
    fprintf(o, more_info);
    exit(1);
}


/**
 *  This is the main function for 'validate' option
 */
int
valMain(int argc, char **argv)
{
    valUsage(argc, argv);
    return 0;
}  
