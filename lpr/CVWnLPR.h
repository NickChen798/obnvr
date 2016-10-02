// CVWnLpr.h - Windows Implementation of LPR Platform
//
//	Author: Mou-Yen Chen (mychen@orbit.com.tw)
// 
// Copyright(C) by Orbit Tech. Inc., All Rights Reserved.
//  The information contained herein is the exclusive property of Orbit Tech. Inc. 
//	and shall not be distributed, reproduced, or disclosed in whole or in part without 
//	prior written permission of Orbit Tech. Inc.
//
//	$Id$
//	$Log$
//
// Document Class: ORBIT CONFIDENTIAL
//

#ifndef _CVWNLPR_H_
#define _CVWNLPR_H_

#include "OBWinDef.h"
#include "OBLogFil.h"
#include "OBImgPrc.h"

#ifdef	__cplusplus
extern "C"  {
#endif	/* __cplusplus */

//D0: LPR License Registration
#define	LPR_LIC_MASK		0x00FF	//license mask
	#define	LPR_LIC_CR20	0x0001	//CarReader 2.0 full (4-ch)
//	#define	LPR_LIC_CR21	0x0002	//CarReader 2.0 (1-ch)
//	#define	LPR_LIC_CR22	0x0003	//CarReader 2.0 (2-ch)
//	#define LPR_LIC_MOB1	0x0010	//CarReader 2.0 Mobile
	#define	LPR_LIC_CR2C01	0x0061	//097 CarLPR 1-ch	
	#define	LPR_LIC_CR2C02	0x0062	//098 CarLPR 2-ch	
	#define	LPR_LIC_CR2C03	0x0063	//099 CarLPR 3-ch	
	#define	LPR_LIC_CR2C04	0x0064	//100 CarLPR 4-ch	
#define LPR_OPT_MASK	0x0F00		//license option mask
	#define LPR_OPT_DEMO	0x0100	//demo license type 1: partial function
#define	LPR_SRC_MASK		0xF000	//license source mask
	#define LPR_SRC_SRNO	0x0000	//serial number
	#define LPR_SRC_FILE	0x1000	//VLK(Volume License Key) file
BOOL	WINAPI	WNLPRInit(		//fn: license check or registration (only for TWN)
		WORD	wType,			//ip: type of license (see D0 above)
		LPSTR	szProdSerial );	//ip: (1) null => check license
								//	  (2) other => registration (by serial # or VLK file)
								//rt: success or failure
HANDLE	WINAPI	WNLPRInitEx(	//fn: license check or registration
		WORD	wType,			//ip: type of license (see D0 above)
		LPSTR	szProdSerial,	//ip: (1) null => check license
								//	  (2) other => registration (by serial # or VLK file)
		DWORD	dwRegion );		//ip: region for the license
								//rt: success or failure
//recommand procedure for license checking:
//(1) use MBLPRInitEx(LPR_LIC_MOB1,NULL,Region) to check license (return TRUE if license OK)
//(2) if (1) fail, ask user to register by
//	(2-1) MBLPRInitEx(LPR_LIC_MOB1|LPR_SRC_SRNO,"Serial#",Region) to register by giving one serial #
//	(2-2) MBLPRInitEx(LPR_LIC_MOB1|LPR_SRC_FILE,"VLK_File",Region) to register by giving VLK file
BOOL	WINAPI	WNLPRClose(		//fn: free license handle
		HANDLE	hLic );			//ip: handle of license
								//rt: success or failure
////////////////////////////////////////////////////////////////////////////////////////////////
#define	MAX_PLATE_NO	10			//max # of plates to locate in a picture
#define	MAX_PLATE_LEN	16			//max length of plate number, e.g., ­xA-12345
typedef struct tagPLTRST {			//structure of a plate result
	BYTE		szCarNo[MAX_PLATE_LEN];	//plate number
	RECT		rectLoc;				//plate location
} PLTRST, *PPLTRST, *LPPLTRST;

typedef struct tagPICRST {			//structure of plate results in a picture
	BYTE		szFile[MAX_PATH];		//picture filename or number
	int			nPlateNo;				//# of plates found in a picture
	PLTRST		PltRst[MAX_PLATE_NO*2];	//plate result structure in a picture
} PICRST, *PPICRST, *LPPICRST;

////////////////////////////////////////////////////////////////////////////////////////////////
//PART I: Resource Manipulation
////////////////////////////////////////////////////////////////////////////////////////////////
//D1: LPR parameters for OBLPRSetParam()
//parameters for channels (only used when LPR_PRM_PMODE=LPR_PMODE_FILE)
//parameters for locating
#define LPR_PRM_EDGE		0x0101	//TEdge, default=8		
#define	LPR_PRM_PWIDTH		0x0102	//plate width, default=138
#define	LPR_PRM_PHEIGHT		0x0103	//plate height, default=62
#define	LPR_PRM_PTHRESH		0x0104	//binary threshold, default=-35
#define LPR_PRM_INDEX		0x0105	//Index, default=110
//#define LPR_PRM_SCANWIDTH	0x0106	//scan width, default=8
//#define LPR_PRM_WHITESCALE	0x0107	//white scale, default=35
#define LPR_PRM_PLATENO		0x0108	//max plate no to locate, default=1
#define LPR_PRM_ROITOP		0x010A	//·j´M¦ì¸m(¤W) ROI: top					(e.g., 0)
#define LPR_PRM_ROIBOT		0x010B	//·j´M¦ì¸m(¤U) ROI: bottom				(e.g., 479)
#define LPR_PRM_ROILFT		0x010C	//·j´M¦ì¸m(¥ª) ROI: left					(e.g., 0)
#define LPR_PRM_ROIRGT		0x010D	//·j´M¦ì¸m(¥k) ROI: right				(e.g., 719)
#define LPR_PRM_LOCMTHD		0x010E	//locate method, default=0		
	#define LPR_LOCMTHD_IR		0x0001	//locate method: blocking for IR		
	#define LPR_LOCMTHD_TRUCK	0x0002	//locate method: blocking for trucks		
	#define	LPR_LOCMTHD_JRTRK	0x0003	//locate method: truck from Jerry (used in TPP)
	#define LPR_LOCMTHD_GRAY	0x0004	//locate method: gray
	#define LPR_LOCMTHD_MYS		0x0005	//locate method: for MYS special
	#define LPR_LOCMTHD_TWN		0x0006	//locate method: for TWN special
	#define	LPR_LOCMTHD_OLD		0x000E	//locate method: old general
#define	LPR_PRM_PWIDTH2		0x0112	//second plate width, default=138
#define	LPR_PRM_PHEIGHT2	0x0113	//second plate height, default=62

//parameters for recognition
#define LPR_PRM_DBSDIR		0x0201	//database location, default=C:\tmp
#define	LPR_PRM_PREGION		0x0202	//plate region, default=LPR_RGN_TWN (see CP_LprFrm.h)
#define LPR_PRM_PFORMAT		0x0203	//plate format, default=LPR_PRM_TWN_FMT_CAR

//parameters for image processing
#define LPR_PRM_NOISEREMOVE	0x0301	//noise remove: 0=off, 1=on

//other parameters 
#define LPR_PRM_PLOGLVL		0x0901	//loglevel: 0(only exception) ~ 9(detail), default=5
#define LPR_PRM_PEXPLVL		0x0902	//excplevel: 0(quiet), 1(log only, default), 2(log & image)
	//	>=2	(1)Exception OBI: \_excp_(func)_(time).obi
	//	>=1	(1)Log: "EXCP(#code) event was raised for (func) at (time)"
	//	=0	quiet		

//for debugging
#define LPR_PRM_DBGCHI		0xFF01	//write CHI/CH image for each segmented character
#define LPR_PRM_DBGOBI		0xFF02	//write OBI image for each extracted plate

HANDLE	WINAPI	WNLPRCreate(	//fn: prepare the LPR resources
		DWORD	dwRegion,		//ip: LPR_RGN_* defined in CP_LprFrm.h
		LPSTR	szDbsDir,
		LPINT	lpnStatus,		//op: Status (<=0 if error)
		HDC		hDC,
		HLOG	hLog );			//ip: handle of log
								//rt: handle of LPR (NULL of failure)
HANDLE	WINAPI	WNLPRCreateEx2(	//fn: prepare the LPR resources
		HANDLE	hLic,
		long	nMaxWidth,
		long	nMaxHeight,
		LPSTR	szDbsDir,
		LPINT	lpnStatus,		//op: Status (<=0 if error)
		HDC		hDC,
		HLOG	hLog );			//ip: handle of log
								//rt: handle of LPR (NULL of failure)

BOOL	WINAPI	WNLPRFree(		//fn: free LPR resource
		HANDLE	hMBLpr );		//ip: handle of LPR
								//rt: success or not
BOOL	WINAPI	WNLPRSetParam(	//fn: set parameter value	 
		HANDLE	hMBLpr, 		//ip: handle of LPR
		WORD	wParam,			//ip: parameter to set (see D1)
		LPSTR	lpszValue );	//ip: value to set
								//rt: success or not
BOOL	WINAPI	WNLPRGetParam(	//fn: get parameter value	 
		HANDLE	hMBLpr, 		//ip: handle of LPR
		WORD	wParam,			//ip: parameter to get (see D1)
		LPSTR	lpszValue,		//op: value to get
		int		nLen );
/*								//rt: success or not
BOOL	WINAPI	WNLPRRecog( 
		HANDLE	hMBLpr, 
		HANDLE	hImage, 
		LPSTR	szResult, 
		int		nSize );
*/
//for szResult, it will have the following syntax: 
//		szFile|CarNo;CarNo;..|Rect;Rect;..|Note
// e.g.,	$szFile$|[ERR]||....							error in recognizing file $szFile$
//			$szFile$|||....									no plate found in $szFile$
//			$szFile$||(x1,y1)(x2,y2);...|					no result from OCR
//			$szFile$|CarNo;CarNo;..|(x1,y1)(x2,y2);..|		got the result as 'CarNo' in $szFile$
//for LPR_CALLBACK_MESSAGE, it will have the following syntax: 
//		[msg_type] note
// e.g.,	[ERR] ....		error in recognizing file $szFile$
//			[MSG] ....		no plate found in $szFile$

BOOL	WINAPI	WNLPRRecog1( 
		HANDLE	hWNLpr, 
		HANDLE	hImage, 
		LPPICRST lpResult );
//for lpResult, it will fill up the following structure
// PICRST 
// {
//		nPlateNo;				//# of plates found in a picture
//		PltRst[MAX_PLATE_NO*2];	//plate result structure in a picture
//		{
//			szCarNo[MAX_PLATE_LEN];	//plate number
//			rectLoc;				//plate location
//		}
//	}

//functions from other modules
//BOOL	WINAPI	WNImageHookBuffer( HIMAGE hImage, LPSTR lpBuffer );
//BOOL	WINAPI	WNImageUnhookBuffer( HIMAGE hImage );
//HIMAGE	WINAPI	WNExtractRectImageEx( 
//				HIMAGE	hSrcImg, 
//				LPRECT	lpRect, 
//				DWORD	dwOption );
//HIMAGE 	WINAPI	WNOCR_CreateImage( 		/* fn:	Create Image Buffer */
//				short 	WidthBytes, 	/* ip:	# of WidthBytes */
//				short 	nWidth,			/*		Width */
//				short 	nLength,		/*		Length */ 
//				short 	nBitsPerSample, /*		# of bits per pixel */
//				BOOL 	bImgInfoOnly );	/*		if Header only */
//										/* rt:	Image handle created */
//void	WINAPI  WNOCR_FreeImage( 		/* fn:	Free image resouce */
//				HIMAGE 	hImage );		/* ip:	Image handle */
//										/* rt:	(none)	*/
#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif //_CVWNLPR_H_