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
#include <stdio.h> /* stderr */
#include <setupapi.h> /* SetupDiXxx */
#include <devioctl.h> /* DEVICE_TYPE */
#include <initguid.h> /* see <http://support.microsoft.com/kb/130869> */
#include <ntddstor.h> /* GUID_DEVINTERFACE_DISK */

#pragma comment (lib, "bufferoverflowu.lib") /* see <http://support.microsoft.com/kb/894573> */
#pragma comment (lib, "setupapi.lib")

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
	_ftprintf (stderr, _T("%s error 0x%08X: %s"), where, err, msg);
	LocalFree (msg);
}

static const LPTSTR help =
	_T("\n")
	_T("Usage: dskacl { -? | -i id [-s ssdl] [-r] [-v] }\n")
	_T("\n")
	_T("\t-?       show help screen\n")
	_T("\n")
	_T("\t-i id    PNP device id to change\n")
	_T("\n")
	_T("\t-s sddl  security descriptor to set\n")
	_T("\n")
	_T("\t-r       reconnect unit to effect changes\n")
	_T("\n")
	_T("\t-v       be verbose\n")
;

static const LPTSTR defaultSDDL =
	_T("D:(A;;FX;;;WD)(A;;FA;;;SY)(A;;FA;;;BA)(A;;FX;;;RC)(A;;GRGW;;;PU)")
;

