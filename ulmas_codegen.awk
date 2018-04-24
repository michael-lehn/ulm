BEGIN {
    inst    = "^[[:space:]]*[[:alpha:]][[:alnum:]]*$"
    reg     = "^[%][[:digit:]]+$"
    mem1    = "^[(][%][[:digit:]]+$"
    mem2    = "^[%][[:digit:]]+[)]$"
    mem_rr  = "^[(][^,]+,[^,]+[)][,]?$"
    imm     = "^[$][-]?[[:digit:]]+[,]?"
    reg_vec = "^[%][(][^,]+,[^,]+[)][,]?$"

    FS = "[, \t]+"

    op_code["halt_r"]       = "00"
    op_code["get_r"]        = "01"
    op_code["put_r"]        = "02"
    op_code["put_rrr"]      = "03"
    op_code["nop"]          = "FF"
    op_code["load_ir"]      = "F6"

    op_code["movq_mmr"]     = "10"
    op_code["movq_imr"]     = "11"  # obsolete
    op_code["movzlq_mmr"]   = "12"
    op_code["movslq_mmr"]   = "13"
    op_code["movzwq_mmr"]   = "16"
    op_code["movswq_mmr"]   = "17"
    op_code["movzbq_mmr"]   = "1A"
    op_code["movsbq_mmr"]   = "1B"

    op_code["movq_rmm"]     = "40"
    op_code["movq_rim"]     = "41"
    op_code["movl_rmm"]     = "42"
    op_code["movw_rmm"]     = "44"
    op_code["movb_rmm"]     = "46"

    op_code["addq_rrr"]     = "60"
    op_code["addq_irr"]     = "61"
    op_code["subq_rrr"]     = "62"
    op_code["subq_irr"]     = "63"

    op_code["mulq_rrr"]     = "70"
    op_code["mulq_irr"]     = "71"

    op_code["orq_rrr"]      = "80"
    op_code["orq_irr"]      = "81"
    op_code["andq_rrr"]     = "82"
    op_code["andq_irr"]     = "83"
    op_code["shlq_rrr"]     = "84"
    op_code["shlq_irr"]     = "85"
    op_code["shrq_rrr"]     = "86"
    op_code["shrq_irr"]     = "87"

    op_code["jmp_i"]        = "9A"
    op_code["jmp_rr"]       = "9B"
    op_code["jmp_r"]        = "9C"


    op_code["jz_i"]         = "90"
    op_code["jnz_i"]        = "91"
    op_code["jl_i"]         = "92"
    op_code["jge_i"]        = "93"
    op_code["jle_i"]        = "94"
    op_code["jg_i"]         = "95"
    op_code["jb_i"]         = "96"
    op_code["jnb_i"]        = "97"
    op_code["jae_i"]        = "97"
    op_code["jbe_i"]        = "98"
    op_code["ja_i"]         = "99"

    code_line_nu = 0
}

function code3(op, x, y, z)
{
    code_ret = length(op) ? op : "??"
    return code_ret " " x " " y " " z
}

function code2(op, x, y)
{
    code_ret = length(op) ? op : "??"
    return code_ret " " x " " y
}

function code1(op, x)
{
    code_ret = length(op) ? op : "??"
    return code_ret " " x
}

function code0(op, x)
{
    code_ret = length(op) ? op : "??"
    return code_ret " 00 00 00"
}


function reg_code (id)
{
    gsub(/[% ]/, "", id)
    return sprintf("%02X", id)
}

function mem_code (id)
{
    gsub(/[%() ]/, "", id)
    return sprintf("%02X", id)
}

function imm_code (id)
{
    gsub(/[;$ ]/, "", id)
    if (id ~ "^-") {
        id = 256-strtonum(substr(id,2))
    } else {
        id = strtonum(id)
    }
    return sprintf("%02X", id)
}

function imm_code16 (id)
{
    gsub(/[;$ ]/, "", id)
    if (id ~ "^-") {
        id = strtonum(substr(id,2))
        if (id>lshift(1,16)) {
            print "error: immediate value " -id " requires more than 2 bytes"
            exit 1
        }
        id = lshift(1,16)-id
    } else {
        id = strtonum(id)
        if (id>lshift(1,16)) {
            print "error: immediate value " id " requires more than 2 bytes"
            exit 1
        }
    }
    return sprintf("%02X %02X",
                   and(rshift(id, 8), 0xFF),
                   and(rshift(id, 0), 0xFF))
}


