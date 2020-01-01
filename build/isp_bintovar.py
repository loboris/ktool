#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import zlib
import binascii
import textwrap

isp_loader = open('isp.bin', 'rb').read()
isp_loader_comp = zlib.compress(isp_loader, 9)
isp_loader_hex = binascii.hexlify(isp_loader_comp)

isp_str = textwrap.fill(isp_loader_hex.decode(), 120)
isp = isp_str.split('\n')

isp_string = "        # 2nd stage K210 ISP binary ('isp.bin'), zipped and hexlified\n"
isp_string += '        ISP_PROG = ('

for i in range(len(isp)):
    if i == 0:
        isp_string += "'" + isp[i] + "'\n"
    else:
        isp_string += "                    '" + isp[i] + "'\n"

isp_string += '                   )\n'

open('ISP_PROG.py', 'w').write(isp_string)

