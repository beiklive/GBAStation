#!/usr/bin/env python3

# A Python script to convert NstDatabase.xml to a file that can be baked into
# the Nestopia core, so that users won't have to maintain NstDatabase.xml in
# their system directory.
#
# Licensed under the same license as Nestopia UE.

import sys
if len(sys.argv) > 1:
    in_file = sys.argv[1]
else:
    in_file = "../NstDatabase.xml"

if len(sys.argv) > 2:
    out_file = sys.argv[2]
else:
    out_file = "nstdatabase.hpp"

f1 = open(in_file, "r")
f2 = open(out_file, "w")
f2.write("const unsigned char nst_db_xml[] = {\n\t")

while (s := f1.read(1).encode("utf-8")) != b'':
    # Handle multibyte characters
    for i in range(0, len(s)):
        f2.write("0x%x, " % s[i])

f2.write("\n};\n")
f1.close()
f2.close()
