function byLength(i1,v1,i2,v2) {
    return length(i2)-length(i1)
}

BEGIN {
    num_instr      = 0
    num_data_bytes = 0

    read_data      = 0
    read_text      = 0

    align_data     = 1
    globl          = 0

    error = 0

    _ord_init()
}

function _ord_init(low, high, i, t)
{
    low = sprintf("%c", 7) # BEL is ascii 7
    if (low == "\a") {    # regular ascii
        low = 0
        high = 127
    } else if (sprintf("%c", 128 + 7) == "\a") {
        # ascii, mark parity
        low = 128
        high = 255
    } else {        # ebcdic(!)
        low = 0
        high = 255
    }

    for (i = low; i <= high; i++) {
        t = sprintf("%c", i)
        _ord_[t] = i
    }
}

function ord(str, pos)
{
    c = substr(str, pos, 1)
    return _ord_[c]
}

function data_write_byte(value) {
    data[num_data_bytes++] = sprintf("%02X", and(value, 0xFF))
}

function data_align(align) {
    while (num_data_bytes % align) {
        data_write_byte(255)
    }
}

function data_add(value, size,      i) {
    for (i in open_data_label) {
        data_label[i] = num_data_bytes
        if (open_data_label[i]) {
            old = data_globl[num_data_bytes]
            data_globl[num_data_bytes] = i " " old
        }
        delete open_data_label[i]
    }

    while (--size >= 0) {
        data_write_byte(rshift(value, 8*size))
    }
    align_data = 1
}

function bss_add(size, align) {
    if (num_bss_bytes % align) {
        print "bss align: ", align - num_bss_bytes % align
        num_bss_bytes += align - num_bss_bytes % align
    }
    for (i in open_data_label) {
        bss_label[i]  = num_bss_bytes
        if (open_data_label[i]) {
            old = data_globl[num_bss_bytes]
            data_globl[num_bss_bytes] = i " " old
        }
        delete open_data_label[i]
    }
    num_bss_bytes += size
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

/^\s*\.comm/ {
    bss_add($2, $3)
}

/^\s*\.ascii/ {
    if (! read_data) {
        print "error in line " NR ": .ascii not in data section"
        error = 1
        exit 1
    }
    if (NF==0) {
        print "expected init value"
        error = 1
        exit 1
    }
    gsub("\"", "", $2)
    for (i=0; i<length($2); ++i) {
        data_add(ord($2, i+1), 1)
    }
    next
}


/^\s*\.byte/ {
    if (! read_data) {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    if (NF==0) {
        print "expected init value"
        error = 1
        exit 1
    }
    data_add(strtonum($2), 1)
    next
}

/^\s*\.word/ {
    if (! read_data) {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    if (NF==0) {
        print "expected init value"
        error = 1
        exit 1
    }
    data_add(strtonum($2), 2)
    next
}

/^\s*\.long/ {
    if (! read_data) {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    if (NF==0) {
        print "expected init value"
        error = 1
        exit 1
    }
    data_add(strtonum($2), 4)
    next
}

/^\s*\.quad/ {
    if (! read_data) {
        print "error in line " NR ": .quad not in data section"
        error = 1
        exit 1
    }
    if (NF==0) {
        print "expected init value"
        error = 1
        exit 1
    }
    data_add(strtonum($2), 8)
    next
}

/^\s*\.globl/ {
    globl = 1
    next
}

/^\s*[[:alpha:]]([[:alnum:]]|[_])*:/ {
    gsub("[. :]","",$0)
    if (read_text) {
        if (text_label[$0]) {
            print "error in line " NR ": redefinition of label " $0
            error = 1
            exit 1
        }
        text_label[$0]        = 4*num_instr
        if (globl) {
            old = text_globl[num_instr]
            text_globl[num_instr] = $0 " " old
        }
    }
    if (read_data) {
        if (data_label[$0]) {
            print "error in line " NR ": redefinition of label " $0
            error = 1
            exit 1
        }
        open_data_label[$0] = globl

    }
    if (globl) {
        globl = 0
    }
    next
}

read_text {
    text[num_instr++] = $0
    next
}

function write_data_init(start_addr,     i) {
    for (i=0; i<4; ++i) {
        data_rec[i] = ""
    }
    data_addr = start_addr
    data_set  = 0
}

function write_data_set(ds_offset, str) {

    data_rec[ds_offset % 4] = sprintf("%-3s", str)
    data_set  = 1

}

function write_data_flush(with_addr,        i) {
    if (! data_set) {
        return
    }

    printf("    ")
    for (i=0; i<4; ++i) {
        printf("%-3s", data_rec[i])
    }
    if (data_rec[0] != "") {
        addr = data_addr + ds_offset
        addr = addr - addr % 4
        printf data_line sprintf("   #- %04X", addr)
    }
    print ""
    data_set = 0
}

END {
    if (!error) {
        if (num_instr % 2) {
            text[num_instr++] = "    nop"
        }

        ds     = num_instr*4
        for (i in data_label) {
            data_label[i] += ds
        }

        bss   = ds + num_data_bytes
        bss  += (bss % 8) ?  8 - bss % 8 : 0
        for (i in bss_label) {
            bss_label[i] += bss
            data_label[i] = bss_label[i]
        }

        print "    #-- TEXT_SECTION --------"

        for (i=0; i<num_instr; ++i) {
            if (i in text_globl) {
                split(text_globl[i], label, " ")
                for (l in label) {
                    print label[l], ":"
                }
            }
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
                addr_abs = sprintf("0x%02X", data_label[s[k]])
                #addr_rel = sprintf("0x%04X", data_label[s[k]]+ds-4*i)
                gsub("[$]" s[k], "$" addr_abs, param)
                #gsub(s[k], addr_rel, param)
            }
            print instr param
        }

        print "    #-- DATA_SECTION --------"
        write_data_init(ds)
        for (i=0; i<num_data_bytes; ++i) {
            if (i in data_globl) {
                write_data_flush(1)
                write_data_init(ds+i)
                split(data_globl[i], label, " ")
                for (l in label) {
                    print label[l], ":"
                }
            }

            if (i % 4 == 0) {
                write_data_init(ds+i)
            }
            write_data_set(i, data[i])
            if (i % 4 == 3) {
                write_data_flush(1)
            }

        }
        write_data_flush(1)
        print "    #-- DATA_SECTION (BSS) --"
        if (num_bss_bytes) {
            print "BSS ", num_bss_bytes
        }
        for (i in bss_label) {
            print i ": "  sprintf("0x%016X", bss_label[i])
        }
        print ""
    }
}
