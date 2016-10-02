/* OBErrMsg.h : Error/Message handling routines
 *	
 *	Copyright(C) 1998 by Orbit Tech. Inc., all rights reserved.
 *
 *	Author: Mou-Yen Chen (mychen1@ms22.hinet.net)
 *
 *	Notes: 
 *	Orbit 32-bit errorcode definition: (follow Windows definition)
 *		 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *		 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *		+---+-+-+-----------------------+-------------------------------+
 *		|Sev|C|R|     Facility          |               Code            |
 *		+---+-+-+-----------------------+-------------------------------+
 *		where
 *			Sev - is the severity code
 *					00 - Success
 *					01 - Informational
 *					10 - Warning
 *					11 - Error
 *			C - is the Customer code flag
 *			R - is a reserved bit
 *			Facility - is the facility code
 *			Code - is the facility's status code
 * $Id$
 * $Log$
 */

#ifndef     _OBERRMSG_H_
#define     _OBERRMSG_H_

#ifdef	__cplusplus
extern "C"  {
#endif	/* __cplusplus */
//#ifndef _DSP
//#include <windows.h>
//#else
#include "OBWinDef.h"
//#endif

#ifndef _WINDOWS
#define	sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#define	strcpy_s(buffer, buffer_size, stringbuffer) (strcpy(buffer, stringbuffer))
#define	strcat_s(buffer, buffer_size, stringbuffer) (strcat(buffer, stringbuffer))
#endif

/*************/
/* Constants */
/*************/
/* D0: error code mask */
#define OB_ERR_MASK_SEV		0xC0000000		/* bit 30~31: severity code */
#define OB_ERR_MASK_USR		0x20000000		/* bit 29: userdefined -> always on */
#define OB_ERR_MASK_FAC		0x0FFF0000		/* bit 16~27: facility */
#define OB_ERR_MASK_COD		0x0000FFFF		/* bit 0~15: codes */

/* D1: Severity codes (bit 30~31) */
#define OB_ERR_NORMAL		0x00000000		/* 00: success */
#define OB_ERR_INFORM		0x40000000		/* 01: information */
#define OB_ERR_WARNING		0x80000000		/* 10: warning */
#define OB_ERR_ERROR		0xC0000000		/* 11: error */

/* D2: Facity codes (bit 16~27) */
#define OB_ERR_FR_S			0x00100000		/* 1: scan/polling */
#define OB_ERR_FR_V			0x00200000
#define OB_ERR_FR_F			0x00300000		/* formread */
#define OB_ERR_FR_E			0x00400000		/* editing */
#define OB_ERR_FR_G			0x00500000		/* gateway */
#define OB_ERR_FR_C			0x00600000		/* cd maker */
#define OB_ERR_FR_Q			0x00700000		/* query */
#define OB_ERR_FR_X			0x00800000		/* problem solving */
#define OB_ERR_FR_M			0x00F00000		/* management */

/* D3: Status codes (bit 0~15) */
/* D3-1: codes which have no argument */
#define OB_ERR_NOERROR 	    0x00000000		/* no error */
#define OB_ERR_OUTOFMEMORY  0x00000001		/* out of memory */
//#define OB_ERR_SYSINTERNAL	0x00000002		/* system internal error */
/* D3-2: error codes which have one argument */
#define OB_ERR_OPEN	    	0x00000100	/* error open %file */	
#define OB_ERR_READ	    	0x00000101	/* error read %file */
#define OB_ERR_WRITE	    0x00000102	/* error write %file */
#define	OB_ERR_SYSPARAM		0x00000103	/* system %parameter error (probabily out of range) */
#define OB_ERR_FLOWCNTLDBS	0x00000104	/* error in flow control %database */
#define OB_ERR_INIFILE		0x00000105	/* bad parameter in %ini_file */
#define OB_ERR_IMAGEPARAM	0x00000106	/* bad image parameter */		
#define OB_ERR_NOIMAGE	    0x00000107	/* image handle contains no image */
#define	OB_ERR_NETWORK		0x00000108	/* network error */
/* D3-3: error code for vender supply */
#define OB_ERR_UNDEFINED    0x00000200

/***********************/
/* Function Prototypes */
/***********************/
/* GetError and ShowError will clear error buffer */
void 	WINAPI 	SetMessageTitle( 	/* fn:	set AP's name in message title */
				LPSTR lpString );	/* ip:	string of AP's name */
									/* rt:	none */
void	WINAPI	ShowMessage( 		/* fn:	show message in a simple dialog */
				LPSTR lpString );	/* ip:	message string to display */
									/* rt:	none */
#ifdef WIN32
void	WINAPI	ShowMsgDlg( HWND hWnd, LPSTR lpString );
#endif

void	WINAPI	SetError( 			/* fn:	set error */
				DWORD dwErrorCode, 	/* ip:	error code defined in D1~D3 as above */
				LPSTR lpAuxMsg );	/* 		associated message */
									/* rt:	none */
DWORD	WINAPI	GetError( 			/* fn:	retrieve error status */
				LPSTR lpAuxMsg, 	/* op:	buffer to retrieve error message */
				UINT nBufMax );		/* ip:	length of lpAuxMsg buffer */
									/* rt:	error code as defined in D1~D3 */
void	WINAPI	ShowError(			/* fn:	retrieve error message and display in a simple dialog */
				void );				/* ip:	none */
									/* rt:	none */
void	WINAPI	SetWarning( 		/* fn:	set error */
				DWORD nErrorCode, 	/* ip:	error code defined in D1~D3 as above */
				LPSTR lpAuxMsg );	/* 		associated message */
									/* rt:	none */
//DWORD	WINAPI	GetWarning(			/* fn:	retrieve error status */
//				LPSTR lpAuxMsg, 	/* op:	buffer to retrieve error message */
//				UINT nBufMax );		/* ip:	length of lpAuxMsg buffer */
//									/* rt:	error code as defined in D1~D3 */
//void	WINAPI	ShowWarning(		/* fn:	retrieve error message and display in a simple dialog */
//				void );				/* ip:	none */
//									/* rt:	none */

#define MYSetError(dwErrorCode,lpAuxMsg) __SetError(dwErrorCode,lpAuxMsg,__FILE__,__LINE__)

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* _OBERRMSG_H_ */
