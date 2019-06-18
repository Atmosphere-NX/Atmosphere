ParseClipboard()

Func FormatLineData($sLineData, $sLineDataAdd)
   Local $lineData = StringReplace($sLineData, @TAB, " ")
   Local $lineDataADRP, $lineDataADD
   Local $isADRP = false
   $lineData = StringReplace($lineData, "Up o sub_71000" , "0x")
   $isADRP = StringInStr($lineData, "ADRP")

   Local $targetRegister = StringSplit(StringSplit($lineData, "X")[2], ",")[1]
   $lineData = StringSplit($lineData, " ")[1]
   $lineDataAddr = StringSplit($lineData, "+")[1]
   $lineDataAddition = StringSplit($lineData, "+")[2]

   $lineDataAddr = StringReplace($lineDataAddr, "0x" , "")
   $lineDataAddition = StringReplace($lineDataAddition, "0x" , "")
   $addrADRP = Dec($lineDataAddr) + Dec($lineDataAddition)

   $lineDataADRP = "0x" & Hex($addrADRP)

   Local $lineDataADD = StringReplace($sLineDataAdd, @TAB, " ")
   Local $isADD = false
   $lineDataADD = StringReplace($lineDataADD, "Up o sub_71000" , "0x")
   $isADD = StringInStr($lineData, "ADD")

   $lineDataADD = StringSplit($lineDataADD, " ")[1]
   $lineDataAddAddr = StringSplit($lineDataADD, "+")[1]
   $lineDataAddAddition = StringSplit($lineDataADD, "+")[2]

   $lineDataAddAddr = StringReplace($lineDataAddAddr, "0x" , "")
   $lineDataAddAddition = StringReplace($lineDataAddAddition, "0x" , "")
   $addrADD = Dec($lineDataAddAddr) + Dec($lineDataAddAddition)
   $addrADD = $addrADD - $addrADRP
   $lineDataADD = "0x" & Hex($addrADD)

   Return @TAB & "{.opcode_reg = " & $targetRegister & ", .adrp_offset = " & $lineDataADRP & ", .add_rel_offset = " & $lineDataADD & "}, \" & @LF
EndFunc

Func ParseClipboard()
   Local $sData = ClipGet()
   Local $oData = ""
   Local $sLineData = StringSplit(StringReplace($sData, @CRLF, @LF), @LF)
   For $i = 2 to UBound($sLineData) - 2 Step 2
	  Local $lineData = FormatLineData($sLineData[$i], $sLineData[$i+1])
	  ;ConsoleWrite($lineData)
	  $oData = $oData & $lineData
   Next

   $oData = "{ \" & @LF & $oData & @TAB & "{.opcode_reg = 0, .adrp_offset = 0, .add_rel_offset = 0}, \" & @LF & "}" & @LF
   ;ConsoleWrite($oData)
   ClipPut($oData)
EndFunc