function imm_code24 (id)
{
    gsub(/[;$ ]/, "", id)
    if (id ~ "^-") {
        id = strtonum(substr(id,2))
        if (id>lshift(1,24)) {
            print "error: immediate value " -id " requires more than 3 bytes"
            exit 1
        }
        id = lshift(1,24)-id
    } else {
        id = strtonum(id)
        if (id>lshift(1,24)) {
            print "error: immediate value " id " requires more than 3 bytes"
            exit 1
        }
    }
    return sprintf("%02X %02X %02X",
                   and(rshift(id,16), 0xFF),
                   and(rshift(id, 8), 0xFF),
                   and(rshift(id, 0), 0xFF))
}

function mem_ir_code (mem_ir)
{
    gsub(/%/, "", mem_ir)
    gsub(/[()]/, ",", mem_ir)
    split(mem_ir, a, ",")
    return reg_code(a[2]) "," imm_code(a[1])
}

function mem_rr_code (mem_rr)
{
    gsub(/[(%)]/, "", mem_rr)
    split(mem_rr, a, ",")
    return reg_code(a[1]) "," reg_code(a[2])
}

function reg_vec_code (reg_vec)
{
    gsub(/[(%)]/, "", reg_vec)
    split(reg_vec, a, ",")
    return reg_code(a[1]) "," reg_code(a[2])
}

function dump(line)
{
    gsub(/^[ ]*/, "", $0)
    print line "    # " sprintf("%04X", code_line_nu) " :  " $0
    code_line_nu += 4
}

function illegal(line)
{
    print "illegal instruction:> " line
}

/^\s*$/ {
    next;
}

/^\s+/ {
    gsub(/^\s*/, "", $0)
}


#
#  3-address:   OP M, M,  R
#
NF==4 && $1~inst && $2~mem1 && $3~mem2 && $4~reg {
    op = $1 "_mmr"
    if (length(op_code[op])) {
        line = code3(op_code[op], mem_code($2), mem_code($3), reg_code($4))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  3-address:   OP R, M, M
#
NF==4 && $1~inst && $2~reg && $3~mem1 && $4~mem2 {
    op = $1 "_rmm"
    if (length(op_code[op])) {
        line = code3(op_code[op], reg_code($2), mem_code($3), mem_code($4))
        dump(line)
    } else {
        illegal($0)
    }
    next
}


#
#  3-address:   OP R, R,  R
#
NF==4 && $1~inst && $2~reg && $3~reg && $4~reg {
    op = $1 "_rrr"
    if (length(op_code[op])) {
        line = code3(op_code[op], reg_code($2), reg_code($3), reg_code($4))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  3-address:   OP I, R,  R
#
NF==4 && $1~inst && $2~imm && $3~reg && $4~reg {
    op = $1 "_irr"
    if (length(op_code[op])) {
        line = code3(op_code[op], imm_code($2), reg_code($3), reg_code($4))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  2-address:   OP  I, R
#
NF==3 && $1~inst && $2~imm && $3~reg {
    op = $1 "_ir"
    if (length(op_code[op])) {
        line = code2(op_code[op], imm_code16($2), reg_code($3))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  1-address:   OP  R
#
NF==2 && $1~inst && $2~reg {
    op = $1 "_r"
    if (length(op_code[op])) {
        line = code3(op_code[op], reg_code($2), imm_code(0), imm_code(0))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  1-address:   OP  I
#
NF==2 && $1~inst && $2~imm {
    op = $1 "_i"
    if (length(op_code[op])) {
        line = code1(op_code[op], imm_code24($2))
        dump(line)
    } else {
        illegal($0)
    }
    next
}

#
#  0-address:   OP
#
NF==1 && $1~inst {
    if (length(op_code[$1])) {
        line = code0(op_code[$1])
        dump(line)
    } else {
        illegal($0)
    }
    next
}

{
    print $0
}
