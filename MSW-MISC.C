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
#include <stdlib.h>

#include "0x.h"
#include "type.h"
#include "varext.h"
#include "macro.h"




char	szInitFileName[]="0x.ini";
char	szInitAppName[]="0x";
char	szBackgroundText[]="BackgroundProcessing";
char	szSkillLevelText[]="SkillLevel";
char	szGridSizeText[]="GridSize";
char	szComputersFirstMoveText[]="FirstMove";
char	szUsersPieceText[]="UsersPiece";
char	szGameEndRowsText[]="GameEndRows";

char	szCrossRText[]="CrossR";
char	szCrossGText[]="CrossG";
char	szCrossBText[]="CrossB";
char	szNoughtRText[]="NoughtR";
char	szNoughtGText[]="NoughtG";
char	szNoughtBText[]="NoughtB";
char	szBoardRText[]="BoardR";
char	szBoardGText[]="BoardG";
char	szBoardBText[]="BoardB";




void FAR  ReadInitFile(HWND hwnd)
	{
	short sCXScreen;
	HDC	hdc;

	sCXScreen = GetSystemMetrics(SM_CXSCREEN);
	if ( sCXScreen >= 1024 )
		{
		sBorderWidth=3;
		sGridSize=32;
		}
	else if ( sCXScreen >= 800 )
		{
		sBorderWidth=2;
		sGridSize=24;
		}
	else if ( sCXScreen >= 600 )
		{
		sBorderWidth=2;
		sGridSize=16;
		}
	else if ( sCXScreen >= 400 )
		{
		sBorderWidth=1;
		sGridSize=12;
		}
	else
		{
		sBorderWidth=1;
		sGridSize=10;
		}

	hdc=GetDC(hwnd);
	if ( GetDeviceCaps(hdc,NUMCOLORS) < 4 )
		{
		CrossRGB[0]=0;
		CrossRGB[1]=0;
		CrossRGB[2]=64;
		NoughtRGB[0]=0;
		NoughtRGB[1]=0;
		NoughtRGB[2]=0;
		BoardRGB[0]=192;
		BoardRGB[1]=192;
		BoardRGB[2]=192;
		}
	else
		{
		CrossRGB[0]=0;
		CrossRGB[1]=0;
		CrossRGB[2]=255;
		NoughtRGB[0]=255;
		NoughtRGB[1]=0;
		NoughtRGB[2]=0;
		BoardRGB[0]=192;
		BoardRGB[1]=192;
		BoardRGB[2]=192;
		}
	ReleaseDC(hwnd,hdc);

	sGridSize			=GetPrivateProfileInt(szInitAppName,szGridSizeText,sGridSize,szInitFileName);
 	sGridSize			= min(MAX_GRID_SIZE,max(MIN_GRID_SIZE,sGridSize));
	sSkillLevel			=GetPrivateProfileInt(szInitAppName,szSkillLevelText,0,szInitFileName);
 	sSkillLevel			= min(5,max(0,sSkillLevel));
	bComputersFirstMove		=GetPrivateProfileInt(szInitAppName,szComputersFirstMoveText,2,szInitFileName);
 	bComputersFirstMove		= min(3,max(0,bComputersFirstMove));
	UsersPiece			=GetPrivateProfileInt(szInitAppName,szUsersPieceText,0,szInitFileName);
 	UsersPiece			= min(1,max(0,UsersPiece));
	ComputersPiece			=1-UsersPiece;
	bBackgroundProcessingEnabled	=GetPrivateProfileInt(szInitAppName,szBackgroundText,1,szInitFileName);
 	bBackgroundProcessingEnabled	= min(1,max(0,bBackgroundProcessingEnabled));
	sGameEndRows			=GetPrivateProfileInt(szInitAppName,szGameEndRowsText,1,szInitFileName);
 	sGameEndRows			= min(9,max(1,sGameEndRows));

	CrossRGB[0]			=GetPrivateProfileInt(szInitAppName,szCrossRText,CrossRGB[0],szInitFileName);
	CrossRGB[0]			=min(255,max(0,CrossRGB[0]));
	CrossRGB[1]			=GetPrivateProfileInt(szInitAppName,szCrossGText,CrossRGB[1],szInitFileName);
	CrossRGB[1]			=min(255,max(0,CrossRGB[1]));
	CrossRGB[2]			=GetPrivateProfileInt(szInitAppName,szCrossBText,CrossRGB[2],szInitFileName);
	CrossRGB[2]			=min(255,max(0,CrossRGB[2]));
	NoughtRGB[0]			=GetPrivateProfileInt(szInitAppName,szNoughtRText,NoughtRGB[0],szInitFileName);
	NoughtRGB[0]			=min(255,max(0,NoughtRGB[0]));
	NoughtRGB[1]			=GetPrivateProfileInt(szInitAppName,szNoughtGText,NoughtRGB[1],szInitFileName);
	NoughtRGB[1]			=min(255,max(0,NoughtRGB[1]));
	NoughtRGB[2]			=GetPrivateProfileInt(szInitAppName,szNoughtBText,NoughtRGB[2],szInitFileName);
	NoughtRGB[2]			=min(255,max(0,NoughtRGB[2]));
	BoardRGB[0]			=GetPrivateProfileInt(szInitAppName,szBoardRText,BoardRGB[0],szInitFileName);
	BoardRGB[0]			=min(255,max(0,BoardRGB[0]));
	BoardRGB[1]			=GetPrivateProfileInt(szInitAppName,szBoardGText,BoardRGB[1],szInitFileName);
	BoardRGB[1]			=min(255,max(0,BoardRGB[1]));
	BoardRGB[2]			=GetPrivateProfileInt(szInitAppName,szBoardBText,BoardRGB[2],szInitFileName);
	BoardRGB[2]			=min(255,max(0,BoardRGB[2]));
	}


