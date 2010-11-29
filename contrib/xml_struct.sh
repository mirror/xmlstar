#!/bin/sh


struct() {
    "${xml:-xml}" sel \
        "$@" \
        --text \
        --var empty="''" \
        --var dot="'.'" \
        --var plus="'+'" \
    -t \
        --var nl -n -b \
        --var PI   -o 'PI  ' -b \
        --var comm -o 'Comm' -b \
        --var text -o 'Text' -b \
        --var NS   -o 'NS  ' -b \
        --var attr -o 'Attr' -b \
        --var elem -o 'Elem' -b \
        --var root -o 'Root' -b \
        --var NA   -o 'N/A ' -b \
    -m '/|//node()|//@*' \
        --var path \
            -o '/' \
            -m 'ancestor-or-self::*' \
            -i 'position() > 1' -o '/' -b \
            -v 'name()' \
            -b \
        -b \
        --var indent \
            -m 'ancestor-or-self::*' -v '$plus' -b \
            -m 'ancestor-or-self::*' -v 'concat($dot, $dot, $dot)' -b \
        -b \
        --var type \
            --var ns='../namespace::*' \
            --choose \
                --when './self::processing-instruction()' -v '$PI'   -b \
                --when 'count(.|$ns) = count($ns)'        -v '$NS'   -b \
                --when './self::comment()'                -v '$comm' -b \
                --when './self::text()'                   -v '$text' -b \
                --when 'count(.|../@*) = count(../@*)'    -v '$attr' -b \
                --when './self::*'                        -v '$elem' -b \
                --when 'not(./parent::*)'                 -v '$root' -b \
                --otherwise -v '$NA' -b \
            -b \
        -b \
        --choose \
            --when '$type = $PI' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v 'name()' -o '][' -v '.' \
                -o ']' -n \
            -b \
            --when '$type = $comm' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v '.' -o ']' -n \
            -b \
            --when '$type = $text' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v '$path' -o '][' -v 'str:replace(., $nl, &quot;\n&quot;)' -o ']' -n \
            -b \
            --when '$type = $attr' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v '$path' -o '][@' -v 'name()' -o '][' -v '.' -o ']' -n \
            -b \
            --when '$type = $root' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v '$path' -o ']' -n \
            -b \
            --when '$type = $elem' \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -v '$path' -o ']' \
                -n \
                --var nslist \
                    -m 'namespace::*[name() != &quot;xml&quot;]' \
                    -s 'A:T:U' 'name()' \
                        -i 'position() > 1' -o ', ' -b \
                        --choose \
                            --when 'name() = $empty' -o '<def>' -b \
                            --otherwise -v 'name()' -b \
                        -b \
                        -o '=' -v '.' \
                    -b \
                -b \
                -o 'NS  ' -o ': ' -v '$indent' \
                -o '[' -v '$path' -o '][' -v '$nslist' -o ']' -n \
            -b \
            --otherwise \
                -v '$type' -o ': ' -v '$indent' \
                -o '[' -c '.' -o ']' -n \
            -b \
        -b \
    -b
}


struct "$@"
