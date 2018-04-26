function byLength(i1,v1,i2,v2) {
    return length(i2)-length(i1)
}

BEGIN {
    num_text_lines = 0
    num_data_bytes = 0
    ds_align       = -1

    read_data = 0
    read_text = 0

    error = 0
}

function data_align(x) {
    while (num_data_bytes % x) {
        data_append_byte(-1)
    }
    if (ds_align==-1) {
        ds_align = x
    }
}

function data_append_byte(x) {
    data_align(1)   # set ds_align to 1 if not previously set by '.align'
    data[num_data_bytes++] = sprintf("%02X", and(x, 0xFF))
}

function data_append_word(x) {
    data_append_byte(and(rshift(x, 8), 0xFF))
    data_append_byte(and(rshift(x, 0), 0xFF))
}

function data_append_long(x) {
    data_append_byte(and(rshift(x, 24), 0xFF))
    data_append_byte(and(rshift(x, 16), 0xFF))
    data_append_byte(and(rshift(x,  8), 0xFF))
    data_append_byte(and(rshift(x,  0), 0xFF))
}

function data_append_quad(x) {
    data_append_byte(and(rshift(x, 56), 0xFF))
    data_append_byte(and(rshift(x, 48), 0xFF))
    data_append_byte(and(rshift(x, 40), 0xFF))
    data_append_byte(and(rshift(x, 32), 0xFF))
    data_append_byte(and(rshift(x, 24), 0xFF))
    data_append_byte(and(rshift(x, 16), 0xFF))
    data_append_byte(and(rshift(x,  8), 0xFF))
    data_append_byte(and(rshift(x,  0), 0xFF))
}

/\s*#.*$/ {
    gsub("\\s*#.*$", "", $0)
}

/^\s*$/ {
    next;
}

/^\s*\.data/ {
    read_data = 1
    read_text = 0
    next
}

/^\s*\.text/ {
    read_data = 0
    read_text = 1
    next
}

/^\s*\.align/ {
    data_align(strtonum($2))
}

/^\s*\.byte/ {
    if (read_data) {
        if (NF==0) {
            data_append_byte(0)
        } else {
            data_append_byte(strtonum($2))
        }
    } else {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    next
}

/^\s*\.word/ {
    if (read_data) {
        if (NF==0) {
            data_append_word(0)
        } else {
            data_append_word(strtonum($2))
        }
    } else {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    next
}

/^\s*\.long/ {
    if (read_data) {
        if (NF==0) {
            data_append_long(0)
        } else {
            data_append_long(strtonum($2))
        }
    } else {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    next
}

/^\s*\.quad/ {
    if (read_data) {
        if (NF==0) {
            data_append_quad(0)
        } else {
            data_append_quad(strtonum($2))
        }
    } else {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    next
}

/^\s*[[:alpha:]][[:alnum:]]*:/ {
    gsub("[. :]","",$0)
    if (read_text) {
        if (text_label[$0]) {
            print "error in line " NR ": redefinition of label " $0
            error = 1
            exit 1
        }
        text_label[$0] = num_text_lines*4
    }
    if (read_data) {
        data_label[$0] = num_data_bytes
    }
    next
}

read_text {
    text[num_text_lines++] = $0
    next
}

END {
    if (!error) {
#       print "-- TEXT_SECTION --"
        ts = 4*num_text_lines

        data_align(1)
        ds = ds_align*int((ts + ds_align-1)/ds_align)
        padding = ds - ts

        for (i=0; i<num_text_lines; ++i) {
            instr =  gensub(/(^\s*[a-z]*\s*)(.*)$/, "\\1", "g", text[i])
            param =  gensub(/(^\s*[a-z]*\s*)(.*)$/, "\\2", "g", text[i])
            n=asorti(text_label, s, "byLength")
            for (k=1; k<=n; ++k) {
                addr_abs = sprintf("0x%02X", text_label[s[k]])
#                addr_rel = sprintf("0x%04X", (text_label[s[k]]-4*i)/4)
#                addr_abs = text_label[s[k]]
                addr_rel = (text_label[s[k]]-4*i)/4
                gsub("[$]" s[k], "$" addr_abs, param)
                gsub("(^|\\s)" s[k], "$" addr_rel, param)
            }
            n=asorti(data_label, s, "byLength")
            for (k=1; k<=n; ++k) {
                addr_abs = sprintf("0x%02X", data_label[s[k]]+ds)
                #addr_rel = sprintf("0x%04X", data_label[s[k]]+ds-4*i)
                gsub("[$]" s[k], "$" addr_abs, param)
                #gsub(s[k], addr_rel, param)
            }
            print instr param
        }

#       print "-- DATA_SECTION --"
        if (padding) {
            print "FF FF FF FF    #=" sprintf("%04X", ds-padding)
        }
        for (i=0; i<num_data_bytes; ++i) {
            printf("%-3s", data[i])
            if (i % 4==3) {
                print "   #-" sprintf("%04X", ds+i-3)
            }
        }
    }
}
