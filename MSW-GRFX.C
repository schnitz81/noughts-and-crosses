/*
 *   Noughts And Crosses Game
 *   Copyright (C) 1993,1994 Pentti Moilanen
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "config.h"

#include <windows.h>

#include "0x.h"
#include "type.h"
#include "varext.h"
 
POINT FAR  RandomOrder(POINT, short , BOOL );



short	sGridBitmapSize;
short	sGridLineWidth;
short	sBoardBorderLineWidth;
short	sCrossPosLineWidth;
short	sCrossNegLineWidth;
short	sCirclePosLineWidth;
short	sCircleNegLineWidth;
RECT	rectCrossPosition;
RECT	rectCirclePosition;
RECT	rectClipGridBitmap;
HBRUSH	hPieceBrush[2];
HBRUSH	hBoardBrush[3];

BOOL FAR  Error(short sErrorCode,BOOL bMessageBox)
	{
	char buffer[]="Error occurred, maybe not enough memory";
	if( bMessageBox)
		MessageBox ( hwndMain ,buffer, szAppLongName ,MB_ICONEXCLAMATION );
	PostMessage (hwndMain, WM_CLOSE, 0, 0L);
	return FALSE;
	}

BOOL _near  CreateTimer(HWND hwnd , int nIDEvent , WORD wElapse )
	{
	while ( !SetTimer ( hwnd , nIDEvent , wElapse , NULL ) )
		if ( IDCANCEL == MessageBox ( hwnd ,"Too many clocks or timers!" , szAppLongName ,MB_ICONEXCLAMATION | MB_RETRYCANCEL ) )
			return FALSE;
	return TRUE;
	}


void FAR  FlashBoard(POINT ptNew,short NewFlashingCount,short NewFlashingEnd)
	{
static 	POINT 	Flashing[16];
static	short 	FlashingCount[16];
static	short 	FlashingEnd[16];
static	BOOL	TimerOn=FALSE;
	WORD	*pwBoardPoint;
	BOOL	FoundMoreFlashing=FALSE;
	short	i,Piece;
	HDC  	hdc,hdcMem;


	if ( NewFlashingCount == -1 )
		{			/* stop all flashing ( used in the beginning of a new game ) */
		for(i=0;i<16;i++)
			FlashingCount[i]=0;
		if(TimerOn)
			KillTimer (hwndMain,TIMER_FLASHING);
		TimerOn=FALSE;
		return;
		}

	hdc = GetDC ( hwndMain ) ;



	IntersectClipRect(hdc,rectView.left,rectView.top,rectView.right,rectView.bottom);
					/* Not to bitblt over the border of window */

if((	hdcMem = CreateCompatibleDC(hdc)						)==FALSE)(Error(2,TRUE));
	SetViewportOrg(hdcMem,0,0);	/* to be sure */

	if ( NewFlashingCount > 0 )	/* if there is a new point to flash */
		{
		for(i=0;( ( i < 15 ) && ( FlashingCount[i] != 0 ));i++); /* search a empty slot */
		Flashing[i]=ptNew;
		FlashingCount[i]=2*(NewFlashingCount);
		FlashingEnd[i]=NewFlashingEnd;
		}

	for(i=0;i<16;i++)
		{
		if ( FlashingCount[i] > 0 )
			{
			if ( (FlashingCount[i]--) > 0 ) FoundMoreFlashing=TRUE;
			pwBoardPoint=pwBoard+((Flashing[i].x<<BoardXExp)+(Flashing[i].y<<BoardYExp));
			if ( ( *(pwBoardPoint) & 0xFFFE ) == 0x8FFE )
				Piece=0;
			else if ( ( *(pwBoardPoint+1) & 0xFFFE ) == 0x8FFE )
				Piece=1;
			else
				Piece=UsersPiece;
			SelectObject(hdc,hBoardBrush[1]);
			PatBlt(hdc,Flashing[i].x*sGridSize+1,Flashing[i].y*sGridSize+1,sGridBitmapSize,sGridBitmapSize,PATCOPY);
			SelectObject(hdc,hPieceBrush[Piece]);
			if ( ((FlashingCount[i]+FlashingEnd[i])%2) == 1 ) Piece = Piece+2;
			SelectObject(hdcMem,hBoardBitmap[Piece]);
			BitBlt(hdc,Flashing[i].x*sGridSize+1,Flashing[i].y*sGridSize+1,sGridBitmapSize,sGridBitmapSize,hdcMem,0,0,0xE20746L);
			}
		}

	if ( TimerOn && !FoundMoreFlashing )
		{
		KillTimer (hwndMain,TIMER_FLASHING);
		TimerOn=FALSE;
		}
	if ( !TimerOn && FoundMoreFlashing )
		{
if((		CreateTimer (hwndMain,TIMER_FLASHING,60)				)==FALSE)(Error(10,FALSE));
		TimerOn=TRUE;
		}

	SelectObject(hdc,GetStockObject(NULL_BRUSH));
	DeleteDC( hdcMem );
	SelectClipRgn(hdc,NULL);
	ReleaseDC( hwndMain , hdc );
	}		

