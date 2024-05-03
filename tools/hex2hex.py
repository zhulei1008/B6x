# -*- coding: utf-8 -*-
"""
Created on Thu Nov  8 17:04:19 2018

@author: rk
"""
"""
:llaaaatt[dd...]cc
:    - start 
ll   - length of data
aaaa - address, start address
tt   - record type, 00 - data, 01, eof, 02-expand block address, 04-linear address
dd   - data
cc   - crc
"""

import sys
import os
import string

def usage():
    print("Usage: python hex2mif.py xxx.hex [outpath] for 256KB depth")
    
def convert(hFile, outPath):
    data={}
    wordAdr=0
    print("process file : "+ hFile)
    (filepath,tempfilename) = os.path.split(hFile)
    #print(tempfilename)
    outFileName=outPath+tempfilename.split('.')[0]+'_SIM.hex'
    hexFile=open(hFile,'r')
    mifFile=open(outFileName, 'w')
 #   mifFile.write("WIDTH=32;\r\n")
 #   mifFile.write("DEPTH=65536;\r\n")
 #   mifFile.write("ADDRESS_RADIX=HEX;\r\n")
 #   mifFile.write("DATA_RADIX=HEX;\r\n\r\n")
 #   mifFile.write("CONTENT BEGIN\r\n")
    baseaddr = 0
    print("output file : "+ outFileName)
    #romHex=open(hFile.split('.')[0]+'.rom.hex', 'w')
    for line in hexFile.readlines():
        if line[0] != ':':
            print("invalid hex file")
            return
        else:
            length=int(line[1:3],16)  #convert 16 string to int
            address=int(line[3:7],16)
            linetype=line[7:9]
            if (length==0):
                if (linetype=='01'):
                    print("EOF of "+hFile)
                else:
                    print("Error: Invalid hex file")
                print ("last addrss %x"%wordAdr)
       #         if wordAdr < 0xFFFF:
       #             mifFile.write("    ["+hex(wordAdr)[2:]+"..FFFF]: 00000000;\r\n")
       #         mifFile.write("ENDS;")
                mifFile.close()
            else:
                if (length==2)&(linetype=='04'):
                    #baseaddr=int(line[9:13],16)*(2**14)
                    print (baseaddr)
                elif linetype=='00':
                    #offset=int(line[9:13],16)
                    for idx in range(0,length):
                        data[idx]=line[9+idx*2:+11+idx*2]
                    if (length!=16):
                        print (line)
                    wordAdr=baseaddr+address//4
                    for idx in range(0,length//4):
                        #mifFile.write("@"+hex(wordAdr)[2:])
                        mifFile.write("@"+'{0:#08X}'.format(wordAdr)[2:]);
                        mifFile.write(' '+data[idx*4+3]+data[idx*4+2]+data[idx*4+1]+data[idx*4+0]+"\n")
                        wordAdr+=1

    hexFile.close()


#print(sys.argv)
if len(sys.argv)==1:
    usage()
    exit
else:
    hexFile=sys.argv[1]
    if len(sys.argv)==3:
        fpath=sys.argv[2]
    else:
        (fpath,fname) = os.path.split(hexFile)
    if (len(fpath)>0):
        fpath=fpath.rstrip('\'\"\\/')+'/'
    #print("fpath: "+fpath)
    convert(hexFile, fpath)
