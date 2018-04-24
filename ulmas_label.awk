function byLength(i1,v1,i2,v2) {
    return length(i2)-length(i1)
}

BEGIN {
    num_text_lines = 0
    num_data_lines = 0

    read_data = 0
    read_text = 0

    error = 0
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

/^\s*\.quad/ {
    if (read_data) {
        if (NF==0) {
            data[num_data_lines++] = "00 00 00 00"
            data[num_data_lines++] = "00 00 00 00"
        } else {
            $2 = strtonum($2)
            b0 = sprintf("%02X", and(rshift($2, 0), 0xFF))
            b1 = sprintf("%02X", and(rshift($2, 8), 0xFF))
            b2 = sprintf("%02X", and(rshift($2,16), 0xFF))
            b3 = sprintf("%02X", and(rshift($2,24), 0xFF))
            b4 = sprintf("%02X", and(rshift($2,32), 0xFF))
            b5 = sprintf("%02X", and(rshift($2,40), 0xFF))
            b6 = sprintf("%02X", and(rshift($2,48), 0xFF))
            b7 = sprintf("%02X", and(rshift($2,56), 0xFF))

            data[num_data_lines++] = b7 " " b6 " " b5 " " b4
            data[num_data_lines++] = b3 " " b2 " " b1 " " b0
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
        data_label[$0] = num_data_lines*4
    }
    next
}

read_data {
    data[num_data_lines++] = $0
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
        ds = ts + (ts % 8);
        padding = (ts==ds) ? 0 : 4

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
                gsub(s[k], "$" addr_rel, param)
            }
            n=asorti(data_label, s, "byLength")
            for (k=1; k<=n; ++k) {
                addr_abs = sprintf("0x%04X", data_label[s[k]]+ds)
                addr_rel = sprintf("0x%04X", data_label[s[k]]+ds-4*i)
                gsub("[$]" s[k], "$" addr_abs, param)
                gsub(s[k], addr_rel, param)
            }
            print instr param
        }


#       print "-- DATA_SECTION --"
        if (padding) {
            print "FF FF FF FF    #=" sprintf("%04X", ds-padding) " :  (padding)"
        }
        for (i=0; i<num_data_lines; ++i) {
            print  data[i]   "    #-" sprintf("%04X", ds+i*4)
        }
    }
}
