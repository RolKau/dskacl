/* Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt */
/* either one is fine */
#ifdef UNICODE
  #ifndef _UNICODE
    #define _UNICODE
  #endif
#endif
#ifdef _UNICODE
  #ifndef UNICODE
    #define UNICODE
  #endif
#endif
#define WIN32_LEAN_AND_MEAN
#include <tchar.h> /* TCHAR, _ftprintf */
#include <windows.h>
#include <stdarg.h>   /* va_list, va_start */
#include <winioctl.h> /* FSCTL_xxx_VOLUME */

#pragma comment (lib, "bufferoverflowu.lib") /* see <http://support.microsoft.com/kb/894573> */

void __cdecl print (DWORD con, LPTSTR fmt, ...) {
  va_list args;
  int chars;
  LPTSTR buf;
  HANDLE h;
  DWORD written;
  DWORD lastErr;
  /* allocate memory to hold the output string */
  va_start (args, fmt);
  chars = _vsctprintf (fmt, args);
  va_end (args);
  buf = HeapAlloc (
	  GetProcessHeap (),
	  HEAP_NO_SERIALIZE,
	  sizeof (TCHAR) * (chars+1));
  if (!buf) {
	/* what to do? we cannot write any messages */
	return;
  }
  _try {
	va_start (args, fmt);
	written = _vsntprintf (buf, chars, fmt, args);
	va_end (args);

	/* to avoid any surprises for the caller */
	lastErr = GetLastError ();
	_try {
	  /* print to console if available, otherwise gui */
	  AttachConsole (GetCurrentProcessId ());
	  if (GetLastError () == ERROR_INVALID_HANDLE) {
		MessageBox (
			NULL,
			buf,
			_T("Xclusive"),
			MB_OK | (con == STD_ERROR_HANDLE ?
				MB_ICONERROR :
				MB_ICONINFORMATION));
	  }
	  else {
		h = GetStdHandle (con);
		if ((h == INVALID_HANDLE_VALUE) || (h == NULL)) {
		  return;
		}
		WriteConsole (h, buf, chars, &written, NULL);
	  }
	}
	_finally {
	  SetLastError (lastErr);
	}
  }
  _finally {
	HeapFree (
		GetProcessHeap (),
		HEAP_NO_SERIALIZE,
		buf);
  }
}

void ReportError (LPTSTR where) {
	LPTSTR msg;
	DWORD err;
	err = GetLastError ();
	FormatMessage (
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0,
		NULL);
	print (STD_ERROR_HANDLE, _T("%s error 0x%08X: %s"), where, err, msg);
	LocalFree (msg);
}

static const LPTSTR help =
  _T("\n")
  _T("Usage: xclusive volume command\n")
  _T("\n")
  _T("\tvolume   native path to volume, e.g. \\\\.\\E:\n")
  _T("\n")
  _T("\tcommand  process to launch when volume is locked\n")
  _T("\n")
;

int APIENTRY _tWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow) {
  LPTSTR volume;
  LPTSTR next;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  BOOL success;
  HANDLE device;
  DWORD bytes;
  DWORD exitCode;
  
  /* if the first argument starts with a quote, then look for the quote
	 to end it as well, otherwise look for space */
  if (lpCmdLine[0] == '"') {
	volume = lpCmdLine+1;
	next = _tcschr (volume, '"');
  }
  else {
	volume = lpCmdLine;
	next = _tcschr (volume, ' ');
  }
  /* skip to next argument */
  if (next == NULL) {
	next = lpCmdLine + _tcslen (lpCmdLine);
  }
  else {
	/* terminate first argument (pointer stored in 'volume') */
	*next = '\0';
	do {
	  next++;
	} while (*next == ' ');
  }
  /* no command is given, show help and bail out */
  if ((*volume == '\0') || (*next == '\0')) {
	print (STD_ERROR_HANDLE, _T("%s"), help);
	return 1;
  }

  /*
  print (STD_OUTPUT_HANDLE, _T("volume:  \"%s\"\n"), volume);
  print (STD_OUTPUT_HANDLE, _T("command: `%s`\n"), next);
  */

  device = CreateFile (
	  volume,
	  GENERIC_READ | GENERIC_WRITE,
	  FILE_SHARE_READ | FILE_SHARE_WRITE,
	  NULL,
	  OPEN_EXISTING,
	  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
	  NULL);
  if (device == INVALID_HANDLE_VALUE) {
	ReportError (_T("CreateFile"));
	return 2;
  }

  _try {
	/* lock disk to ensure exclusive access */
	success = DeviceIoControl (
		device,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&bytes,
		NULL);
	if (!success) {
	  ReportError (_T("FSCTL_LOCK_VOLUME"));
	  return 3;
	}
	_try {
	  ZeroMemory (&si, sizeof (STARTUPINFO));
	  si.cb = sizeof (STARTUPINFO);
	  success = CreateProcess (
		  NULL,
		  next,
		  NULL,
		  NULL,
		  FALSE,
		  CREATE_SUSPENDED,
		  NULL,
		  NULL,
		  &si,
		  &pi);
	  if (!success) {
		ReportError (_T("CreateProcess"));
		return 4;
	  }
	  _try {
		success = DeviceIoControl (
			device,
			FSCTL_DISMOUNT_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&bytes,
			NULL);
		if (!success) {
		  ReportError (_T("FSCTL_DISMOUNT_VOLUME"));
		  return 5;
		}
		
		/* start the process */
		ResumeThread (pi.hThread);
		WaitForSingleObject (pi.hProcess, INFINITE);
		success = GetExitCodeProcess (pi.hProcess, &exitCode);
		if (!success) {
		  ReportError (_T("GetExitCodeProcess"));
		  return 6;
		}
		return exitCode ? 7 : 0;
	  }
	  _finally {
		CloseHandle (pi.hThread);
		CloseHandle (pi.hProcess);
	  }
	}
	_finally {
	  DeviceIoControl (
		  device,
		  FSCTL_UNLOCK_VOLUME,
		  NULL,
		  0,
		  NULL,
		  0,
		  &bytes,
		  NULL);
	}	  
  }
  _finally {
	CloseHandle (device);
  }
}
