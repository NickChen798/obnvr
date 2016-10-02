/* OBImgPrc.h : General Image Manipulation Library
 *
 *	Copyright(C) 1998 by Orbit Tech. Inc., all rights reserved.
 *
 *	Author: Mou-Yen Chen (mychen1@ms22.hinet.net)
 * 
 * $Id$
 * $Log$
 */
#ifndef _OBIMGPRC_H_
#define _OBIMGPRC_H_

#ifdef	__cplusplus
extern	"C"     {
#endif	/* __cplusplus */

//#if defined(WIN32) || defined(_WIN32_WCE)
//#include <windows.h>
//#else
#include "OBWinDef.h"
//#endif	//WIN32

typedef HGLOBAL HIMAGE;

/* Definition D1: Supported Image Format */
#define IMG_FMT_MASK		0xFF00		/* major file format */
#define IMG_FMT_SUBMSK		0x00FF		/* file format extension */

#define IMG_FMT_UNKNOWN		0x0000		/* Unknown format */
#define IMG_FMT_TIFF		0x0100		/* TIFF format */
	#define IMG_FMT_TIFRAW		0x0101		/* no compression */
	#define IMG_FMT_TIFPKBIT	0x0102		/* packbits */
	#define IMG_FMT_TIFLZW		0x0103		/* LZW */
	#define IMG_FMT_TIFLZWD		0x0104		/* LZW /w differentiation */
	#define IMG_FMT_TIFG2		0x0105		/* CCITT Group 3 1D NO EOL */
	#define IMG_FMT_TIFG3		0x0106		/* CCITT Group 3 1D */
	#define IMG_FMT_TIFG3_2D	0x0107		/* CCITT Group 3 2D */
	#define IMG_FMT_TIFG4		0x0108		/* CCITT Group 4 */
	#define	IMG_FMT_TIFJPEG		0x0109		/* JPEG */
	#define IMG_FMT_TIFAUTO		0x01FF		/* B/W->G4, ow->JPEG */
#define IMG_FMT_BITMAP		0x0200		/* Windows bitmap format (no compression) */
	#define IMG_FMT_BMPRAW		0x0201		/* no compression */
	#define IMG_FMT_BMPRLE		0x0202		/* /w RLE compression */
	#define IMG_FMT_BMPAUTO		0x02FF		/* B/W->raw, ow->RLE */
#define IMG_FMT_PNG			0x0300		// PNG format: lossless compression 
#define IMG_FMT_GIF			0x0400		// GIF format: need LZW license
#define IMG_FMT_JPEG		0x0800		/* JPEG format */
	#define IMG_FMT_JPGLOSSLESS	0x0801		// lossless JPEG: may not be read by some viewer	
#define IMG_FMT_PCX			0x1100		/* PCX format */
#define IMG_FMT_JBIG		0x2100		/* JBIG format */
#define IMG_FMT_PNM			0x4100		/* PNM format */
	#define	IMG_FMT_PBMTXT		0x4101		/* P1: PBM ASCII */
	#define	IMG_FMT_PGMTXT		0x4102		/* P2: PGM ASCII */
	#define	IMG_FMT_PPMTXT		0x4103		/* P3: PPM ASCII */
	#define	IMG_FMT_PBMBIN		0x4104		/* P4: PBM Raw */
	#define	IMG_FMT_PGMBIN		0x4105		/* P5: PGM Raw */
	#define	IMG_FMT_PPMBIN		0x4106		/* P6: PPM Raw */
#define IMG_FMT_PDF			0x8100		/* PDF format */
#define IMG_FMT_OBI			0x8200		/* OBI format: DATA */
	#define	IMG_FMT_OBIBW		0x8201
	#define	IMG_FMT_OBIGRAY		0x8202
	#define	IMG_FMT_OBICOL		0x8203
#define	IMG_FMT_CHI			0x8300		// CHI format (bit image) 
#define IMG_FMT_RAW			0x8800		// RAW format: DATA 
	#define	IMG_FMT_RAWBW		0x8801
	#define	IMG_FMT_RAWGRAY		0x8802
	#define	IMG_FMT_RAWCOL		0x8803	//RGB
	#define	IMG_FMT_RAWYV12		0x8804	//YV12

/* Definition D2: Option for Image Reading & Writing */
#define IMG_OPT_ROTATE		0x0003	/* Image rotation option */
#define IMG_ROTATE_NO		0x0000		/* no rotation */
#define IMG_ROTATE_90CCW	0x0001		/* CCW  90 degree */
#define IMG_ROTATE_180CCW	0x0002		/* CCW 180 degree */
#define IMG_ROTATE_270CCW	0x0003		/* CCW 270 degree */

#define IMG_OPT_TYPE		0x000C	/* image type option */
#define IMG_TYPE_ORIG		0x0000		/* no change */
#define	IMG_TYPE_BIN		0x0004		/* Black & white */
#define IMG_TYPE_GRAY		0x0008		/* gray scale */
#define IMG_TYPE_COLOR		0x000C		/* color */

#define IMG_OPT_OTHERS		0x00f0	/* Other options */
#define IMG_OPT_INVERT		0x0010		/* invert image */
#define IMG_OPT_APPEND		0x0020		/* append image at the end of file (only for writing) */
#define IMG_OPT_ENCRPT		0x0040		/* with encryption */

//for save only
//#define IMG_QLT_MASK		0x0f00	/* image quality factor from 1(rough) to 15(fine), default 0=7(medium) */
#define IMG_QLT_MASK		0xff00	/* image quality factor from 1(rough) to ff(fine), default 0=80(medium) */

/* Definition D3: Image Type */
#define IMG_TYPE_BIN_MASK			0x000f
#define IMG_TYPE_BIN_OBJ_IS_ONE		0x0001
#define IMG_TYPE_BIN_OBJ_IS_ZERO	0x0002
#define IMG_TYPE_GRAY_MASK			0x00f0
#define IMG_TYPE_GRAY_OBJ_IS_ONE	0x0010
#define IMG_TYPE_GRAY_OBJ_IS_ZERO	0x0020
#define IMG_TYPE_COLOR_MASK			0x0f00

/* Definition D4: Conversion method */
#define IMG_CONV_C2G_MASK			0x000f
#define IMG_CONV_C2G_MAGIC			0x0001	
#define IMG_CONV_C2G_AVERAGE		0x0002	//=(r+b+g)/3
#define IMG_CONV_C2G_RED			0x0003
#define IMG_CONV_C2G_GREEN			0x0004
#define IMG_CONV_C2G_BLUE			0x0005
#define IMG_CONV_C2G_BWAVG			0x0006	//AVERAGE /w B/W enhance

/* Part I : Basic Image Manipulaation */
/* part 1-1: FRImgHdl.c : image create, free, load, duplicate */
HIMAGE 	WINAPI	OCR_CreateImage( 		/* fn:	Create Image Buffer */
				short 	WidthBytes, 	// ip:	WidthBytes; 0 => auto assign
				short 	nWidth,			/*		Width */
				short 	nLength,		/*		Length */ 
				short 	nBitsPerSample, /*		# of bits per pixel */
				BOOL 	bImgInfoOnly );	/*		if Header only */
										/* rt:	Image handle created */
void	WINAPI  OCR_FreeImage( 			/* fn:	Free image resouce */
				HIMAGE 	hImage );		/* ip:	Image handle */
										/* rt:	(none)	*/
HIMAGE 	WINAPI	OCR_LoadImage( 			/* fn:	Load image from file (first page only if multi-page image) */
				LPSTR 	FileName, 		/* ip:	File name */
				short 	nRotate, 		/*		Rotation angle: 0 -	no rotate; 1 - 90 CCW; 2 - 180 CCW; 3 - 270 CCW	*/
				BOOL 	bInvert );		/*		If invertion right after loading */
										/* rt:	Image handle loaded */
HIMAGE 	WINAPI 	OCR_LoadImageEx( 		/* fn:	Load image from file */
				LPSTR 	FileName, 		/* ip:	File name */
				short 	nPage, 			/*		Page number (starting from 0) */
				WORD 	wOption );		/*		Option for reading (see definition D2) */
										/* rt:	Image handle loaded */
HIMAGE	WINAPI	OCR_LoadRawImage(
				LPSTR	FileName,
				WORD	wImgFmt,
				short	nWidth,
				short	nHeight,
				WORD 	wOption );		/*		Option for reading (see definition D2) */

HIMAGE 	WINAPI 	OCR_DuplicateImage( 	/* fn:	Duplicate image */
				HIMAGE 	hImage, 		/* ip:	Image handle */
				BOOL 	bImgInfoOnly );	/* 		If image header only */
										/* rt:	Image handle duplicated (null if failure) */
BOOL 	WINAPI 	OCR_WriteImage( 		/* fn:	Write image to file */
				LPSTR 	FileName, 		/* ip:	Output file name */
				HIMAGE 	hImage, 		/*		Image handle to write */
				WORD 	wImgFmt,		/*		Image format (see definition D1) */
				WORD 	wSaveOpt );		/*		Option for writing (see definition D2) */
										/* rt:	Success or failure */
short 	WINAPI	OCR_GetPageNumber(		/* fn:	Get total number of pages */ 
				LPSTR	lpFile );		/* ip:	Image file name */
										/* rt:	Total number of pages (-1 if failure) */
HIMAGE	WINAPI	OCR_ReadImageMemory(	// fn:	Read hImage from memory buffer
				LPSTR	pBuffer,		// ip:  Buffer to read from 
				DWORD	dwBufLen,		// ip:  Length of buffer 
				WORD	wOption );		// ip:  options for reading
										// rt:	Handle of image (NULL if error)
int		WINAPI	OCR_WriteImageMemory(	// fn:	write hImage to memory buffer 	 
				LPBYTE	szBuffer,		// op:	buffer to write to 
				DWORD	dwSize,			// ip:	size of buffer 
				HIMAGE	hImage,			// ip:	hImage to read from
				WORD	wImgFmt,		// ip:	image format 
				WORD	wSaveOpt );		// ip:  options for writing
										// rt:  size of buffer written (<0 if error)
BOOL	WINAPI	OCR_CopyImage(			// fn:	Copy hImage 
				HIMAGE	hDstImg,		// ip:  Destination
				HIMAGE	hSrcImg );		// ip:	Source
										// rt:	Successs or failure
//D5: Operations for combining images
#define IMG_COMB_AND    0x0001
#define IMG_COMB_OR     0x0002
#define IMG_COMB_XOR    0x0003
#define IMG_COMB_ADD    0x0004
//D6: Options for combining images
#define IMG_COMB_TILE	0x0001		//Tile the source image
//#define IMG_COMB_STRTCH	0x0001		//Stretch the source image to the target image
	
HIMAGE	WINAPI	OCR_CombImage(
				HIMAGE	hDstImg,
				HIMAGE	hSrcImg,
				WORD	wOperation,		//		(see D5)
				WORD	wOption );		//		(see D6)
//Image list
HANDLE	WINAPI	OCR_LoadImgLst(			// fn:	load image list from file to memory 
				LPSTR	szFileName,		// ip:	i/p file name
				WORD	wOption );		//		option (not yet use)
										// rt:	handle of image list (multipage)
BOOL	WINAPI	OCR_FreeImgLst( 
				HANDLE	hImgLst );
HIMAGE	WINAPI	OCR_GetPageFromImgLst( 
				HANDLE	hImgLst, 
				short	nPage, 
				WORD	wOption );
int		WINAPI	OCR_GetImgLstCount( 
				HANDLE	hImgLst );

/* part 1-2: hBitmap.c: bitmap conversion */
HIMAGE 	WINAPI 	Bitmap2Image( 			/* fn:	Conversion from DDB to hImage */
				HBITMAP hBitmap );		/* ip:	handle of DDB */
										/* rt:	hImage (null if error) */
HBITMAP WINAPI	Image2Bitmap( 			/* fn:	Conversion from hImage to DDB */
				HIMAGE hImage );		/* ip:	hImage */
										/* rt:	handle of DDB (null if error) */
HIMAGE 	WINAPI 	DIB2Image( 				/* fn:	Conversion from DIB to hImage */
				HANDLE hDib );			/* ip:	handle of DIB */
										/* rt:	hImage (null if error) */
HANDLE 	WINAPI 	Image2DIB( 				/* fn:	Conversion from hImage to DIB */
				HIMAGE hImage );		/* ip:	hImage */
										/* rt:	handle of DIB (null if error) */
BOOL	WINAPI	Bitmap2File( 
				LPSTR	szFileName, 
				HBITMAP hBitmap, 
				WORD	wImgFmt, 
				WORD	wSaveOpt );

HBITMAP	WINAPI	File2Bitmap( 
				LPSTR	szFileName, 
				WORD	wOption );

BOOL	WINAPI	DIB2File( 
				LPSTR	szFileName, 
				HANDLE	hDIB, 
				WORD	wImgFmt, 
				WORD	wSaveOpt );

HANDLE	WINAPI	File2DIB( 
				LPSTR	szFileName, 
				WORD	wOption );

BOOL	WINAPI	RAW2File( 
				LPSTR	szFileName, 
				HANDLE	hRawImg,
				int		nWidth,
				int		nHeight,
				short	nBitsPerSample,
				WORD	wImgFmt, 
				WORD	wSaveOpt );

HANDLE	WINAPI	File2RAW( 
				LPSTR	szFileName, 
				int		*pnWidth, 
				int		*pnHeight, 
				PSHORT	pnBitsPerSample, 
				WORD	wOption );

BOOL	WINAPI	FreeRaw( 
				HANDLE	hRaw );

HIMAGE	WINAPI	Raw2Image( 
				HANDLE	hRaw, 
				int		nWidth, 
				int		nHeight, 
				int		nBitsPerPixel );
HANDLE	WINAPI	Image2Raw( 
				HIMAGE	hImage, 
				int		*pnWidth, 
				int		*pnHeight, 
				int		*pnBitsPerSample );

/* Part II: Get/Set Image Information */
/* part 2-1: FRImgHdl.c: get/set image information */
LPSTR	WINAPI  ImageLockBuffer(		/* fn:	Get pointer of image buffer */
				HIMAGE 	hImage );		/* ip:	Image handle */
										/* rt:	Pointer of image buffer (null if failure) */
BOOL	WINAPI  ImageUnlockBuffer( 		/* fn:	Unlock image buffer (have no use actually) */
				HIMAGE 	hImage );		/* ip:	Image handle */
										/* rt:	Success or failure */
short	WINAPI  ImageGetWidth( 			/* fn:	Get width of specified image */
				HIMAGE 	hImage );		/* ip:	Image handle */
										/* rt:	Image width (-1 if failure) */
short	WINAPI  ImageGetBitsPerSample( 
				HIMAGE 	hImage );

short	WINAPI  ImageGetLength( 
				HIMAGE 	hImage );

short	WINAPI  ImageGetWidthBytes( 
				HIMAGE 	hImage );

LPSTR	WINAPI  ImageGetRowAddr( 
				HIMAGE 	hImage, 
				int 	nRow );

short	WINAPI  ImageGetXResolution( 
				HIMAGE 	hImage );
short	WINAPI  ImageGetYResolution( 
				HIMAGE 	hImage );
short	WINAPI  ImageGetCompressMode( 
				HIMAGE 	hImage );

BOOL	WINAPI  ImageSetXResolution( 
				HIMAGE 	hImage, 
				short 	xRes );
BOOL	WINAPI  ImageSetYResolution( 
				HIMAGE 	hImage, 
				short 	yRes );
BOOL	WINAPI  ImageSetCompressMode( 
				HIMAGE 	hImage, 
				WORD 	nMode );

/* Part III: Image Processing */
/* part 3-1: FRImgHdl.c: basic image processing */
int     WINAPI  ImageEnhance(
				HIMAGE 	hImage, 
				LPRECT 	lpRect);
HIMAGE 	WINAPI	ExtractRectImage( 
				HIMAGE 	hSrcImg, 
				LPRECT 	lpRect );
HIMAGE	WINAPI	ExtractRectImageEx( 
				HIMAGE	hSrcImg, 
				LPRECT	lpRect, 
				DWORD	dwOption );
BOOL 	WINAPI	ClearRectImage( 
				HIMAGE 	hImage, 
				LPRECT 	lpRect );
BOOL 	WINAPI	SmoothRectImage( 
				HIMAGE 	hImage, 
				LPRECT 	lpRegion,
				WORD	wNoiseThresh,	/* max isolate pixels to remove */
				WORD	wHoleThresh );	/* min pixels of hole to fill */
HIMAGE	WINAPI	Cat2Images( 
				HIMAGE	hImage1, 
				HIMAGE	hImage2, 
				DWORD	dwOption );
BOOL	WINAPI	ImageDrawText( 
				HIMAGE	hImage, 
				LPSTR	szTextStr, 
				LPRECT	lpRect, 
				COLORREF nColor, 
				LPSTR	szFontName,	
				short	nFontSize );

/* image analysis */
PSHORT 	WINAPI	GetHorPrjProfile( 
				HIMAGE 	hImage, 
				LPRECT 	lpRect );
PSHORT 	WINAPI	GetVerPrjProfile( 
				HIMAGE 	hImage, 
				LPRECT 	lpRect );
PSHORT 	WINAPI	GetHorCrossingCount( 
				HIMAGE	hImage, 
				LPRECT	lpRegion );
PSHORT 	WINAPI	GetVerCrossingCount( 
				HIMAGE	hImage, 
				LPRECT	lpRegion );
void	WINAPI	FreeProfile( 
				PSHORT	pProfile );

/* part 3-2: ImgCnvrt.c: image conversion */
HIMAGE 	WINAPI	GrayToBin(				// fn:	convert gray-scale(8) to binary(1) image
				HIMAGE 	hImage,			// ip:  IP gray-scale image
				short 	Thresh );		// ip:	-1 => auto threshold 
										//		<0 => means percentage 
										//		Others(>0) => threshold value
										//		-101 => Halftone
										//		-102 => Otsu
										//		-103 => LM
HIMAGE	WINAPI	BinToGray(				// fn:	upgrade binary(1) to gray-scale(8) image
				HIMAGE	hBin );			// ip:	binary image
										// rt:	gray-scale image (NULL if error)
HIMAGE	WINAPI	ColorToGray( 
				HIMAGE	hImage, 
				short	nMethod );		//		see D4 as above

HIMAGE	WINAPI	GrayToColor(			// fn:	upgrade gray-scale(8) to color(24) image
				HIMAGE	hGray );		// ip:	gray-scale image
										// rt:	color image (NULL if error)
HIMAGE 	WINAPI	Std2Fine( 
				HIMAGE 	hImage );

HIMAGE	WINAPI	ImageResample( 
				HIMAGE	hImage, 
				short	nResX, 
				short	nResY );
#define	CCNV_MTHD_MASK		0x0000FF00
#define	CCNV_MTHD_YV12RGB	0x00000100	//YV12 to RGB24
#define CCNV_MTHD_RGBYV12	0x00000200	//RGB24 to YV12
#define CCNV_MTHD_RGBBGR	0x00000300	//RGB24 to BGR24
BOOL	WINAPI	ColorConvert( 
				HANDLE	hSource, 
				HANDLE	hDestinate, 
				int		nImgWidth, 
				int		nImgHeight, 
				DWORD	dwOption );

BOOL	WINAPI	IP_AutoLevel( HIMAGE hImage, LPRECT lpRect, DWORD dwOption );
BOOL	WINAPI	ImageFillRect( HIMAGE hImage, LPRECT lpRect, COLORREF nColor, DWORD dwOption );
HIMAGE	WINAPI	ImageResize( HIMAGE hImage, int nWidth, int nHeight, DWORD dwOption );

/* Part IV: Image Rotation */
/* part 4-1: Rotation.c: image rotation with specified wImgType */
HIMAGE 	WINAPI	ImageRotateEx( 			/* fn:	Image rotation at any angle */
				HIMAGE 	hInImage, 		/* ip:	Source image */
				double 	degree,			/*		Rotation angle (CCW in degree) */
				WORD	wImgType );		/*		Image Type (see definition D3) */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI	ImageRotateEx90(		/* fn:	Image rotation of 90 degree CCW */
				HIMAGE 	hInImage, 		/* ip:	Source image */
				WORD	wImgType );		/*		Image Type (see definition D3) */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI	ImageRotateEx180(		/* fn:	Image rotation of 180 degree */
				HIMAGE 	hInImage, 		/* ip:	Source image */
				WORD	wImgType );		/*		Image Type (see definition D3) */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI	ImageRotateEx270(		/* fn:	Image rotation of 270 degree CCW */
				HIMAGE 	hInImage, 		/* ip:	Source image */
				WORD	wImgType );		/*		Image Type (see definition D3) */
										/* rt:	Rotated image (NULL if failure) */
/* Part 4-2: Rotation.c: image rotation with auto detected wImgType */
HIMAGE 	WINAPI	ImageRotate( 			/* fn:	Same as ImageRotateEx except wImgType is auto detected */
				HIMAGE 	hInImage, 		/* ip:	Source image */
				double 	degree );		/*		Rotation angle (CCW in degree) */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI 	ImageRotate90( 			/* fn:	Same as ImageRotate90 except wImgType is auto detected */
				HIMAGE 	hOldImg );		/* ip:	Source image */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI 	ImageRotate180( 		/* fn:	Same as ImageRotate180 except wImgType is auto detected */  
				HIMAGE 	hOldImg );		/* ip:	Source image */
										/* rt:	Rotated image (NULL if failure) */
HIMAGE 	WINAPI 	ImageRotate270( 		/* fn:	Same as ImageRotate270 except wImgType is auto detected */
				HIMAGE 	hOldImg );		/* ip:	Source image */
										/* rt:	Rotated image (NULL if failure) */
BOOL 	WINAPI	ImageInvert( 			/* fn:	Inverse pixels of image */
				HIMAGE 	hImage );		/* ip:	Source image */
										/* rt:	Success or failure */
BOOL	WINAPI	IsImageInverse( 
				HIMAGE	hImage );

/* Part 5: from Lead Tool */
HANDLE	WINAPI	LdCreateBitmap(			// fn:	create the handle of Lead-bitmap */ 
				WORD	wOption );		// ip:	options (not yet) */
										// rt:	handle of Lead bitmap (pixel map is empty)
HANDLE	WINAPI	LdReadBmp(				// fn:	read particular page from image file */
				LPSTR	szFileName,		// ip:	filename
				WORD	wPage,			//		the page number to read (starting from 0)
				WORD	wOption );		//		options
										// rt:	handle of Lead bitmap
HANDLE	WINAPI	LdReadBmpLst(			/* fn:	read a sequence of images(multi-page) from file */ 
				LPSTR	szFileName );
HANDLE	WINAPI	LdReadBmpFromMemory( 
				LPSTR	pBuffer, 
				DWORD	dwBufLen, 
				WORD	wOption );
void	WINAPI	LdFreeBmp(				// fn:	free bitmap handle
				HANDLE	hLdBmp );
void	WINAPI	LdFreeBmpLst(			// fn:	free the handle of bitmap sequence
				HANDLE	hLdBmpLst );
HIMAGE	WINAPI	LdBmp2Img( 				// fn:	convert Lead bitmap to image 
				HANDLE	hLdBmp );
HANDLE	WINAPI	LdImg2Bmp(				// fn:	convert image to Lead bitmap
				HIMAGE	hImage );
BOOL	WINAPI	LdSaveBmp(				// fn:	save bitmap into file
				HANDLE	hLdBmp, 
				LPSTR	szFileName, 
				WORD	wFormat, 
				WORD	wOption );
DWORD	WINAPI	LdWriteBmpToMemory( 
				HANDLE	hLdBmp, 
				LPSTR	szBuffer, 
				DWORD	dwLen, 
				WORD	wFormat, 
				WORD	wSaveOpt );
BOOL	WINAPI	LdConvertBitmap( 
				HANDLE	hLdBmp, 
				WORD	wOption );
#define	CHANNEL_RGB		0x000F
#define CHANNEL_RGB_R	0x0001
#define CHANNEL_RGB_G	0x0002
#define CHANNEL_RGB_B	0x0004
#define	CHANNEL_CMYK	0x00F0
#define	CHANNEL_CMYK_C	0x0010
#define	CHANNEL_CMYK_M	0x0020
#define	CHANNEL_CMYK_Y	0x0040
#define	CHANNEL_CMYK_K	0x0080
#define CHANNEL_HSV		0x0F00
#define CHANNEL_HSV_H	0x0100
#define CHANNEL_HSV_S	0x0200
#define CHANNEL_HSV_V	0x0400
HIMAGE	WINAPI	LdGetChannelBitmap( 
				HANDLE	hLdBmp, 
				WORD	wChannel );
HIMAGE	WINAPI	LdGetChannelBitmapEx( 
				HANDLE	hLdBmp, 
				WORD	wChannel, 
				BOOL	bToBin );
#define	LD_PRP_WIDTH		0x0001	/* Image width */
#define	LD_PRP_HEIGHT		0x0002	/* Image height */
#define	LD_PRP_BITSPERPIXEL	0x0003	/* Number of bits per pixel */
#define	LD_PRP_BYTESPERLINE	0x0004	/* Bytes per line, rounded by four */
#define	LD_PRP_XRESOLUTION	0x0005	/* Bitmaps X resolution (DPI) */
#define	LD_PRP_YRESOLUTION	0x0006	/* Bitmaps Y resolution (DPI) */
BOOL	WINAPI	LdGetImgProperty( 
				HANDLE	hLdBmp, 
				WORD	wProperty, 
				LPSTR	lpszValue );
BOOL	WINAPI	LdCopyBitmap( 
				HANDLE	pBmpDst, 
				HANDLE	hLdBmp, 
				WORD	wOption );
BOOL	WINAPI	LdCropBitmap( 
				HANDLE	hLdBmp );
HANDLE	WINAPI	LdExtractBitmap( 
				HANDLE	hLdBmp, 
				LPRECT	lpRect );
BOOL	WINAPI	LdGray2Bin( 
				HANDLE	hLdBmp, 
				BYTE	nThresh );
HANDLE	WINAPI	LdDlgReadBmp( 
				HWND	hWnd,
				LPSTR	szFileName,
				WORD	nLen );
BOOL	WINAPI	LdDlgSaveBmp( 
				HWND	hWnd, 
				HANDLE	hLdBmp,
				LPSTR	szFileName,
				WORD	nLen );
BOOL	WINAPI	LdDlgPrintBmp( 
				HANDLE	hLdBmp );
HANDLE	WINAPI	LdCaptureBmp( 
				HWND	hWnd, 
				HDC		hDC );
BOOL	WINAPI	LdBmpInBmp(				// fn:	Picture-in-Picture implementation
				HANDLE	hLdBmpDst,		// ip:	Destination image 
				HANDLE	hLdBmpSrc,		//		Source image
				LPPOINT lpOffset );		//		Offset
BOOL	WINAPI	LdSizeBitmap( 
				HANDLE	hLdBmp, 
				int		nWidth, 
				int		nHeight, 
				DWORD	dwOption );

short	WINAPI	LdSetDesc( 
				LPSTR	szDesc );

short	WINAPI	ImageWriteDesc( 
				LPSTR	szFile, 
				LPSTR	szDesc );
short	WINAPI	ImageReadDesc( 
				LPSTR	szFile, 
				LPSTR	szBuffer, 
				WORD	wLen );
// User-defined tag valued from 0x8000 to 0xFFFE
//0x8298			reserved by LeadTool (Copyright comment)
//0x8769			reserved by LeadTool (General Exif comments)
//0x8825			reserved by LeadTool (Exif GPS comments)
//0x80A4			reserved by LeadTool (Annotation TIFF tag defined as ANNTAG_TIFF.)
//0xB001 - 0xB0FF	coordinate of blocking boxes (max. 255 boxes)
BOOL	WINAPI	ImageWriteRectTag(	/* fn:	write RECT value as user-defined tag into a tif file */
				LPSTR	szFile,		/* ip:	tif file name */ 
				WORD	wTag,		/* ip:	user-defined tag valued from 0x8000 to 0xFFFE */
				LPRECT	lpRect );	/* ip:	RECT value (left,top)(right,bottom) to write */
									/* rt:	success or failure */
BOOL	WINAPI	ImageReadRectTag(	/* fn:	read RECT value as user-defined tag from a tif file */ 
				LPSTR	szFile,		/* ip:	tif file name */ 
				WORD	wTag,		/* ip:	user-defined tag valued from 0x8000 to 0xFFFE */ 
				LPRECT	lpRect );	/* op:	RECT value (left,top)(right,bottom) to read */
									/* rt:	success or failure */

/* prn2img.c */
BOOL	WINAPI	PrnToImg( 
				HWND	hWnd, 
				LPSTR	szImg );
HDC		WINAPI	CreatePrinterHDC( 
				LPSTR	szFile );
void	WINAPI	FreePrinterHDC( 
				HDC		hDC );

BOOL	WINAPI	LdDrawText( 
				HANDLE	hLdBmp, 
				LPSTR	szText, 
				LPRECT	lpRect,
				COLORREF nColor, 
				LPSTR	szFontName,
				short	nFontSize );
BOOL	WINAPI	LdRemoveLine( 
				HANDLE	hLdBmp, 
				WORD	wOption );
BOOL	WINAPI	LdDisplayBitmap( 
				HANDLE	hLdBmp, 
				LPRECT	lpRect, 
				HDC		hDC, 
				short	nWinWidth, 
				short	nWinHeight, 
				DWORD	dwRop );
HANDLE	WINAPI	LdRaw2Bmp( 
				HANDLE	hRawImg, 
				int		nWidth, 
				int		nHeight, 
				short	nBitsPerSample );
HANDLE	WINAPI	LdBmp2Raw( 
				HANDLE	hLdBmp, 
				int		*lpnWidth, 
				int		*lpnHeight, 
				int		*lpnBitsPerPixel );
HIMAGE	WINAPI	CreateErrorImage( 
				LPSTR	szErrStr );
BOOL	WINAPI	DisplayImage( 
				HIMAGE	hImage, 
				LPRECT	lpRect, 
				HDC		hDC, 
				short	nWinWidth, 
				short	nWinHeight, 
				DWORD	dwRop );
BOOL	WINAPI	LdFillRect( 
				HANDLE	hLdBmp, 
				LPRECT	lpRect, 
				COLORREF nColor );

//scan2img.c
HANDLE	WINAPI	CVScanCreate( HWND hWnd, DWORD dwOption );
BOOL	WINAPI	CVScanFree( HANDLE hCvScan );
HIMAGE	WINAPI	CVScanToImg( HANDLE hCvScan );
HANDLE	WINAPI	CVScanToImgLst( HANDLE hCvScan );	//hImgLst
BOOL	WINAPI	CVScanSelect( HANDLE hCvScan );

// other testing
BOOL	WINAPI	PrintTiffTags( LPSTR lpFileName, short nPage, WORD wTag );
BOOL	WINAPI	LdBmp2Hsv( HANDLE hLdBmp, COLORREF nColor );

// advance
BOOL	WINAPI	ImageHookBuffer( HIMAGE hImage, LPSTR lpBuffer );
BOOL	WINAPI	ImageUnhookBuffer( HIMAGE hImage );

//
HIMAGE	WINAPI	ImageTest1( HIMAGE hImage, DWORD dwOption );

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* _OBIMGPRC_H_ */
