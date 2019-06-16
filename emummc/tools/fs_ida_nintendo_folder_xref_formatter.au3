ParseClipboard()

Func FormatLineData($sLineData)
   Local $lineData = StringReplace($sLineData, @TAB, " ")
   Local $isADRP = false
   $lineData = StringReplace($lineData, "Up o sub_71000" , "0x")
   $isADRP = StringInStr($lineData, "ADRP")

   Local $targetRegister = StringSplit(StringSplit($lineData, "X")[2], ",")[1]
   $lineData = StringSplit($lineData, " ")[1]
   $lineDataAddr = StringSplit($lineData, "+")[1]
   $lineDataAddition = StringSplit($lineData, "+")[2]

   $lineDataAddr = StringReplace($lineDataAddr, "0x" , "")
   $lineDataAddition = StringReplace($lineDataAddition, "0x" , "")

   If $isADRP Then
	  $lineData = "0x" & Hex(Dec($lineDataAddr) + Dec($lineDataAddition))
   Else
	  Return ""
   EndIf

   Return "{.opcode_reg = " & $targetRegister & ", .adrp_offset = " & $lineData & "}, \" & @LF
EndFunc

Func ParseClipboard()
   Local $sData = ClipGet()
   Local $oData = ""
   Local $sLineData = StringSplit(StringReplace($sData, @CRLF, @LF), @LF)
   For $i = 2 to UBound($sLineData) - 2
	  Local $lineData = FormatLineData($sLineData[$i])
	  ;ConsoleWrite($lineData)
	  $oData = $oData & $lineData
   Next
   ClipPut($oData)
EndFunc