BOOL FAR   MakeBoardBitmap(HDC hdc)
	{
	HDC   	hdcMem;
	HPEN 	hPen;
	short i;


	if ( hPieceBrush[0] == 0 )
		{
if((		hPieceBrush[0]=CreateSolidBrush(RGB(NoughtRGB[0],NoughtRGB[1],NoughtRGB[2]))				)==FALSE)(Error(0,TRUE));
		}
	if ( hPieceBrush[1] == 0 )
		{
if((		hPieceBrush[1]=CreateSolidBrush(RGB(CrossRGB[0],CrossRGB[1],CrossRGB[2]))				)==FALSE)(Error(1,TRUE));
		}
	

	if ( hBoardBrush[0] == 0 )
		{
if((		hBoardBrush[0]=CreateSolidBrush(RGB(BoardRGB[0]/2,BoardRGB[1]/2,BoardRGB[2]/2))				)==FALSE)(Error(1,TRUE));
		}
	if ( hBoardBrush[1] == 0 )
		{
if((		hBoardBrush[1]=CreateSolidBrush(RGB(BoardRGB[0],BoardRGB[1],BoardRGB[2]))				)==FALSE)(Error(1,TRUE));
		}
	if ( hBoardBrush[2] == 0 )
		{
if((		hBoardBrush[2]=CreateSolidBrush(RGB(255,255,255))				)==FALSE)(Error(1,TRUE));
		}
	


if((	hdcMem = CreateCompatibleDC(hdc)									)==FALSE)return(Error(20,TRUE));

	SelectObject(hdcMem,GetStockObject(NULL_PEN));
	

	for(i=0;i<4;i++)	
		{
		if (hBoardBitmap[i] == 0 )
			{
if((			hBoardBitmap[i] = CreateBitmap(MAX_GRID_SIZE,MAX_GRID_SIZE,1,1,NULL)	)==FALSE)return(Error(21,TRUE));
			}
		SelectObject(hdcMem,hBoardBitmap[i]);
		if ( i < 2 )
			PatBlt(hdcMem,0,0,MAX_GRID_SIZE,MAX_GRID_SIZE,BLACKNESS);
		else
			PatBlt(hdcMem,0,0,MAX_GRID_SIZE,MAX_GRID_SIZE,WHITENESS);
		}
	for(i=1;i<4;i=i+2)
		{
		if (i==1)
			{
if((			hPen=CreatePen(PS_SOLID,sCrossPosLineWidth,RGB(255,255,255))			)==FALSE)return(Error(23,TRUE));
			}
		else
			{
if((			hPen=CreatePen(PS_SOLID,sCrossNegLineWidth,RGB(0,0,0))			)==FALSE)return(Error(24,TRUE));
			}
		SelectObject(hdcMem,hBoardBitmap[i]);
		SelectObject(hdcMem,hPen);
		IntersectClipRect(hdcMem,rectClipGridBitmap.left,rectClipGridBitmap.top,rectClipGridBitmap.right,rectClipGridBitmap.bottom);
		MoveTo(hdcMem,rectCrossPosition.left,rectCrossPosition.top);
		LineTo(hdcMem,rectCrossPosition.right,rectCrossPosition.bottom);
		MoveTo(hdcMem,rectCrossPosition.right,rectCrossPosition.top);
		LineTo(hdcMem,rectCrossPosition.left,rectCrossPosition.bottom);
		SelectObject(hdcMem,GetStockObject(NULL_PEN));
		DeleteObject(hPen);
		}

	for(i=0;i<4;i=i+2)
		{
		if (i==0)
			{
if((			hPen=CreatePen(PS_SOLID|PS_INSIDEFRAME,sCirclePosLineWidth,RGB(255,255,255))			)==FALSE)return(Error(25,TRUE));
			}
		else
			{
if((			hPen=CreatePen(PS_SOLID|PS_INSIDEFRAME,sCircleNegLineWidth,RGB(0,0,0))			)==FALSE)return(Error(26,TRUE));
			}
		SelectObject(hdcMem,hBoardBitmap[i]);
		SelectObject(hdcMem,GetStockObject(NULL_BRUSH));
		SelectObject(hdcMem,hPen);
		IntersectClipRect(hdcMem,rectClipGridBitmap.left,rectClipGridBitmap.top,rectClipGridBitmap.right,rectClipGridBitmap.bottom);
		Ellipse(hdcMem,rectCirclePosition.left,rectCirclePosition.top,rectCirclePosition.right,rectCirclePosition.bottom);
		SelectObject(hdcMem,GetStockObject(NULL_PEN));
		DeleteObject(hPen);
		}
   	DeleteDC( hdcMem );
   	return (TRUE );
	}




