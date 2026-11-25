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


char	szAppLongName[]		= "Noughts And Crosses";
char	szAppShortName[]	= "0x";
char	szVersionNumber[]	= "1.1";
char	szHelpFileName[]	="0x.hlp";
char	szThinking[]		="X - X  Noughts and Crosses - Thinking ";
char	szYourTurn[]		="X - X  Noughts and Crosses - Your turn";
char	szIWon[]		="X - X  Noughts and Crosses - You lost ";
char	szYouWon[]		="X - X  Noughts and Crosses - You won  ";
char	szDraw[]		="X - X  Noughts and Crosses - It is a draw";

#ifdef MSWINDOWS
HANDLE	hInst;
HANDLE	hAccelMain ;
HWND	hwndMain;
HBITMAP	hBoardBitmap[4];
#endif

RECT	rectUsed,rectBoard,rectBoardView;
RECT	rectView,rectClient,rectClientView;
short	sXClientOrigin,sYClientOrigin;
BOOL	bGameGoing		=FALSE;
short	sGameCount;
short	sSkillLevel		=5;
short	sBorderWidth		=4;
short	sGridSize		=16;


short	*pwBoard;

BOOL	bComputersFirstMove=0;
short	ComputersPiece=0;
short	UsersPiece=1;
short	sUserWins;
short	sUserLosses;

BOOL	bProcessing=FALSE;
BOOL	bBackgroundProcessingEnabled=TRUE;
BOOL	bBackgroundProcessing=FALSE;
POINT	pointBackgroundProcessing;
BOOL	bSendMessageBackgroundResponse=FALSE;
BOOL	bInterruptBackgroundProcessing=FALSE;
POINT	ptUserNew;

short	CrossRGB[3];
short	NoughtRGB[3];
short	BoardRGB[3];

POINT	ptAlreadyFoundRowEnds[16][2][2];
short	sAlreadyFoundRows[2];
short	sCreditRowLength=5;	/* YOU CANNOT CHANGE THIS */
short	sGameEndRows=1;	

short	sMasterAttackEvaluateMin[8] ={ 4,1,1,1,1,1,1,1};
short	sMasterAttackEvaluateMax[8] ={16,16,12,8,8,8,8,6};
short	sMasterLosersEvaluateMax[8]={ 4,4,4,1,1,1,1,1};
short	sMasterDefenceEvaluateMax[8]={16,16,12,6,6,6,6,4};

/*
short	sMasterAttackEvaluateMin[8] ={ 4,1,1,1,1,1,1,1};
short	sMasterAttackEvaluateMax[8] ={16,16,8,8,8,8,8,6};
short	sMasterLosersEvaluateMax[8]={ 4,4,1,1,1,1,1,1};
short	sMasterDefenceEvaluateMax[8]={16,16,8,6,6,6,6,4};
*/

short   sAlgTest=1;
short   sGameWinSureCount[2];
short   sGameWinSureFirst[2];
short   sGameLoseSureCount[2];
short   sGameLoseSureFirst[2];
unsigned long   sAttackCount[2];
unsigned long   sDefenceCount[2];

/*
#ifndef MSWINDOWS
FILE *debugfp;
#endif
*/



















