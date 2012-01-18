' Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt
' validate command-line
Set args = WScript.Arguments
If args.Count = 0 Then
  WScript.Echo "Usage: startvm.vbs MyVM"
  WScript.Quit 1
End If
vm = args(0)

' check if we are already running in a console
Set sh = CreateObject ("WScript.Shell")
Set fs = CreateObject ("Scripting.FileSystemObject")
Set windir = fs.GetSpecialFolder (1)
cscript = fs.BuildPath (windir, "cscript.exe")
f = fs.GetFile (WScript.FullName).Path
If Not (StrComp (cscript, f, 1) = 0) Then

  ' launch ourself in a hidden console
  ' could have used just sh.Run here
  startvm = WScript.ScriptFullName
  cmd = cscript + " /nologo """ + startvm + """ """ + vm + """"
  retVal = sh.Run (cmd, 0, True)
  WScript.Quit retVal
Else
  ' this part executes in a console; here it is OK to read stdout

  ' kick off the virtual machine; this will just trigger a signal
  ' to the VirtualBox daemon, and then return immediately
  ' we could have used sh.Run here too, but this do a better way
  ' with the focus and we don't need the return value since we are
  ' going to poll right afterwards anyway.
  Set wmi = GetObject("winmgmts:root\cimv2")
  Set proc = wmi.Get("Win32_Process")
  Set startup = wmi.Get("Win32_ProcessStartup")
  Set config = startup.SpawnInstance_
  config.ShowWindow = 12 'HIDDEN_WINDOW
  cmd = "VBoxManage startvm """ + vm + """"
  proc.Create cmd, Null, config, id

  'wait until child is gone before we start polling
  Set mon = wmi.ExecNotificationQuery _
    ("select * from  __InstanceDeletionEvent " & _
	 "within 1 where TargetInstance isa 'Win32_Process' " & _
	 "and TargetInstance.ProcessId = '" & id & "'")
  mon.NextEvent

  ' query the state of the virtual machine
  Set regex = New RegExp
  regex.Pattern = "^VMState=""(.*)"""
  Dim retval
  Do
    Set cmd = sh.Exec ("VBoxManage showvminfo """ + vm + """ --machinereadable")
    Set out = cmd.StdOut

    ' parse the output
    running = False
    While Not out.AtEndOfStream
      line = out.ReadLine
      If (Not running) And (regex.Test (line)) Then
	    Set match = regex.Execute (line)
		state = match(0).SubMatches(0)
	    If state = "running" Then
	      running = True
        Else
		  Select Case state
	      Case "saved"
		    retval = 4
		  Case "powered off"
		    retval = 3
	      Case Else
		    retval = 2
		  End Select
	      running = False
	    End If
      End If
    Wend

    If Not (running = True) Then Exit Do

    ' wait until we poll again
    WScript.Sleep 3000
  Loop
End If

WScript.Quit retval
