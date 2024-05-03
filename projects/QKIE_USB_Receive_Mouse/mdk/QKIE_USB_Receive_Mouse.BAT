SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\TortoiseSVN\bin;C:\Program Files\IDM Computer Solutions\UltraCompare;C:\Users\朱书海\AppData\Local\Programs\Python\Python38\Scripts\;C:\Users\朱书海\AppData\Local\Programs\Python\Python38\;C:\Users\朱书海\AppData\Local\Microsoft\WindowsApps
SET ARMCC5_ASMOPT=--diag_suppress=9931
SET ARMCC5_CCOPT=--diag_suppress=9931
SET ARMCC5_LINKOPT=--diag_suppress=9931
SET CPU_TYPE=B6x
SET CPU_VENDOR=Generic
SET UV2_TARGET=QKIE_USB_Receive_Mouse
SET CPU_CLOCK=0x00F42400
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\output\startup._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\main.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app_actv.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app_msg.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app_gapm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app_gapc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\app_gatt.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\usbd_hid.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\hid_boot.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\output\debug.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\output\QKIE_USB_Receive_Mouse_20240109.lnp"
"C:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" ".\output\QKIE_USB_Receive_Mouse_20240109.axf" --i32combined --output ".\output\QKIE_USB_Receive_Mouse_20240109.hex"
fromelf.exe  --bin -o "D:\work\Dragon_C\SDK\bxx_DragonC1\sdk6\projects\QKIE_USB_Receive_Mouse\mdk\output\QKIE_USB_Receive_Mouse_20240109.bin" "D:\work\Dragon_C\SDK\bxx_DragonC1\sdk6\projects\QKIE_USB_Receive_Mouse\mdk\output\QKIE_USB_Receive_Mouse_20240109.axf"