void FAR  WriteInitFile(void)
	{
	char buffer[4];

	*_itoa(sSkillLevel,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szSkillLevelText,buffer,szInitFileName);
	*_itoa(sGridSize,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szGridSizeText,buffer,szInitFileName);
	*_itoa(bComputersFirstMove,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szComputersFirstMoveText,buffer,szInitFileName);
	*_itoa(UsersPiece,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szUsersPieceText,buffer,szInitFileName);
	*_itoa(bBackgroundProcessingEnabled,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szBackgroundText,buffer,szInitFileName);
	*_itoa(sGameEndRows,&buffer[0],10);
	WritePrivateProfileString(szInitAppName,szGameEndRowsText,buffer,szInitFileName);
	}





POINT FAR  RandomOrder(POINT ptBase, short sSize, BOOL bInitialize)
	{
	static	WORD	wShiftReg;
	static	short	sRegLength;
	static	short 	sRegTap;
	static	short 	sHalfLength;
	static  short   wTestPattern;
	static  short	i;
	static  POINT   point;

	if ( bInitialize )
		{
		srand((unsigned)GetCurrentTime());

		wShiftReg=0;

		for(i=1;i<16;i++)
			wShiftReg=wShiftReg | ( ( 0x0001 & GetRandom(0,1) ) << i );

		switch (sSize)
			{
			case 4: 
					sRegLength=5;
					sRegTap=2;
					wTestPattern=0x0001;
					sHalfLength=2;
					break;
			case 8: 
					sRegLength=7;
					sRegTap=3;
					wTestPattern=0x0001;
					sHalfLength=3;
					break;
			case 16: 
					sRegLength=9;
					sRegTap=4;
					wTestPattern=0x0001;
					sHalfLength=4;
					break;
			case 32: 
					sRegLength=11;
					sRegTap=2;
					wTestPattern=0x0001;
					sHalfLength=5;
					break;
			case 64: 
					sRegLength=15;
					sRegTap=14;
					wTestPattern=0x0007;
					sHalfLength=6;
					break;
			case 128: 
					sRegLength=15;
					sRegTap=4;
					wTestPattern=0x0001;
					sHalfLength=7;
					break;
			}
		}
	
	do
		{
		wShiftReg = wShiftReg << 1;
	
		wShiftReg =   ( wShiftReg & 0xFFFE )
					^ ( ( wShiftReg & (  0x0001 << sRegTap    ) ) >> sRegTap )
					^ ( ( wShiftReg & (  0x0001 << sRegLength ) ) >> sRegLength )
					^ 0x0001 ;
		}
	while ( ( wShiftReg & wTestPattern ) != 0 );
	
	point.x = ptBase.x + (( wShiftReg & ( (sSize-1) << (sRegLength-sHalfLength) ) ) >> (sRegLength-sHalfLength) );
	point.y = ptBase.y + (( wShiftReg & ( (sSize-1) << (sRegLength-2*sHalfLength) ) ) >> (sRegLength-2*sHalfLength) );

	return(point);
	}