void _near	DisplayBorder(HDC hdc) 
	 {

	 SelectObject(hdc,hBoardBrush[1]);
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.top-sBorderWidth*4,
		   rectView.right-rectView.left+sBorderWidth*8,sBorderWidth*4,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.top-sBorderWidth*4,
		   sBorderWidth*4,rectView.bottom-rectView.top+sBorderWidth*8,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.bottom,
		   rectView.right-rectView.left+sBorderWidth*8,sBorderWidth*4,PATCOPY);	 
	 PatBlt(hdc,rectView.right,rectView.top-sBorderWidth*4,
		   sBorderWidth*4,rectView.bottom-rectView.top+sBorderWidth*8,PATCOPY); 

	 SelectObject(hdc,hBoardBrush[0]);
	 PatBlt(hdc,rectView.left-sBorderWidth,rectView.top-sBorderWidth,
		   rectView.right-rectView.left+sBorderWidth*2,sBorderWidth,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth,rectView.top-sBorderWidth,
		   sBorderWidth,rectView.bottom-rectView.top+sBorderWidth*2,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.bottom+sBorderWidth*3,
		   rectView.right-rectView.left+sBorderWidth*8,sBorderWidth,PATCOPY);	 
	 PatBlt(hdc,rectView.right+sBorderWidth*3,rectView.top-sBorderWidth*4,
		   sBorderWidth,rectView.bottom-rectView.top+sBorderWidth*8,PATCOPY); 
	
	 SelectObject(hdc,hBoardBrush[2]);
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.top-sBorderWidth*4,
		   rectView.right-rectView.left+sBorderWidth*15/2,sBorderWidth,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth*4,rectView.top-sBorderWidth*4,
		   sBorderWidth,rectView.bottom-rectView.top+sBorderWidth*15/2,PATCOPY);	 
	 PatBlt(hdc,rectView.left-sBorderWidth/2,rectView.bottom,
		   rectView.right-rectView.left+sBorderWidth,sBorderWidth,PATCOPY);	 
	 PatBlt(hdc,rectView.right,rectView.top-sBorderWidth/2,
		   sBorderWidth,rectView.bottom-rectView.top+sBorderWidth,PATCOPY);
	 SelectObject(hdc,GetStockObject(NULL_BRUSH));
	 
	}		



void FAR  CleanUp(void)
	{
	short i;
	for(i=0;i<2;i++)
		if (hPieceBrush[i] != 0 ) DeleteObject(hPieceBrush[i]);
	for(i=0;i<3;i++)
		if (hBoardBrush[i] != 0 ) DeleteObject(hBoardBrush[i]);
	for(i=0;i<4;i++)
		if (hBoardBitmap[i] != 0 ) DeleteObject(hBoardBitmap[i]);
	}

