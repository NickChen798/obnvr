// OBLogFil.h : Logfile Manipulation Toolkits
//
//	Copyright(C) 1998 by Orbit Technology Inc., All Rights Reserved.
//	Written by Marvin Chen
//
// $Id$
// $Log$
//
#ifndef _OBLOGFIL_H_
#define _OBLOGFIL_H_

#include "OBWinDef.h"

#ifdef	__cplusplus
extern "C"  {
#endif	/* __cplusplus */

typedef LPVOID	HLOG;

HLOG	WINAPI	CreateLog( 
		LPSTR	lpszDir );
/* CreateLog: Creating a logfile naming mmddhhmm.in the given directory
 * 	I/P: 	lpszDir - Directory to create log file
 * 	O/P: 	none
 *	Return:	hLog    - handle of logfile (null if failure)
 */
void	WINAPI	FreeLog( 
		HLOG	hLog );
/* FreeLog: Free logfile resource and close associated logfile
 * 	I/P: 	hLog    - handle of logfile
 * 	O/P: 	none
 *	Return:	none
 */

void	WINAPI	WriteLog( 
		HLOG	hLog, 
		HWND	hSysRec, 
		LPSTR	lpszMsg );
/* WriteLog: Write message to logfile and message window if given.
 *			 If there is any error, its error message will be written to logfile as well
 * 	I/P: 	hLog    - handle of logfile
 *			hSysRec	- window handle of message window (ignore if null is given)
 *			lpszMsg - disired message to output
 * 	O/P: 	none
 *	Return:	none
 */

BOOL	WINAPI	GetLogFilename( 
		HLOG	hLog, 
		LPSTR	szFile, 
		short	nLen );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBSCM: Orbit Storage Control Mechanism
BOOL	WINAPI	OBStgCreateFolder(	//fn:	Create sub folder under SCM
		LPSTR	szBaseDir,			//ip:	Base directory to manipulate	
		LPSTR	szSubFolder,		//ip:	Sub folder to create
		long	nMinSize,			//ip:	Start house keeping if the space less than this minimal size (in MB)
		long	nMinDay,			//ip:   Data should not be deleted if its life is less than this day (in day) 
		HLOG	hLog );				//ip:   Handle of log 
									//rt:	Success or not
HLOG	WINAPI	OBStgCreateDailyLog(//fn:	Create daily log under SCM 
		LPSTR	szBaseDir,			//ip:	Base directory to manipulate	
		long	nMinSize,			//ip:	Start house keeping if the space less than this minimal size (in MB)
		long	nMinDay,			//ip:   Data should not be deleted if its life is less than this day (in day) 
		DWORD	dwOption );			//ip:	(reserved)
									//rt:	Handle of log
//Note: 傳回之hLog仍可使用於原來之WriteLog()及GetLogFilename() functions
int		WINAPI	OBStgHouseKeeping(	//fn:	Housekeeping under SCM
		LPSTR	szBaseDir,			//ip:	Base directory to manipulate	
		long	nMaxDay,			//ip:   Data will be kept if its life is less than this day (in day) 
		HLOG	hLog );				//ip:   Handle of log 
									//rt:	# of entries deleted (-1 if error)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* _OBLOGFIL_H_ */
