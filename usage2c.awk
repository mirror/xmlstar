#!/usr/bin/awk -f

BEGIN {
    print("#include <stdio.h>");
    print("#include <libxml/xmlversion.h>");
}

length(command_name) == 0 {
    command_name = FILENAME;
    sub(/\.txt$/, "", command_name);
    sub(/^([^\/]+\/)*/, "", command_name);
    gsub(/-/, "_", command_name);
    printf("static const char %s[] = {\n", command_name);
    progs = 0;
}

/PROG/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "PROG") {
            progs++;
            $i = "%s";
        }
    }
}

/^#/
!/^#/ {
    gsub(/./, "'&',");
    gsub(/'\\'/, "'\\\\'");     # '\' --> '\\'
    gsub(/'''/, "'\\''");       # ''' --> '\''
    print($0 "'\\n',");
}

END {
    print("0 };");
    printf("void fprint_%s(FILE* out, const char* argv0) {\n", command_name);
    printf("  fprintf(out, %s", command_name);
    for (i = 1; i <= progs; i++)
        printf(", argv0");
    print(");\n}");
}

