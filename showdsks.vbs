' -*- mode: visual-basic -*-
' Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt
Option Explicit
Dim mode, cmd

' --- parse command line arguments ---
If WScript.Arguments.Count >= 1 Then
  cmd = WScript.Arguments(0)
Else
  cmd = "-?"
End If
If cmd = "-d" Then
  mode = "disk"
ElseIf cmd = "-p" Then
  mode = "part"
Else
  mode = "help"
End If
If mode = "help" Then
  WScript.Echo vbCrLf & _
		 "Usage: cscript /nologo showdsks.vbs { -h | -d | -p } [ ""PNPDeviceID"" [ partition ] ]" & vbCrLf & _
		 vbCrLf & _
		 "   -h    Show this help" & vbCrLf & _
		 vbCrLf & _
		 "   -d    List connected disks" & vbCrLf & _
		 vbCrLf & _
		 "   -p    List all partitions in system" & vbCrLf & _
		 vbCrLf & _
		 "If PNPDeviceID or partition number is given, only entries matching these are listed." & vbCrLf & _
		 "Identifier is matched by checking if the argument is a proper prefix of it."
  WScript.Quit 1
End If

Dim part_filter, disk_filter
If WScript.Arguments.Count >= 2 Then
  disk_filter = WScript.Arguments(1)
Else
  disk_filter = ""
End If
If WScript.Arguments.Count >= 3 Then
  part_filter = CInt (WScript.Arguments(2))
Else
  part_filter = -1
End If

' --- common initialization ---
Dim fs, out, host, wmi, regex, match, disk, disks
Set fs = CreateObject ("Scripting.FileSystemObject")
Set out = fs.GetStandardStream (1) 'stdout
host = "."
Set wmi = GetObject ("winmgmts:" _
					 & "{impersonationLevel=impersonate}!\\" & _
					 host & "\root\cimv2")

' --- list disks ---
If mode = "disk" Then
  out.WriteLine "Disk;""PnPDeviceID"";BytesPerSector;TotalSectors;"
  Set regex = New RegExp
  regex.Pattern = "^\\\\.\\PHYSICALDRIVE([0-9]*)"
  Set disks = wmi.ExecQuery ("select * from Win32_DiskDrive")
  For Each disk In disks
	With disk
	  If regex.Test (.DeviceID) Then
		If InStr (1, .PNPDeviceID, disk_filter, vbTextCompare) = 1 Then
		  Set match = regex.Execute (.DeviceID)
		  out.WriteLine match(0).SubMatches(0) & ";" & _
			 """" & .PNPDeviceID & """;" & _
			 .BytesPerSector & ";" & _
			 .TotalSectors & ";"
		End If
	  End If
	End With
  Next
' --- list partitions ---
ElseIf mode = "part" Then
  Dim part, parts, boot, vol, vols, flsys, serno, label, mount, diskno, partno
  out.WriteLine "Disk;Partition;Boot;Offset;Size;Mount;FileSystem;Serial;Label;"
  Set regex = New RegExp
  regex.Pattern = "^Disk #([0-9]*), Partition #([0-9]*)"
  Set parts = wmi.ExecQuery ("select * from Win32_DiskPartition")
  For Each part In parts
	With part
	  If regex.Test (.DeviceID) Then
		Set match = regex.Execute (.DeviceID)
		diskno = CInt (match(0).SubMatches(0))
		partno = CInt (match(0).SubMatches(1))
		' see if this disk matches our filter
		Set disks = wmi.ExecQuery ("select * from Win32_DiskDrive " & _
								   "where DeviceID='\\\\.\\PHYSICALDRIVE" & _
								   CStr(diskno) & "' and PNPDeviceID like '" & _
								   disk_filter & "%'")
		If (disks.Count > 0) And _
		   ((partno = part_filter) Or (part_filter = -1)) Then
		  For Each disk In disks
			Exit For
		  Next
		  If .BootPartition Then
			boot = "*"
		  Else
			boot = " "
		  End If
		  Set vols = wmi.ExecQuery ("associators of " & _
									"{Win32_DiskPartition.DeviceID='" & .DeviceID & "'} " & _
									"where AssocClass=Win32_LogicalDiskToPartition")
		  If vols.Count > 0 Then
			For Each vol In vols
			  With vol
				flsys = .FileSystem
				serno = .VolumeSerialNumber
				label = .VolumeName
				mount = .Caption
			  End With
			  Exit For
			Next
		  Else
			flsys = ""
			serno = ""
			label = ""
			mount = ""
		  End If
		  ' VBoxManage uses one-based partition numbering
		  out.WriteLine diskno & ";" & _
			 (partno + 1) & ";" & _
			 boot & ";" & _
			 (.StartingOffset / disk.BytesPerSector) & ";" & _
			 (.Size / disk.BytesPerSector) & ";" & _
			 mount & ";" & _
			 flsys & ";" & _
			 serno & ";" & _
			 """" & label & """;"
		End If
	  End If
	End With
  Next
End If