void FAR  Paint(PAINTSTRUCT *ps)
	{
	HDC		hdcMem;
	RECT		rectTemp;
	unsigned int 	i;
	unsigned int	sSize,sSize2;
	POINT		ptBase,ptRandom;
	short		Piece;
	WORD		*pwBoardPoint;

	DisplayBorder(ps->hdc);

	IntersectClipRect(ps->hdc,rectView.left,rectView.top,rectView.right,rectView.bottom);
	
	hdcMem = CreateCompatibleDC(ps->hdc);
	SetViewportOrg(hdcMem,0,0);




	rectTemp.left=max(ps->rcPaint.left/sGridSize,rectBoard.left);
	rectTemp.right=min(ps->rcPaint.right/sGridSize+1,rectBoard.right);
	rectTemp.top=max(ps->rcPaint.top/sGridSize,rectBoard.top);
	rectTemp.bottom=min(ps->rcPaint.bottom/sGridSize+1,rectBoard.bottom);

	SelectObject(ps->hdc,hBoardBrush[1]);
	PatBlt(ps->hdc,rectTemp.left*sGridSize,rectTemp.top*sGridSize,(rectTemp.right-rectTemp.left+1)*sGridSize,(rectTemp.bottom-rectTemp.top+1)*sGridSize,PATCOPY);

	SelectObject(ps->hdc,hBoardBrush[0]);
	if ( ps->rcPaint.bottom > (rectBoardView.bottom+sGridSize) ) 
		PatBlt(ps->hdc,rectBoardView.left-sBoardBorderLineWidth,rectBoardView.bottom+sGridSize,rectBoardView.right-rectBoardView.left+sGridSize+2*sBoardBorderLineWidth,sBoardBorderLineWidth,PATCOPY);
	if ( ps->rcPaint.right > (rectBoardView.right+sGridSize) ) 
		PatBlt(ps->hdc,rectBoardView.right+sGridSize,rectBoardView.top-sBoardBorderLineWidth,sBoardBorderLineWidth,rectBoardView.bottom-rectBoardView.top+sGridSize+2*sBoardBorderLineWidth,PATCOPY);

	SelectObject(ps->hdc,hBoardBrush[2]);
	if ( ps->rcPaint.top < rectBoardView.top ) 
		PatBlt(ps->hdc,rectBoardView.left-sBoardBorderLineWidth,rectBoardView.top-sBoardBorderLineWidth,rectBoardView.right-rectBoardView.left+sGridSize+2*sBoardBorderLineWidth,sBoardBorderLineWidth,PATCOPY);
	if ( ps->rcPaint.left < rectBoardView.left ) 
		PatBlt(ps->hdc,(rectBoardView.left)-sBoardBorderLineWidth,rectBoard.top*sGridSize-sBoardBorderLineWidth,sBoardBorderLineWidth,rectBoardView.bottom-rectBoardView.top+sGridSize+2*sBoardBorderLineWidth,PATCOPY);

	SelectObject(ps->hdc,hBoardBrush[0]);
	if ( ps->rcPaint.bottom > (rectBoardView.bottom+sGridSize) ) 
		{
		PatBlt(ps->hdc,rectBoardView.left-(sBoardBorderLineWidth)/3,rectBoardView.bottom+sGridSize+sBoardBorderLineWidth/3,sBoardBorderLineWidth-(sBoardBorderLineWidth)/3,sBoardBorderLineWidth-(sBoardBorderLineWidth)/3,PATCOPY);
		PatBlt(ps->hdc,rectBoardView.left-(sBoardBorderLineWidth*2)/3,rectBoardView.bottom+sGridSize+(sBoardBorderLineWidth*2)/3,sBoardBorderLineWidth-(sBoardBorderLineWidth*2)/3,sBoardBorderLineWidth-(sBoardBorderLineWidth*2)/3,PATCOPY);
		}
	if ( ps->rcPaint.right > (rectBoardView.right+sGridSize) )
		{ 
		PatBlt(ps->hdc,rectBoardView.right+sGridSize+sBoardBorderLineWidth/3,rectBoardView.top-sBoardBorderLineWidth/3,sBoardBorderLineWidth-sBoardBorderLineWidth/3,sBoardBorderLineWidth-sBoardBorderLineWidth/3,PATCOPY);
		PatBlt(ps->hdc,rectBoardView.right+sGridSize+(sBoardBorderLineWidth*2)/3,rectBoardView.top-(sBoardBorderLineWidth*2)/3,sBoardBorderLineWidth-(sBoardBorderLineWidth*2)/3,sBoardBorderLineWidth-(sBoardBorderLineWidth*2)/3,PATCOPY);
		}
	for(i=rectTemp.left+1;i<=rectTemp.right;i++)
		PatBlt(ps->hdc,i*sGridSize-sGridLineWidth/2,rectTemp.top*sGridSize,sGridLineWidth,(rectTemp.bottom-rectTemp.top+1)*sGridSize,BLACKNESS);
	for(i=rectTemp.top+1;i<=rectTemp.bottom;i++)
		PatBlt(ps->hdc,rectTemp.left*sGridSize,i*sGridSize-sGridLineWidth/2,(rectTemp.right-rectTemp.left+1)*sGridSize,sGridLineWidth,BLACKNESS);

	rectTemp.left=max(ps->rcPaint.left/sGridSize,rectUsed.left);
	rectTemp.right=min(ps->rcPaint.right/sGridSize+1,rectUsed.right);
	rectTemp.top=max(ps->rcPaint.top/sGridSize,rectUsed.top);
	rectTemp.bottom=min(ps->rcPaint.bottom/sGridSize+1,rectUsed.bottom);

	sSize=4;
	for(i=4;i<=64;i=i*2)
		if ( max(rectTemp.right-rectTemp.left,rectTemp.bottom-rectTemp.top) >= i ) sSize = 2*i;

	sSize2=sSize*sSize;
	ptBase.x=(rectTemp.left);
	ptBase.y=(rectTemp.top);


	RandomOrder(ptBase,sSize,1);

	for (i=0;i++<=sSize2;)
		{
		ptRandom = RandomOrder(ptBase,sSize,0);
		if	( 
				( ptRandom.x>=rectTemp.left ) 
			&&	( ptRandom.x<=rectTemp.right ) 
			&& 	( ptRandom.y>=rectTemp.top ) 
			&& 	( ptRandom.y<=rectTemp.bottom ) 
			)
			{

			pwBoardPoint=pwBoard+(ptRandom.x<<BoardXExp)+(ptRandom.y<<BoardYExp);
			if 	( *(pwBoardPoint) == 0x8FFF )
				Piece=0;
			else if ( *(pwBoardPoint+1) == 0x8FFF )
				Piece=1;
			else if ( *(pwBoardPoint) == 0x8FFE )
				Piece=2;
			else if ( *(pwBoardPoint+1) == 0x8FFE )
				Piece=3;
			else if ( (ptUserNew.x == ptRandom.x) && (ptUserNew.y == ptRandom.y) ) 
				Piece=UsersPiece;
			else	continue;

			SelectObject(hdcMem,hBoardBitmap[Piece]);
			SelectObject(ps->hdc,hPieceBrush[Piece%2]);
			BitBlt(ps->hdc,ptRandom.x*sGridSize+1,ptRandom.y*sGridSize+1,sGridBitmapSize,sGridBitmapSize,hdcMem,0,0,0xE20746L);
			}
		}

	
	SelectObject(ps->hdc,GetStockObject(NULL_BRUSH));
	DeleteDC ( hdcMem );
	SelectClipRgn(ps->hdc,NULL);

	}

