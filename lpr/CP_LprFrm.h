// CP_LprFrm.h : Error/Message handling routines
//  Copyright(C) by Orbit Tech. Inc., All Rights Reserved.
//
// Author: Mou-Yen Chen (mychen@orbit.com.tw)
//
// Version History:
//	$Id$
//	$Log$

#ifndef     _CP_LPRFRM_H_
#define     _CP_LPRFRM_H_

#ifdef	__cplusplus
extern "C"  {
#endif	/* __cplusplus */

//#ifdef _DSP
#include "OBWinDef.h"
//#else
//#include <windows.h>
//#endif //_DSP
#include "OBLogFil.h"
#include "OBImgPrc.h"

/*************/
/* Constants */
/*************/
#define LPR_RGN_MSK		0x00000FFF	//Normal region 000~FFF
// D1: Region code (can only choose one region from the below)
///////////////////////////////////////////////////////////////////////////////////
//		//Define		//Hex		//Code	//country				//License
///////////////////////////////////////////////////////////////////////////////////
//						0x000000FF	//Asia	
#define LPR_RGN_GEN		0x00000000	//(0)	General					OBLB_KRNLIC_INT
///////////////////////////////////////////////////////////////////////////////////
//						0x000000FF	//Asia	
#define LPR_RGN_TWN		0x00000001	//(1)	Taiwan					OBLB_KRNLIC_TWN	
#define LPR_RGN_CHN		0x00000002	//(2)	China					OBLB_KRNLIC_CHN 
#define	LPR_RGN_JPN		0x00000003	//(3)	Japan					OBLB_KRNLIC_JPN
#define	LPR_RGN_MYS		0x00000004	//(4)	Malaysia				OBLB_KRNLIC_INT
#define	LPR_RGN_HKG		0x00000005	//(5)	Hong Kong				OBLB_KRNLIC_INT
#define LPR_RGN_SGP		0x00000006	//(6)	Singapore (General)
#define	LPR_RGN_THA		0x00000007	//(7)	Tailand (Complete)		OBLB_KRNLIC_THA
#define LPR_RGN_PHL		0x00000008	//(8)	Philippines (General)	OBLB_KRNLIC_INT
#define	LPR_RGN_THA_S	0x00000009	//(9)	Tailand (General)		OBLB_KRNLIC_INT
#define	LPR_RGN_ARE		0x00000081	//(129)	United Arab Emirates	OBLB_KRNLIC_ARE
///////////////////////////////////////////////////////////////////////////////////
//						0x000001FF	//Europe
#define LPR_RGN_ITA		0x00000101	//(257)	Italy (General)
#define LPR_RGN_GBR		0x00000102	//(258)	United Kindom			OBLB_KRNLIC_INT
#define LPR_RGN_FRA		0x00000103	//(259)	France (General)
#define LPR_RGN_DEU		0x00000104	//(260)	Germany (General)
//#define LPR_RGN_IRL	0x00000105	//(261)	Ireland
//#define LPR_RGN_NOR	0x00000106	//(262)	Norway
//#define LPR_RGN_AUT	0x00000107	//(263)	Austria
#define LPR_RGN_BEL		0x00000108	//(264)	Belgium (General)
//#define LPR_RGN_CYP	0x00000109	//(265)	Cyprus
#define LPR_RGN_DNK		0x0000010A	//(266)	Denmark (General)
#define LPR_RGN_FIN		0x0000010B	//(267)	Finland (General)
//#define LPR_RGN_GRC	0x0000010C	//(268)	Greece
//#define LPR_RGN_ISL	0x0000010D	//(269)	Iceland
#define LPR_RGN_HUN		0x0000010E	//(270)	Hungary (General)
//#define LPR_RGN_SWE	0x0000010F	//(271)	Sweden
#define LPR_RGN_CHE		0x00000110	//(272)	SWITZERLAND (General)
//#define LPR_RGN_NLD	0x00000111	//(273)	NETHERLANDS
#define LPR_RGN_PRT		0x00000112	//(274)	Portugal (General)
#define LPR_RGN_CZE		0x00000113	//(275)	CZECH REPUBLIC (General)
#define LPR_RGN_ESP		0x00000114	//(276) SPAIN (General)
#define LPR_RGN_POL		0x00000115	//(277) POLAND (General)
#define LPR_RGN_TUR		0x00000116	//(278) TURKEY (General)
///////////////////////////////////////////////////////////////////////////////////
//						0x000002FF	//Africa
#define	LPR_RGN_ZAF		0x00000201	//(513)	South Africa			OBLB_KRNLIC_INT
///////////////////////////////////////////////////////////////////////////////////
//						0x000003FF	//America
#define LPR_RGN_USA		0x00000301	//(769)	USA						OBLB_KRNLIC_USA
#define LPR_RGN_CAN		0x00000302	//(770)	Canada					OBLB_KRNLIC_CAN
#define LPR_RGN_BRA		0x00000351	//(849)	Brazil					OBLB_KRNLIC_INT
///////////////////////////////////////////////////////////////////////////////////
//						0x000004FF	//Oceania
//#define LPR_RGN_AUS		0x00000401	//(1025)AUSTRALIA				OBLB_KRNLIC_AUS
///////////////////////////////////////////////////////////////////////////////////
//						0x8FFFFFFF	//Others
#define	LPR_RGN_TEST	0x80000000	//the highest bit is for testing 
#define	LPR_RGN_NUL		0x80000001	//(1879048193)	Null LPR (neither locate nor LPR)
#define	LPR_RGN_LOC		0x80000002	//(1879048194)	Only locate, no LPR
#define	LPR_RGN_TST		0x8FFFFFFF	//(2147483647)	Testing

// D2: parameter for format
#define LPR_PRM_FMT		0x00000001		//parameter for LPR format
	//V2-1: TWN format (can be the combination of the following)					DOL		LOD
	#define LPR_PRM_TWN_FMT_MSK		0x7FFF
	#define LPR_PRM_TWN_FMT_CAR		0x0001		//XX-NNNN or NNNN-XX				*
//	#define	LPR_PRM_TWN_FMT_TAXI	0x0002		//XX-NNN or NNN-XX (include truck)	*		*		phase out
//	#define LPR_PRM_TWN_FMT_SPC		0x0004		//外-NN or 使-NN or 軍X-NNNNN		*				phase out
	#define LPR_PRM_TWN_FMT_HBIKE	0x0004		//XX-NN										*
	#define LPR_PRM_TWN_FMT_BIKE	0x0008		//XXX-NNN or NNN-XXX				*		*
//
	#define	LPR_PRM_TWN_FMT_PTAXI	0x0010		//XX-NNN or NNN-XX (pure taxi)		*
	#define LPR_PRM_TWN_FMT_TRUCK	0x0020		//XX-NNN or NNN-XX (truck)			*		*
	#define	LPR_PRM_TWN_FMT_DPLMT	0x0040		//外-NN or 使-NN						*
	#define	LPR_PRM_TWN_FMT_ARMY	0x0080		//軍X-NNNNN									*
//#define LPR_PRM_TWN_FMT_FLAG_OLD	0x4000	
#define LPR_PRM_TWN_FMT_FLAG_NEW	0x8000	
//#define LPR_PRM_TWN_FMT_FLAG_MSK	0x8000	

	//V2-2: CHN format (can be the combination of the following)					DOL		LOD
	#define LPR_PRM_CHN_FMT_CAR		0x0001		//General Car								*		
	#define LPR_PRM_CHN_FMT_BUS		0x0002		//Big Car (Bus)						*
	#define LPR_PRM_CHN_FMT_POLICE	0x0004		//Police Car						*
	#define LPR_PRM_CHN_FMT_WJ		0x0008		//WuJin(武警)						*
	#define LPR_PRM_CHN_FMT_ARMY	0x0010		//Military Vehicle					*
	#define LPR_PRM_CHN_FMT_LEARN	0x0020		//Training School					*
	#define LPR_PRM_CHN_FMT_DPLMT	0x0040		//外交										*
	#define LPR_PRM_CHN_FMT_HKGMAC	0x0080		//港澳										*
	//V2-3: GEN format (can be the combination of the following)					DOL		LOD
	#define LPR_PRM_GEN_FMT_DOL		0x0001		//淺底深字							*		
	#define LPR_PRM_GEN_FMT_LOD		0x0002		//深底淺字									*

#define LPR_PRM_THRESH	0x00000002		//parameter for binarization threshold
#define	LPR_PRM_TWSPCNF	0x00000003		//TW_Special confidence: 0~3 (default=1)
#define	LPR_PRM_SLANTCR	0x00000004		//slant correction: 0~1 (default=0)
//debug
#define LPR_PRM_EXCPLVL	0x00000101		//exception handle level: 0(quiet), 1(log only, default), 2(dump OBI)
#define LPR_PRM_CHIOUTOLD	0x00000102	//write CHI/CH output for each segmented character
#define LPR_PRM_LOGLVL	0x00000901		//loglevel: 0(only exception) ~ 9(detail), default=5
	//	>=9	"Not a plate"
	//	>=8	output TwSepecial OBI file
	//	>=5	szRst=%s
	//	>=4
	//	>=3
	//	>=2	Save Exception image
	//	>=1	Exception log
	//	=0	quiet
#define LPR_PRM_CHIOUT	0x00000902		//write CHI/CH output for each segmented character
#define LPR_PRM_OBIOUT	0x00000903		//write OBI output for each extracted block
//Get
#define	LPR_PRM_CHARNO	0x00001000
#define	LPR_PRM_CHRECTL	0x00001100		//lower bound of character rect
#define	LPR_PRM_CHRECTU	0x000011FF		//upper bound of character rect

/***********************/
/* Function Prototypes */
/***********************/
//Create & Free
HANDLE	WINAPI	CP_LPRCreate(		//fn: allocate resource for LPR
				LPSTR	szDbsDir,	//ip: the dbs directory 
				UINT	dwRegion,	//ip: region of LPR (D1 as above)
				UINT	dwOption,	//ip: options (reserved)
				HLOG	hLog );		//ip: handle of log
									//rt: handle of LPR (null if failure)
BOOL	WINAPI	CP_LPRFree(			//fn: free resource of LPR 
				HANDLE	hLpr,		//ip: handle of LPR
				HLOG	hLog );		//ip: handle of log
									//rt: success or failure
//Recognition
int		WINAPI	CP_LPRRecog(		//fn: recognition 
				HANDLE	hLpr,		//ip: handle of LPR
				HANDLE	hRaw,		//ip: I/P image to recognize
				int		nWidth,
				int		nHeight,
				LPSTR	szRst,		//op: result string
				HLOG	hLog );		//ip: handle of log
									//rt: no of characters returned (<0 if error)
int		WINAPI	CP_LPRRecogEx(		//fn: recognition 
				HANDLE	hLpr,		//ip: handle of LPR
				HANDLE	hRaw,		//ip: I/P image to recognize
				int		nWidth,
				int		nHeight,
				int		nBitsPerPixel,
				LPSTR	szRst,		//op: result string
				HLOG	hLog );		//ip: handle of log
									//rt: no of characters returned (<0 if error)
int		WINAPI	CP_LPRRecogEx2(		//fn: recognition 
				HANDLE	hLpr,		//ip: handle of LPR
				HANDLE	hRaw,		//ip: I/P image to recognize
				int		nWidth,
				int		nHeight,
				int		nWidthBytes,
				int		nBitsPerPixel,
				LPSTR	szRst,		//op: result string
				HLOG	hLog );		//ip: handle of log
									//rt: no of characters returned (<0 if error)
//Set Parameters
BOOL	WINAPI	CP_LPRSetParam(		//fn: setting parameters
				HANDLE	hLpr,		//ip: handle of LPR
				UINT	uiParam,	//ip: parameter to set (D2 as above)
				int		nValue,		//ip: value to set (V2 as above)
				HLOG	hLog );		//ip: handle of log
									//rt: success or failure
BOOL	WINAPI	CP_LPRGetParam(		//fn: setting parameters
				HANDLE	hLpr,		//ip: handle of LPR
				UINT	uiParam,	//ip: parameter to set (D2 as above)
				LPSTR	lpszValue,	//op: buffer to get value
				UINT	uiLen,
				HLOG	hLog );		//ip: handle of log
									//rt: success or failure
HIMAGE	WINAPI	CP_LPRGetImage( 
				HANDLE	hLpr, 
				UINT	uiOption, 
				HLOG	hLog );

//DSP
int		WINAPI	CPDSP_LPRCreate( 
				BYTE *lpBuffer, int nBufferSize, LPSTR szDbsDir, UINT uiRegion, HLOG hLog, UINT uiOption );
int		WINAPI	CPDSP_LPRFree( HLOG hLog, UINT uiOption );
int		WINAPI	CPDSP_LPRRecogEx( 
				HANDLE hRaw, int nWidth, int nHeight, int nBitsPerPixel, LPSTR szRst, HLOG hLog, UINT uiOption );
int		WINAPI	CPDSP_LPRSetParam( 
				UINT uiParam, int nValue, HLOG hLog, UINT uiOption );

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* _CP_LPRFRM_H_ */
