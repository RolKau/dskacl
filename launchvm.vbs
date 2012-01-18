' Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt
Option Explicit
Dim args, vm
' validate command-line
Set args = WScript.Arguments
If args.Count = 0 Then
  WScript.Echo "Usage: launchvm.vbs MyVM"
  WScript.Quit 1
End If
vm = args(0)

' launch the virtual machine and wait for it to run
Dim client, backend, machine, progress
Set client = CreateObject ("VirtualBox.Session")
Set backend = CreateObject ("VirtualBox.VirtualBox")
Set machine = backend.findMachine (vm)
Set progress = machine.launchVMProcess (client, "gui", "")
progress.waitForCompletion -1

' request the console shown in the foreground
Dim wnd, sh
wnd = machine.ShowConsoleWindow
Set sh = CreateObject ("WScript.Shell")
sh.Run "%SystemRoot%\System32\rundll32.exe user32.dll,SetForegroundWindow " & CStr(wnd), 2, True

' from src/VBox/Main/idl/VirtualBox.xidl
Const PoweredOff = 1
Const Saved = 2
Const Aborted = 4

' poll the machine until it exits
Dim state
Do
  state = machine.state
  WScript.Sleep 1000
Loop Until (state = PoweredOff) Or _
		   (state = Saved) Or _
		   (state = Aborted)

' set exit code from script depending on whether accessing disk is safe or not
If state = PoweredOff Then
  WScript.Quit 0
Else
  WScript.Quit state
End If
