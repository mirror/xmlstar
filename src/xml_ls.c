/*  $Id: xml_ls.c,v 1.17 2005/03/19 01:18:02 mgrouch Exp $  */

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002-2004 Mikhail Grushinskiy.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libxml/xmlmemory.h>
#include <libxml/c14n.h>

#include "xmlstar.h"
#include "escape.h"

static const char ls_usage_str[] =
"XMLStarlet Toolkit: List directory as XML\n"
"Usage: %s ls\n"
"Lists current directory in XML format.\n"
"Time is shown per ISO 8601 spec.\n"
"\n";

/**
 *  Print small help for command line options
 */
void
lsUsage(int argc, char **argv, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, ls_usage_str, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}


char *
get_file_type(mode_t mode)
{
   switch(mode & S_IFMT)
   {
       case S_IFREG:
            return("f"); /* regular file */
       case S_IFDIR:
            return("d"); /* directory */
       case S_IFCHR:
            return("c"); /* character device */
       case S_IFBLK:
            return("b"); /* block device */
#ifdef S_IFLNK
       case S_IFLNK:
            return("l"); /* symbolic link */
#endif
       case S_IFIFO:
            return("p"); /* fifo */
#ifdef S_IFSOCK
       case S_IFSOCK:
            return("s"); /* socket */
#endif
       default:
            return("u"); /* unknown */
   }
   return(NULL);
}

char *
get_file_perms(mode_t mode)
{
   int i;
   char *p;
   static char perms[10];

   p = perms;
   strcpy(perms, "---------");

   for(i=0; i<3; i++)
   {
       if(mode &(S_IREAD>>(i*3)))
           *p='r';
       ++p;

       if(mode &(S_IWRITE>>(i*3)))
           *p='w';
       ++p;

       if(mode &(S_IEXEC>>(i*3)))
           *p='x';
       ++p;
   }

#ifdef S_ISUID
   if((mode & S_ISUID))
       perms[2] = 's';
#endif

#ifdef S_ISGID
   if((mode & S_ISGID))
       perms[5] = 's';
#endif

#ifdef S_ISVTX
   if((mode & S_ISVTX))
       perms[8] = 't';
#endif

   return(perms);
}

int
xml_print_dir(char* dir)
{
   DIR *dirp;
   struct dirent *d;
   struct stat stats;
   int i = 0, num_files = 0;

   if((dirp = opendir(dir)) == NULL)
      return(-1);
   
   while((d = readdir(dirp)) != NULL)
   {
      xmlChar *xml_str = NULL;
      char *type = NULL;
      char *perm = NULL;
      char  last_acc[20];
      char  last_mod[20];
      char  sz[16];
      int   sz_len;
      char  sp[16];
      int   k;

      if ((d->d_name == NULL) || !strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
          continue;
      
#if defined (__MINGW32__)
      if(stat(d->d_name, &stats) != 0)
#else
      if(lstat(d->d_name, &stats) != 0)
#endif
      {
        fprintf(stderr, "couldn't stat: %s\n", d->d_name);
      }

      type = get_file_type(stats.st_mode);
      perm = get_file_perms(stats.st_mode);

#if defined (__MINGW32__)
      /* somehow atime is -1 on Windows XP when the atime is in future */
      if (stats.st_atime < 0) stats.st_atime = 0; 
      /* somehow mtime is -1 on Windows XP when the mtime is in future */
      if (stats.st_mtime < 0) stats.st_mtime = 0; 
#endif

      /* format time as per ISO 8601 */
      strftime(last_acc, 20, "%Y%m%dT%H%M%SZ", gmtime(&stats.st_atime));
      strftime(last_mod, 20, "%Y%m%dT%H%M%SZ", gmtime(&stats.st_mtime));
      sz[15] = '\0';
      sz_len = snprintf(sz, 15, "%lu", (unsigned long) stats.st_size);
      for(k=0; k<(16-sz_len); k++) sp[k] = ' ';
      sp[16-sz_len] = '\0';

      xml_str = xml_C11NNormalizeAttr((const xmlChar *) d->d_name);      
      printf("<%s p=\"%s\" a=\"%s\" m=\"%s\" s=\"%s\"%s n=\"%s\"/>\n",
              type, perm, last_acc, last_mod, sz, sp, xml_str);
      i++;
      xmlFree(xml_str);

   } /* end of for loop */

   closedir(dirp);
   num_files = i;
   return num_files;
}

int
lsMain(int argc, char** argv)
{
    int res = -1;
    if (argc != 2) lsUsage(argc, argv, EXIT_BAD_ARGS);
    printf("<dir>\n");
    res = xml_print_dir(".");
    printf("</dir>\n");
    return res;
}

