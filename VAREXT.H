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


extern	char 	szAppLongName[];
extern	char	szAppShortName[];
extern	char	szVersionNumber[];
extern	char	szThinking[];
extern	char	szYourTurn[];
extern	char	szIWon[];
extern	char	szYouWon[];
extern	char	szDraw[];

#ifdef MSWINDOWS
extern	HANDLE	hInst;
extern	HANDLE  hAccelMain ;
extern	HWND    hwndMain;
extern	HBITMAP hBoardBitmap[4];
#endif

extern	RECT	rectUsed,rectBoard,rectBoardView;
extern	RECT	rectView,rectClient,rectClientView;
extern	short	sXClientOrigin,sYClientOrigin;
extern	BOOL	bGameGoing;
extern	short	sGameCount;
extern	short	sSkillLevel;
extern	short	sBorderWidth;
extern	short	sGridSize;

extern	short	*pwBoard;

extern	BOOL	bComputersFirstMove;
extern	short	ComputersPiece;
extern	short	UsersPiece;
extern	short	sUserWins;
extern	short	sUserLosses;

extern	BOOL	bProcessing;
extern	BOOL	bBackgroundProcessingEnabled;
extern	BOOL	bBackgroundProcessing;
extern	POINT	pointBackgroundProcessing;
extern	BOOL 	bSendMessageBackgroundResponse;
extern	BOOL	bInterruptBackgroundProcessing;
extern	POINT	ptUserNew;

extern	short	CrossRGB[3];
extern	short	NoughtRGB[3];
extern	short	BoardRGB[3];
extern	POINT	ptAlreadyFoundRowEnds[16][2][2];
extern	short	sAlreadyFoundRows[2];
extern	short	sCreditRowLength;
extern	short	sGameEndRows;	
extern	short	sMasterAttackEvaluateMin[8];
extern	short	sMasterAttackEvaluateMax[8];
extern	short	sMasterLosersEvaluateMax[8];
extern	short	sMasterDefenceEvaluateMax[8];

extern  short   sAlgTest;
extern  short   sGameWinSureCount[2];
extern  short   sGameWinSureFirst[2];
extern  short   sGameLoseSureCount[2];
extern  short   sGameLoseSureFirst[2];
extern  unsigned long   sAttackCount[2];
extern  unsigned long   sDefenceCount[2];
#ifndef MSWINDOWS
extern  FILE *debugfp;
#endif