void FAR UpdateGridSize(void)
   	{
	if ( sGridSize >= 32 )
		sGridLineWidth=2;
	else 
		sGridLineWidth=1;
	sGridBitmapSize=sGridSize-sGridLineWidth;
	sBoardBorderLineWidth=sGridSize/4;

	sCrossPosLineWidth=max(3,sGridSize/4);
	sCrossNegLineWidth=sGridSize/6;
	sCirclePosLineWidth=max(4,sGridSize/4);
	sCircleNegLineWidth=sGridSize/6;

	rectCrossPosition.left= 0;
	rectCrossPosition.right= sGridBitmapSize;
	rectCrossPosition.top= 0;
	rectCrossPosition.bottom= sGridBitmapSize;

	rectCirclePosition.left= sGridBitmapSize/16-(sGridBitmapSize<8);
	rectCirclePosition.right= sGridBitmapSize-sGridBitmapSize/16+(sGridBitmapSize<8);
	rectCirclePosition.top= sGridBitmapSize/16-(sGridBitmapSize<8);
	rectCirclePosition.bottom= sGridBitmapSize-sGridBitmapSize/16+(sGridBitmapSize<8);
	

	rectClipGridBitmap.left = sGridSize/16;
	rectClipGridBitmap.right = sGridBitmapSize-sGridSize/16;
	rectClipGridBitmap.top = sGridSize/16;
	rectClipGridBitmap.bottom = sGridBitmapSize-sGridSize/16;
	}


