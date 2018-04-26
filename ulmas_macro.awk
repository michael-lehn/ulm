BEGIN {
    record_macro = 0
}

/#/ {
    gsub(/#.*$/, "", $0)
}

/^\s*$/ {
    next
}

/^\s*#/ {
    next
}

$1 == ".equ" {
    gsub(/,/, "", $2)
    equ[$2] = $3
    next
}

$1 == ".macro" {
    macro_name = $2
    macro[macro_name]["num_args"] = NF - 2
    macro[macro_name]["num_lines"] = 0
    for (i=3; i<=NF; ++i) {
        gsub(/[, ]*$/, "", $i)
        macro[macro_name]["name_arg"][i-2] = $i
    }
    record_macro = 1
    next
}

$1 == ".endm" {
    record_macro = 0
    next
}



(record_macro == 1) {
    macro[macro_name][++macro[macro_name]["num_lines"]] = $0
    next
}

function format(line)
{
    print line
}

function expand_equ(line)
{
    found = 0
    do {
       for (key in equ) {
            found = gsub(key, equ[key], line)
        }
    } while (found);
    return line
}

function expand_macro(line,     found,key,i,k)
{
    found = 0
    line = expand_equ(line)
    for (key in macro) {
        if (gsub("^\\s*" key, "", line)) {
            split(line, args, ",")
            for (i=1; i<=macro[key]["num_lines"]; ++i) {
                mline = macro[key][i]
                for (k=1; k<=macro[key]["num_args"]; ++k) {
                    pattern = "\\\\" macro[key]["name_arg"][k]
                    gsub(/[ ]*/, "", args[k])
                    gsub(pattern, args[k], mline)
                }
                expand_macro(mline)
            }
            found = 1
        }
    }
    if (!found) {
        format(line)
    }
}

{
    expand_macro($0)
}