int _tmain (int argc, _TCHAR *argv[]) {
  HDEVINFO dev;
  LPTSTR target = NULL;
  BOOL showHelp = FALSE;
  BOOL reconnect = FALSE;
  BOOL verbose = FALSE;
  int at = 1; // start at the first cmd line arg
  GUID const * guidIntf = &GUID_DEVINTERFACE_DISK;
  LPTSTR newSDDL = defaultSDDL;

  // parse command line arguments
  if (argc <= at) { // no args?
	showHelp = TRUE;
  }
  else if (!_tcscmp (argv[at], _T("-?"))) {
	showHelp = TRUE;
  }
  else if (!_tcscmp (argv[at], _T("-i"))) {
	++at; // swallow this arg
	if (argc <= at) { // is mandatory arg not there?
	  showHelp = TRUE;
	}
	else {
	  target = (LPTSTR) argv[at];
	  ++at; // swallow this arg
	  if (argc > at) { // more args?
		if (!_tcscmp (argv[at], _T("-s"))) { // opt. new sddl
		  ++at; // swallow this arg
		  if (argc <= at) { // is mandatory sddl not there?
			showHelp = TRUE;
		  }
		  newSDDL = (LPTSTR) argv[at];
		  ++at; // swallow this arg
		}
	  }
	  if (argc > at) { // still more args?
		if (!_tcscmp (argv[at], _T("-r"))) { // opt. release
		  ++at; // swallow this arg
		  reconnect = TRUE;
		}
	  }
	  if (argc > at) { // still more args?
		if (!_tcscmp (argv[at], _T("-v"))) {
		  ++at; // swallow this arg
		  verbose = TRUE;
		}
	  }
	}
  }
  if (argc > at) { // more args after parse?
	showHelp = TRUE;
  }

  // if not content with parameters, then don't continue
  if (showHelp) {
	_ftprintf (stderr, _T("%s"), help);
	return 0;
  }

  if (verbose) {
	_ftprintf (stdout, _T("Looking for disk devices...\n"));
  }
  dev = SetupDiGetClassDevs (guidIntf, NULL, NULL, DIGCF_DEVICEINTERFACE);
  if (dev == INVALID_HANDLE_VALUE) {
	ReportError (_T("GetClassDevs"));
	return 1;
  }
  _try {
	int i;
	for (i = 0; ;++i) {
	  BOOL bSuccess;
	  SP_DEVICE_INTERFACE_DATA intf;
	  PSP_DEVICE_INTERFACE_DETAIL_DATA name;
	  PTSTR id;
	  DWORD reqSize;
	  ZeroMemory (&intf, sizeof (SP_DEVICE_INTERFACE_DATA));
	  intf.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
	  bSuccess = SetupDiEnumDeviceInterfaces (dev, NULL, guidIntf, i, &intf);
	  if (!bSuccess) {
		if (GetLastError () == ERROR_NO_MORE_ITEMS) {
		  break;			 
		}
		else {
		  ReportError (_T("EnumDeviceInterfaces"));
		  return 2;
		}
	  }
	  // first get the number of bytes to allocate
	  bSuccess = SetupDiGetDeviceInterfaceDetail (dev, &intf, NULL, 0, &reqSize, NULL);
	  if (!bSuccess && (GetLastError () != ERROR_INSUFFICIENT_BUFFER)) {
		ReportError (_T("GetDeviceInterfaceDetail"));
		return 3;
	  }
	  name = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc (LPTR, reqSize);
	  _try {
		SP_DEVINFO_DATA info;
		ZeroMemory (&info, sizeof (SP_DEVINFO_DATA));
		info.cbSize = sizeof (SP_DEVINFO_DATA);
		ZeroMemory (name, reqSize);
		name->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
		bSuccess = SetupDiGetDeviceInterfaceDetail (dev, &intf, name, reqSize, NULL, &info);
		if (!bSuccess) {
		  ReportError (_T("GetDeviceInterfaceDetail"));
		  return 4;
		}

		bSuccess = SetupDiGetDeviceInstanceId (dev, &info, NULL, 0, &reqSize);
		if (!bSuccess && (GetLastError () != ERROR_INSUFFICIENT_BUFFER)) {
		  ReportError (_T("GetDeviceInstanceId"));
		  return 5;
		}
		/* note that this variant only gave us the number of characters, not the number of bytes */
		id = (PTSTR) LocalAlloc (LPTR, (reqSize+1) * sizeof (TCHAR));
		_try {
		  bSuccess = SetupDiGetDeviceInstanceId (dev, &info, id, reqSize, NULL);
		  if (!bSuccess) {
			ReportError (_T("GetDeviceInstanceId"));
			return 6;
		  }
		  if (!_tcscmp (id, target)) {
			LPTSTR sddl;
			if (verbose) {
			  _ftprintf (stdout, _T("Found: %s\n"), id);
			}

			bSuccess = SetupDiGetDeviceRegistryProperty (
				dev,
				&info,
				SPDRP_SECURITY_SDS,
				NULL,
				NULL,
				0,
				&reqSize);
			if (!bSuccess && (GetLastError () != ERROR_INVALID_DATA)) {
			  if (!bSuccess && (GetLastError () != ERROR_INSUFFICIENT_BUFFER)) {
				ReportError (_T("GetDeviceRegistryProperty"));
				return 7;
			  }
			  sddl = (LPTSTR) LocalAlloc (LPTR, reqSize);
			  _try {
				bSuccess = SetupDiGetDeviceRegistryProperty (
					dev,
					&info,
					SPDRP_SECURITY_SDS,
					NULL,
					(LPBYTE) sddl,
					reqSize,
					NULL);
				if (!bSuccess) {
				  ReportError (_T("GetDeviceRegistryProperty"));
				  return 8;
				}
				if (verbose) {
				  _ftprintf (stdout, _T("Existing security descriptor: %s\n"), sddl);
				}
			  }
			  _finally {
				LocalFree (sddl);
			  }
			}
		   
			// write new SDDL
			bSuccess = SetupDiSetDeviceRegistryProperty (
				dev,
				&info,
				SPDRP_SECURITY_SDS,
				(LPBYTE)newSDDL,
				(_tcslen (newSDDL) + 1) * sizeof (TCHAR));
			if (!bSuccess) {
			  DWORD err;
			  err = GetLastError ();
			  ReportError (_T("SetDeviceRegistryProperty"));
			  if ((err == ERROR_INVALID_DATA) && verbose) {
				_ftprintf (stderr, _T("Are you running as Administrator?\n"));
			  }
			  return 9;
			}
			if (verbose) {
			  _ftprintf (stdout, _T("Access control list successfully changed!\n"));
			}

			// reconnect after access property has changed
			if (reconnect) {
			  SP_PROPCHANGE_PARAMS pcp;
			  if (verbose) {
				_ftprintf (stdout, _T("Reconnecting device to reload security parameters...\n"));
			  }
			  // note that the size set here is for the header only; when we
			  // call the function we submit the size for the entire structure
			  pcp.ClassInstallHeader.cbSize = sizeof (SP_CLASSINSTALL_HEADER);
			  pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
			  pcp.StateChange = DICS_PROPCHANGE;
			  // only current hardware profile; could also be DICS_FLAG_GLOBAL
			  pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
			  pcp.HwProfile = 0;
			  // tell the class installer to restart the device
			  bSuccess = SetupDiSetClassInstallParams (
				  dev,
				  &info,
				  &pcp.ClassInstallHeader,
				  sizeof (pcp));
			  if (!bSuccess) {
				ReportError (_T("SetupDiSetClassInstallParams"));
				return 10;
			  }
			  bSuccess = SetupDiCallClassInstaller (
				  DIF_PROPERTYCHANGE,
				  dev,
				  &info);
			  if (!bSuccess) {
				ReportError (_T("SetupDiCallClassInstaller"));
				return 11;
			  }
			}
			if (verbose) {
			  _ftprintf (stdout, _T("Done!\n"));
			}
		  }		  
		}
		_finally {
		  LocalFree (id);
		}
	  }
	  _finally {
		LocalFree (name);
	  }
	}
  }
  _finally {
	SetupDiDestroyDeviceInfoList (dev);
  }
  return 0;
}