void FAR UpdateWindowSize(HWND hwnd, RECT rectClient)
   	{
	HDC hdc;

	if ( sGridSize > 31 ) sGridLineWidth=2;
	else sGridLineWidth=1;
	sGridBitmapSize=sGridSize-sGridLineWidth;
	sBoardBorderLineWidth=sGridSize/4;

	rectView.top  =rectClientView.top + sYClientOrigin;
	rectView.bottom =rectClientView.bottom + sYClientOrigin;
	rectView.left =rectClientView.left + sXClientOrigin;
	rectView.right =rectClientView.right + sXClientOrigin;

	
	rectBoardView.left= rectBoard.left*sGridSize;
	rectBoardView.right= rectBoard.right*sGridSize;
	rectBoardView.top= rectBoard.top*sGridSize;
	rectBoardView.bottom= rectBoard.bottom*sGridSize;



	rectClientView.top = rectClient.top +(sBorderWidth << 2);
	rectClientView.bottom = rectClient.bottom -(sBorderWidth << 2 );
	rectClientView.left = rectClient.left + (sBorderWidth << 2);
	rectClientView.right = rectClient.right -(sBorderWidth << 2);

	if ((rectClientView.right-rectClientView.left)>((rectBoard.right+2)*sGridSize-rectBoard.left*sGridSize))
	 	{
	 	sXClientOrigin = ((rectBoard.right+1+rectBoard.left)*sGridSize)/2-(rectClientView.right-rectClientView.left)/2-(sBorderWidth<<2);
	 	SetScrollRange(hwnd,SB_HORZ,sXClientOrigin,sXClientOrigin,FALSE);
	 	}
	else
	 	{
	 	sXClientOrigin = min((rectBoard.right+3)*sGridSize-(rectClientView.right-rectClientView.left)-(sBorderWidth<<2),max((rectBoard.left-2)*sGridSize-(sBorderWidth<<2),sXClientOrigin));
	 	SetScrollRange(hwnd,SB_HORZ,(rectBoard.left-2)*sGridSize-(sBorderWidth<<2),(rectBoard.right+3)*sGridSize-(rectClientView.right-rectClientView.left)-(sBorderWidth<<2),FALSE);
	 	}

	if ((rectClientView.bottom-rectClientView.top)>((rectBoard.bottom+2)*sGridSize-rectBoard.top*sGridSize))
	 	{
	 	sYClientOrigin = ((rectBoard.bottom+1+rectBoard.top)*sGridSize)/2-(rectClientView.bottom-rectClientView.top)/2-(sBorderWidth<<2);
	 	SetScrollRange(hwnd,SB_VERT,sYClientOrigin,sYClientOrigin,FALSE);
	 	}
	else
	 	{
	 	sYClientOrigin = min((rectBoard.bottom+3)*sGridSize-(rectClientView.bottom-rectClientView.top)-(sBorderWidth<<2),max((rectBoard.top-2)*sGridSize-(sBorderWidth<<2),sYClientOrigin));
	 	SetScrollRange(hwnd,SB_VERT,(rectBoard.top-2)*sGridSize-(sBorderWidth<<2),(rectBoard.bottom+3)*sGridSize-(rectClientView.bottom-rectClientView.top)-(sBorderWidth<<2),FALSE);
	 	}
	SetScrollPos(hwnd,SB_VERT,sYClientOrigin,TRUE);
	SetScrollPos(hwnd,SB_HORZ,sXClientOrigin,TRUE);

	rectView.top  =rectClientView.top + sYClientOrigin;
	rectView.bottom =rectClientView.bottom + sYClientOrigin;
	rectView.left =rectClientView.left + sXClientOrigin;
	rectView.right =rectClientView.right + sXClientOrigin;

	hdc=GetDC(hwnd);

	SetViewportOrg(hdc,-sXClientOrigin,-sYClientOrigin);

	ReleaseDC(hwnd,hdc);

	}
