# -*- coding: utf-8 -*-
"""
Created on 2020 @HungYi

@author: 6vip
"""
"""
Intel Hex Format:
:llaaaatt[dd...]cc
:    - start 
ll   - length of data
aaaa - address, start address
tt   - record type, 00 - data, 01, eof, 02-expand block address, 04-linear address
dd   - data
cc   - crc
"""
"""
ROM Blocks: 
    1 - BOT(0x000000 ~ 0x0003FF)  4KB, DEPTH = 1024 (0x0000 ~ 0x03FF)
"""

import sys
import os
import string
import time

def usage():
    print("Usage: python mif_rom.py xxx.hex [outpath] for 4KB(_BOT)") # + 160KB(_BLE) depth")
    
def convert(hFile, outPath):
    print("process file : "+ hFile)
    (filePath,fileName) = os.path.split(hFile)
    #print(fileName)
    hexFile = open(hFile,'r')
    
    # botROM: xxx_BOT.mif 
    botROM = outPath+fileName.split('.')[0]+'_BOT.mif'
    botFile = open(botROM, 'w')
    botFile.write("WIDTH=32;\n")
    botFile.write("DEPTH=1024;\n")
    botFile.write("ADDRESS_RADIX=HEX;\n")
    botFile.write("DATA_RADIX=HEX;\n\n")
    botFile.write("CONTENT BEGIN\n")
    print("output botROM : "+ botROM)
    
    baseAddr = 0
    wordAddr = 0
    botAddr = 0
    bleAddr = 0
    curDate = 0
    hexData = {}

    for line in hexFile.readlines():
        if line[0] != ':':
            print("invalid hex file")
            return
        else:
            length = int(line[1:3],16)  #convert 16 string to int
            address = int(line[3:7],16)
            linetype = line[7:9]
            if (length == 0):
                if (linetype == '01'):
                    print("EOF of "+hFile)
                else:
                    print("Error: Invalid hex file")
                print ("last botROM Addr: 0x%X"%botAddr)
                if botAddr <= 0x400:
                    botFile.write("    ["+hex(botAddr)[2:]+"..03FC]: 00000000;\n")
                    #modify by door
                    #get current date, write to rom's the last word 
                    curDate = time.strftime('%Y%m%d',time.localtime(time.time()))
                    botFile.write("    "+'3FF :' + curDate +";\n")
                    botFile.write("ENDS;")
                else:
                    print("Error: botROM Size > 4K")
                botFile.close()                
            else:
                if (length == 2)and(linetype == '04'):
                    baseAddr = int(line[9:13],16)*(2**14)
                    #print ("baseAddr: 0x%08X"%(baseAddr*4))
                    if (baseAddr >= 0xA000):
                        print("Error: invalid hex size, baseAddr=0x%08X"%(baseAddr*4))
                        return
                elif linetype == '00':
                    for idx in range(0,length):
                        hexData[idx] = line[9+idx*2:+11+idx*2]
                    if (length != 16):
                        print (line)
                    wordAddr = baseAddr+address//4
                    if (wordAddr < 0x400):  #botRom
                        botAddr = wordAddr
                        for idx in range(0,length//4):
                            botFile.write("    "+hex(botAddr)[2:])
                            botFile.write(' : '+hexData[idx*4+3]+hexData[idx*4+2]+hexData[idx*4+1]+hexData[idx*4+0]+";\n")
                            botAddr+= 1

    hexFile.close()


#print(sys.argv)
argc = len(sys.argv)
if argc == 1:
    usage()
    exit
else:
    doCopy = False
    hFile = sys.argv[1]
    if argc == 3:
        fpath = sys.argv[2]
        doCopy = True
    else:
        (fpath,fname) = os.path.split(hFile)
    if (len(fpath)>0):
        fpath = fpath.rstrip('\'\"\\/')+'\\'
        if not os.path.exists(fpath):
            os.makedirs(fpath)
        if doCopy:
            print('copy '+hFile+' '+fpath)
            os.system('copy '+hFile+' '+fpath+' > nul')
    
    #print("fpath: "+fpath)
    convert(hFile, fpath)
