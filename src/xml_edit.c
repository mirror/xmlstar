/* $Id: xml_edit.c,v 1.2 2002/11/16 03:39:46 mgrouch Exp $ */


/*
   TODO:

   xml ed { <action> }


   where <action>

   
   -d or --delete <xpath>  

   -i or --insert <xpath> -t (--type) elem|text|attr -v (--value) <value>
   
   -a or --append <xpath> -t (--type) elem|text|attr -v (--value) <value>

   -s or --subnode <xpath> -t (--type) elem|text|attr -v (--value) <value>

   -m or --move   <xpath1> <xpath2>

   -u or --update <xpath> -v (--value) <value>

                          -x (--expr) <xpath>

   -r or --rename <xpath1> -v <new-name>


   How to increment value of every attribute @weight?
   How in --update refer to current value?
   How to insert from a file?

   xml ed --update "//*@weight" -x "./@weight+1"?
   
*/

int xml_edit(int argc, char **argv)
{
    printf("EDIT\n");
    return 0;
}  
