#! /usr/bin/env python

F_WHITESPACE = '1'
F_DIGIT      = '2'
F_NUMBER     = '4'
F_LITERAL    = '8'
F_CONTROL    = '16'

C_WHITESPACE = "\r\n\t "
C_DIGIT      = "0123456789"
C_NUMBER     = C_DIGIT+".eE-+"
C_LITERAL    = "truefalsenull"

for x in xrange(0,256):
    flags = []
    c = chr(x)
    if c in C_WHITESPACE: flags.append(F_WHITESPACE)
    if c in C_DIGIT: flags.append(F_DIGIT)
    if c in C_NUMBER: flags.append(F_NUMBER)
    if c in C_LITERAL: flags.append(F_LITERAL)
    if x <= 0x1F: flags.append(F_CONTROL)
    
    fs = '0'
    if len(flags): fs = '|'.join(flags)
    ascii = '   '
    if x >= 32 and x <= 126: ascii = "'"+c+"'"
    line = '/* %3d 0x%02x %s */ %8s,' % (x, x, ascii, fs)
    print line+'  ',
    if ((x%4) == 3): print ""
