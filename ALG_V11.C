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

 
#ifdef MSWINDOWS
#include <windows.h>
#endif

#ifndef MSWINDOWS
#include<stdio.h>
#endif


#include <stdlib.h>
#include "0x.h"
#include "type.h"
#include "varext.h" 
#include "macro.h" 


POINT RandomOrder(POINT, short , BOOL );

#ifdef MSWINDOWS
void FAR FlashBoard(POINT,short,short);
#endif


#define WinValue                (  2000-sLevelCount )
#define LoserValue             ( -2000+sLevelCount )
#define UnDefined               ( 0 )
#define TurnAroundResult(vp)    ( vp.value = -vp.value )

short   sUpdateValueMin;


	POINT   ptLastPutPiece;
	long    lCounter;
	short   sAttackValueMin[32];
	short   sDefenceValueMin[32];
	short   sAttackEvaluateMin[32];
	short   sAttackEvaluateMax[32];
	short   sLosersEvaluateMax[32];
	short   sDefenceEvaluateMax[32];
	short   sDepth;
	short	NearValue[16][2];
	short	FarValue[16][2];
	VALUEPOINT	BackgroundResult[32][2];


#ifndef MSWINDOWS
	BOOL PeekForAWindowsMessage(void)
		{
		return(FALSE);
		}



#endif

#ifdef DEBUG
void Error(void) 
{
MessageBeep(0);
}
#endif


BOOL  
#ifdef MSWINDOWS
FAR  
#endif
AllocateMemory(void)
{	if ( pwBoard == 0 )
		pwBoard=(short *)malloc(
			 (BOARD_SIZE+2*BOARD_SAFETY+1)
			*(BOARD_SIZE+2*BOARD_SAFETY)
			*2*sizeof(short));

	if ( pwBoard == 0 ) 
		return(FALSE);
	else
		return(TRUE);

}

void 
#ifdef MSWINDOWS
FAR  
#endif
DeAllocateMemory(void)
{
free(pwBoard);
}



#ifdef MSWINDOWS
/*
 * PeekForAWindowsMessage(void)
 *
*/
	BOOL __near __fastcall PeekForAWindowsMessage(void)
		{
		MSG             msg;
		if ( !bInterruptBackgroundProcessing )
			{
			if ( PeekMessage(&msg,NULL,0,0,PM_REMOVE) ) 
				{
				if (!TranslateAccelerator (hwndMain, hAccelMain, &msg ))
					{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
					}
				}
			}
		return ( bInterruptBackgroundProcessing );
		}
#endif





BOOL
#ifdef MSWINDOWS
FAR  
#endif
InitAlgorithmNewGame(void)
	{
	short x,y,piece;

	for(x=0 ; x<(BOARD_SIZE+2*BOARD_SAFETY) ; x++)
		for(y=0 ; y<(BOARD_SIZE+2*BOARD_SAFETY) ; y++)
			for( piece=0 ; piece<2 ; piece++)
				*(pwBoard+((x<<BoardXExp)+(y<<BoardYExp)+piece))=0;

	sAlreadyFoundRows[0]=0;
	sAlreadyFoundRows[1]=0;

	rectBoard.left= BOARD_SAFETY;
	rectBoard.right= BOARD_SAFETY+BOARD_SIZE-1;
	rectBoard.top= BOARD_SAFETY;
	rectBoard.bottom= BOARD_SAFETY+BOARD_SIZE-1;

	rectUsed.left= rectBoard.left;
	rectUsed.right= rectBoard.left;
	rectUsed.top= rectBoard.top;
	rectUsed.bottom= rectBoard.top;

	pointBackgroundProcessing.x=0;
	pointBackgroundProcessing.y=0;

	sGameCount=0;
	return(TRUE);
	}



/*
 *
 *  CheckIfTestedBefore
 *
 *
 *  POINT *pptNew			-- point to be checked
 *  POINT ptAlreadyChecked[]		-- list of previous points 
 *  short *psPointsAlreadyChecked	-- number of previous points
 *
 *
 *  returns if pptNew is listed in ptAlreadyChecked[] or not
 *  if not, then it is added to the list and number of points increased
 *
*/

#define MAX_POINT_ALREADY_CHECKED 15

BOOL 
#ifdef MSWINDOWS
__near __fastcall 
#endif
CheckIfTestedBefore(POINT *pptNew, POINT ptAlreadyChecked[],short *psPointsAlreadyChecked)
	{
	short l;
	for(l=0;(l< *psPointsAlreadyChecked) && ((ptAlreadyChecked[l].x != pptNew->x) || (ptAlreadyChecked[l].y != pptNew->y));l++)
		continue;
	if (l<*psPointsAlreadyChecked) return(TRUE);
	ptAlreadyChecked[*psPointsAlreadyChecked  ].x = pptNew->x;
	ptAlreadyChecked[(*psPointsAlreadyChecked)++].y = pptNew->y;
	if ( *psPointsAlreadyChecked > MAX_POINT_ALREADY_CHECKED )
		*psPointsAlreadyChecked = MAX_POINT_ALREADY_CHECKED;
	return(FALSE);
	}


/*
 *
 * FindSimplyBest()
 *
 * short Piece				-- nought or a cross
 * short CountMax			-- max number of returned points
 * VALUEPOINT vpBest[2][UPDATECOUNT]	-- place to put the return points
 *
 * returns a list of best empty squares on a board based on values listed in
 * the database of squares considering only rows of one kind of pieces
 * special: checks the board in a random order. So squares with same value are listed
 * in a random order to achieve little randomness in the computers movemaking.
 * 
 *
*/


BOOL
#ifdef MSWINDOWS
FAR  
#endif
FindSimplyBest(short Piece, short CountMax,VALUEPOINT vpBest[2][UPDATECOUNT]) 
	{
	POINT ptI;
	short TempValue;
	short i,j,sSize,sSize2;
	POINT ptBase;
	short   sCount=0;
	BOOL    bEmptyFound=FALSE;
   
	for(i=0;i<CountMax;i++)
		vpBest[Piece][i].value=0;


				/* from what area we need points */	
	sSize=4;
	for(i=4;i<=64;i=i*2)
		if ( max(rectUsed.right-rectUsed.left,rectUsed.bottom-rectUsed.top) > i ) sSize = 2*i;
		else break;
	sSize2=sSize*sSize;

	ptBase.x=(rectUsed.left);
	ptBase.y=(rectUsed.top);
				/* initialize random number generator */
	RandomOrder(ptBase,sSize,1);


	for (i=0;i<sSize2;i++)
		{
				/* every 32th round check for a windowsmessage and return garbage */
		if ( ((i&0x0020)==0) && ( bProcessing || bBackgroundProcessing ) && PeekForAWindowsMessage())
			return(TRUE);
		
		ptI = RandomOrder(ptBase,sSize,0);

		if ( 
			(ptI.x>=rectUsed.left)
			&& (ptI.x<=rectUsed.right)
			&& (ptI.y>=rectUsed.top)
			&& (ptI.y<=rectUsed.bottom)
			&& bEmptySpec(ptI) )
			{
			bEmptyFound=TRUE;
			TempValue=BoardValue(ptI,Piece);

				/* put ptI in the list on its position, shift old ones if necessary */
			if ( TempValue  > vpBest[Piece][CountMax-1].value )  
				{
				for(j=sCount; ( ( j > 0 ) && ( TempValue > vpBest[Piece][j-1].value ) ) ;j--)
					vpBest[Piece][j]=vpBest[Piece][j-1];
				vpBest[Piece][j].value=TempValue;
				vpBest[Piece][j].point=ptI;
				if ( ( ++sCount ) >= CountMax )
					sCount=CountMax-1;
				}
			}
		}

				/* mark the last one on the list */
	if ( vpBest[Piece][sCount].value==0 )
		vpBest[Piece][sCount].point.x=0;

	return( bEmptyFound );
	}


/*
 *
 * UpdateBest()
 *
 *
 * BOARDCHANGE *Changed			-- All Possibly Changed Squares
 * short Piece				-- Nought or a cross
 * VALUEPOINT vpIN[2][UPDATECOUNT]	-- Input old best squares
 * VALUEPOINT vpOUT[2][UPDATECOUNT]	-- Output the new list of best squares
 *
 *
 * If a new piece is added to the board the list of highest valued squares
 * need updating
 *
*/



void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
UpdateBest(BOARDCHANGE *Changed,short Piece, VALUEPOINT vpIN[2][UPDATECOUNT],VALUEPOINT vpOUT[2][UPDATECOUNT])
	{
	short   i,INindex,CHindex,OUTindex,sCount;
	short   TempValue;
	BOOL    INEnable,ChandedEnable;
	VALUEPOINT vpCH[16];
	POINT ptI;

	for(i=0;i<16;i++)
		vpCH[i].value=0;

				/* sort Changed squares */

	sCount=0;
	for( i = 1; ( i < 17 ) && ( Changed->Coord[i].x != 0 );i++)
		{
		ptI=Changed->Coord[i];
		TempValue=BoardValue(ptI,Piece);
		if ( TempValue == Changed->wBoard[i][Piece] ) continue;

		if ( TempValue  > vpCH[15].value )  
			{
			for(CHindex=sCount; ( ( CHindex > 0 ) && ( TempValue > vpCH[CHindex-1].value ) );CHindex--) 
				vpCH[CHindex]=vpCH[CHindex-1];
			vpCH[CHindex].value=TempValue;
			vpCH[CHindex].point=ptI;
			sCount++;
			}
		}

	if ( vpCH[sCount].value==0 )
		vpCH[sCount].point.x=0;


	INindex=0;
	CHindex=0;
	OUTindex=0;

	INEnable = TRUE;
	ChandedEnable = TRUE;

				/* merge new changed squares to the list */
	do
		{
				/* take away old ones which have been changed to avoid duplicates */
		while ( ( INindex<UPDATECOUNT ) && (vpIN[Piece][INindex].point.x != 0 ) && ( vpIN[Piece][INindex].value != BoardValue(vpIN[Piece][INindex].point,Piece) ) ) 
			INindex++;
		
		if (( INindex >= UPDATECOUNT ) || (vpIN[Piece][INindex].point.x == 0 ) )
			{
			INEnable = FALSE;
			if ( !ChandedEnable ) break;
			}

		if ( ( CHindex > 15 ) || (vpCH[CHindex].point.x == 0 ) )        
			{
			ChandedEnable = FALSE;
			if ( !INEnable ) break;
			}

		if (( ChandedEnable ) && (  ( !INEnable ) || ( vpIN[Piece][INindex].value < vpCH[CHindex].value ) )) 
			{
			vpOUT[Piece][OUTindex].point=vpCH[CHindex].point;
			vpOUT[Piece][OUTindex++].value=vpCH[CHindex++].value;
			}
		else
			{
			vpOUT[Piece][OUTindex].point=vpIN[Piece][INindex].point;
			vpOUT[Piece][OUTindex++].value=vpIN[Piece][INindex++].value;
			}
		}
	while ( OUTindex < UPDATECOUNT );

				/* mark the end of the list, if the whole array was not filled */
	if ( OUTindex < UPDATECOUNT ) 
		{
		vpOUT[Piece][OUTindex].point.x=0;
		vpOUT[Piece][OUTindex].value=0;
		}
	}



/*
 *
 * TakeAwayPiece()
 *
 * BOARDCHANGE *Changed			-- list of changes that must be reversed
 *
 * Whenever a piece is taken away from the board the board must be restored
 * to the same state it had before the piece is put there.
 *
 *
*/


void 
#ifdef MSWINDOWS
FAR  
#endif
TakeAwayPiece(BOARDCHANGE *Changed)
	{
	short i;
	for(i=0;((i<17)&&( Changed->Coord[i].x != 0 ));i++)
		{
		*(pwBoard+((Changed->Coord[i].x<<BoardXExp)+(Changed->Coord[i].y<<BoardYExp))) = Changed->wBoard[i][0];
		*(pwBoard+((Changed->Coord[i].x<<BoardXExp)+(Changed->Coord[i].y<<BoardYExp)+1)) = Changed->wBoard[i][1];
		}
	}
				



/*
 *
 * UpdateValueArray
 *
 * makes an array of values of squares 
 * to be used as a look-up table
 *
 *
 *
*/

short 
#ifdef MSWINDOWS
__near __fastcall 
#endif
NearValueFunction(short sLength, BOOL Open)
	{
	if ( sSkillLevel <= 1 ) 
		{
                if ( sLength == 1 ) return(0x1);
                if ( sLength == 2 ) return(0x10);
                if ( sLength == 3 ) return(0x100);
                if ( sLength >= 4 ) return(0x1000);
		}
	else
		{
                if ( sLength == 1 )
			{
			if ( Open ) return(9);
			else return(1); 
			}
		else if ( sLength == 2 )
			{
			if ( Open ) return(0xA1);
			else return(0x21); 
			}
		else if ( sLength == 3 )
			{
			if ( Open ) return(0x401);
			else return(0xD1); 
			}
		else
			if ( Open )return(0x1101);
			else return(0x1001); 
		}
	return(0);
	}

short 
#ifdef MSWINDOWS
__near __fastcall 
#endif
FarValueFunction(short sLength, BOOL Open)
	{
	if ( sSkillLevel <= 1 ) return(0);
	if ( sLength == 1 )
		{
		if ( Open ) return(8);
		else return(1); 
		}
	else if ( sLength == 2 )
		{
		if ( Open ) return(0xA0);
		else return(0x20); 
		}
	else 
		{
		if ( Open ) return(0xD1);
		else return(0xD0); 
		}
	}

void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
UpdateValueArray(void)
	{
	short RowLength,AreBothEndsOpen;
	for(RowLength=0;RowLength<16;RowLength++)
		for(AreBothEndsOpen=0;AreBothEndsOpen<2;AreBothEndsOpen++)
			{
			NearValue[RowLength][AreBothEndsOpen]=NearValueFunction(RowLength,AreBothEndsOpen);
			FarValue[RowLength][AreBothEndsOpen]=FarValueFunction(RowLength,AreBothEndsOpen);
			}
	}

/*
 *
 * CountValue()
 *
 * short NA,short NB, short NC, short ND	-- lengths of rows around the empty square
 * BOOL OA, BOOL OB, BOOL OC, BOOL OD		-- are gaps between rows open or not
 *
 * short *sChange1				-- place to put the return value ( historical reasons )
 *
 * let me try to little explain this:
 *
 *      NA    NB          NC    ND           ** rows
 *  OA     OB    NewEmpty    OC    OD        ** empty spots between the rows
 *
 * for example consider this:     0 XX^ X0  ( ^ is the empty spot for which we are counting a value for X)
 * NB=2 	because the two X:s are right next to the new empty position
 * OB=true 	because of the empty spot between 0 and XX is open for X
 * NA=0 	because the leftmost 0 is an enemypiece and it is ignored
 * OA=false     because the leftmost 0 is blocking this square from X when looking from ^
 * NC=0 	because there is nothing between the ^empty slot and the empty slot right next to it
 * OC=true 	because the empty spot right next to the ^empty slot
 * ND=1 	because of the X on the right of ^
 * OD=false 	because there is an enemypiece and not an empty spot on the far side of the rightmost X
 *
*/

void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
CountValue(short NA,short NB, short NC, short ND, BOOL OA, BOOL OB, BOOL OC, BOOL OD, short *sChange1)
	{
	short a,b;
	short c;
	BOOL  d;

        if ( !OA && !OD && ((OB+OC+NA+NB+NC+ND ) < 4 )) return;

	if( NB || NC )
		{
		if ( (NB==1)&&(NC==1)&&(NA||ND))
			{
			c=3;
			d=NA&&ND;
			}
		else
			{
			c=NB+NC;
			d=OB&&OC;
			}
		a=NearValue[c][d];
		}
	else a=0;
	if( NA && !NC ) 
		{
		b=FarValue[NA+NB][OA && OC];
		if ( b > a ) 
			a = b;
		}
	else if( NA && NC && !NB && OB )
		{
		b=FarValue[NA+NC][OA && OC];
		if ( b > a ) 
			a = b;
		}
	if( ND && !NB )
		{
		b=FarValue[NC+ND][OB && OD];
		if ( b > a ) 
			a = b;
		}
	else if( NB && ND && !NC &&OC )
		{
		b=FarValue[NB+ND][OB && OD];
		if ( b > a ) 
			a = b;
		}
	*sChange1+=a;
	}


/*
 *
 *
 *
 * FindNeighbours()
 * POINT   *pptMidPos			-- input, the point in the middle
 * BOOL    Open[8][4]			-- place to put the fact are gaps between rows open or not
 * POINT   OpenPos[8][4]		-- positions of the found empty gaps
 * short   N[8][4]			-- length of the rows between gaps
 * short   NotPiece[8]			-- information which kind of piece the rows are not
 * ( you may wonder of the purpose of the notpiece, but it does actually make sense )
 *
 * Gets as input the middle position and this functions job is to return information from the board
 * which kind of rows and gaps are around the middle position in all 8 directions
 * 
 * Travels through the pwBoard "array" using pointer juggling.
 * remember: there is always 0 value on the edges of the board, so you have to
 * check for pointer overflow only if the value on the board is 0
 *
*/

void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
FindNeighbours(   POINT   *pptMidPos, 
					BOOL    Open[8][4],
					POINT   OpenPos[8][4],
					short   N[8][4],
					short   NotPiece[8] )
	{
	short   dir,dir2;
	short   *pTempwBoard;
	short   xMul=0;
	short   yMul=0;
	short   sBoardAdressChange;
	short   OpenPosTempX,OpenPosTempY;
	short   i;

	dir=0;
	while ( dir < 8 )
		{
		switch (dir)
			{
			case 0: xMul=0;
				yMul=1;
				break;
			case 2: xMul=1;
				yMul=0;
				break;
			case 4: xMul=1;
				yMul=1;
				break;
			case 6: xMul=1;
				yMul=-1;
				break;
			}
		for ( dir2 = 0 ; dir2 < 2 ; dir2++ )
			{
			xMul *= -1;
			yMul *= -1;
			pTempwBoard=pwBoard+(((pptMidPos->x+xMul)<<BoardXExp)+((pptMidPos->y+yMul)<<BoardYExp));
			sBoardAdressChange=(xMul<<BoardXExp)+(yMul<<BoardYExp);
			N[dir][0]=0;
			N[dir][1]=0;
			N[dir][2]=0;
			N[dir][3]=0;
			Open[dir][0]=FALSE;
			Open[dir][1]=FALSE;
			Open[dir][2]=FALSE;
			Open[dir][3]=FALSE;
			NotPiece[dir]=-1;
			for(i=0;i<4;i++)
				{
							/* if there is 0 on the *pTempBoard */
				if ( (*(pTempwBoard) & 0xFFFC ) == 0x8FFC )
					{
							/* if we were not counting Zeros, break */
					if ( NotPiece[dir]==0 )break;
							/* Count the whole row of Zeros */
					NotPiece[dir]=1;
					do      {
						N[dir][i]++;
						pTempwBoard +=sBoardAdressChange; 
						}
					while ( (*(pTempwBoard) & 0xFFFC ) == 0x8FFC );
					}
							/* if there is X on the *pTempBoard */
				else if ( (*(pTempwBoard+1) & 0xFFFC ) == 0x8FFC  )
					{
							/* if we were not counting Crosses, break */
					if ( NotPiece[dir]==1 )break;
							/* Count the whole row of Crosses */
					NotPiece[dir]=0;
					do      {
						N[dir][i]++;
						pTempwBoard +=sBoardAdressChange; 
						}
					while ( (*(pTempwBoard+1) & 0xFFFC ) == 0x8FFC);
					}
							/* if there is empty spot on *pTempwBoard */
				if (( (*(pTempwBoard) & 0xFFFC ) != 0x8FFC )&&( (*(pTempwBoard+1) & 0xFFFC ) != 0x8FFC) )
					{
							/* Take it's coordinates from *pTempBoard */
					OpenPosTempX=((pTempwBoard-pwBoard)>>BoardXExp)%TOTAL_BOARD_SIZE;
					OpenPosTempY=((pTempwBoard-pwBoard)>>BoardYExp)%TOTAL_BOARD_SIZE;
							/* If we are over the playing area, do not count further in this direction */
							/* ( we are still on the allocated memory area though */
					if (   ( OpenPosTempX > rectBoard.right )
						|| ( OpenPosTempX < rectBoard.left )
						|| ( OpenPosTempY > rectBoard.bottom )
						|| ( OpenPosTempY < rectBoard.top ) )
						{
						break;
						}
							/* we have found an valid empty spot */
					Open[dir][i]=TRUE;
					if (i<2)
						{
						OpenPos[dir][i].x=OpenPosTempX;
						OpenPos[dir][i].y=OpenPosTempY;
						}
							/* update the board pointer for next square */
					pTempwBoard +=sBoardAdressChange; 
					} 
				}
			dir++;
			}
		}
	}


/*
 *
 * 
 * PutPiece()
 *
 * POINT *pptNewPos			-- position of the new piece
 * short NewPiece		-- what kind of a piece is it
 * BOARDCHANGE *Changed		-- return all changes made to the board
 * 
 * 
 * When a new piece is added to the board
 * a) the new position is marked occupied
 * b) all the surrounding empty squares need their values updated
 * c) all changes must be recorded for possibly later whitdrawal of the piece
 *
 * Most of the complexity of this function is because of we must provide
 * quite complicated parameters to CountValue function, and call it several times
 * in different situations. 
 * ( twice for all 16 possibly changed squares ( before and after the new piece is added ))
 * 
 * 
 */

void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
PutPiece(POINT *pptNewPos, short NewPiece, BOARDCHANGE *Changed)
	{
	short   dir;
	BOOL    Open[8][4];
	POINT   OpenPos[8][4];
	short   N[8][4];
	short   NotPiece[8];
	short   tempPiece;
	short   Change1,Change2;
	BOOL    bAlreadyChanged[4];
	short   sCount=1;
	BOOL    bOpen0_0,bOpen0_1,bOpen0_2,bOpen0_3,bOpen1_0,bOpen1_1,bOpen1_2,bOpen1_3;
	short   sN0_0,sN0_1,sN0_2,sN0_3,sN1_0,sN1_1,sN1_2,sN1_3;

	FindNeighbours( pptNewPos,Open,OpenPos,N,NotPiece );

	for ( dir=0;dir < 8;dir=dir+2 )
		{
		bAlreadyChanged[0] = FALSE;
		bAlreadyChanged[1] = FALSE;
		bAlreadyChanged[2] = FALSE;
		bAlreadyChanged[3] = FALSE;
		for ( tempPiece = 0 ; tempPiece < 2 ; tempPiece++ )
			{

			if (NotPiece[dir]!=tempPiece)
			  {
			  sN0_0=N[dir][0];
			  sN0_1=N[dir][1];
			  sN0_2=N[dir][2];
			  sN0_3=N[dir][3];
			  }
			else
			  {
			  sN0_0=0;
			  sN0_1=0;
			  sN0_2=0;
			  sN0_3=0;
			  }

			if(NotPiece[dir+1]!=tempPiece)
			  {
			  sN1_0=N[dir+1][0];
			  sN1_1=N[dir+1][1];
			  sN1_2=N[dir+1][2];
			  sN1_3=N[dir+1][3];
			  }
			else
			  {
			  sN1_0=0;
			  sN1_1=0;
			  sN1_2=0;
			  sN1_3=0;
			  }

			bOpen0_0=Open[dir][0]&&( ((N[dir][0])==0)                               ||(NotPiece[dir]!=tempPiece));
			bOpen0_1=Open[dir][1]&&( ((N[dir][0]+N[dir][1])==0)                     ||(NotPiece[dir]!=tempPiece)),
			bOpen0_2=Open[dir][2]&&( ((N[dir][0]+N[dir][1]+N[dir][2])==0)           ||(NotPiece[dir]!=tempPiece)),
			bOpen0_3=Open[dir][3]&&(NotPiece[dir]!=tempPiece),
			bOpen1_0=Open[dir+1][0]&&( ((N[dir+1][0])==0)                           ||(NotPiece[dir+1]!=tempPiece));
			bOpen1_1=Open[dir+1][1]&&( ((N[dir+1][0]+N[dir+1][1])==0)               ||(NotPiece[dir+1]!=tempPiece));
			bOpen1_2=Open[dir+1][2]&&( ((N[dir+1][0]+N[dir+1][1]+N[dir+1][2])==0)   ||(NotPiece[dir+1]!=tempPiece));
			bOpen1_3=Open[dir+1][3]&&(NotPiece[dir+1]!=tempPiece);
			if ( bOpen0_0)
				{
				Change1=0;
				CountValue(     sN1_0,
						sN0_0,
						sN0_1,
						sN0_2,
						bOpen1_0,
						TRUE,
						bOpen0_1,
						bOpen0_2,
						&Change1);
				Change1*=-1;
				CountValue(
					sN1_1*(NewPiece==tempPiece),
					(sN1_0+1)*(NewPiece==tempPiece)+sN0_0,
					sN0_1,
					sN0_2,
					bOpen1_1&&(NewPiece==tempPiece),
					bOpen1_0&&(NewPiece==tempPiece),
					bOpen0_1,
					bOpen0_2,
					&Change1);
				if ( Change1 != 0 )
					{
					if ( !bAlreadyChanged[0] )
						{
						Changed->Coord[sCount]=OpenPos[dir][0];
						Changed->wBoard[sCount][0]=*(pwBoard+((OpenPos[dir][0].x<<BoardXExp)+(OpenPos[dir][0].y<<BoardYExp)));
						Changed->wBoard[sCount++][1]=*(pwBoard+((OpenPos[dir][0].x<<BoardXExp)+(OpenPos[dir][0].y<<BoardYExp)+1));
						}
					bAlreadyChanged[0] = TRUE;
					*(pwBoard+((OpenPos[dir][0].x<<BoardXExp)+(OpenPos[dir][0].y<<BoardYExp)+tempPiece)) += Change1;
					}

				if ( bOpen0_1)
					{
					Change2=0;
					CountValue(
						sN0_0,
						sN0_1,
						sN0_2,
						sN0_3,
						TRUE,
						bOpen0_0,
						bOpen0_2,
						bOpen0_3,
						&Change2);
					Change2*=-1;
					CountValue(
						(sN1_0+1)*(NewPiece==tempPiece)+sN0_0,
						sN0_1,
						sN0_2,
						sN0_3,
						bOpen1_0&&(NewPiece==tempPiece),
						bOpen0_0,
						bOpen0_2,
						bOpen0_3,
						&Change2);
					
	
	
					if ( Change2 != 0 )
						{
						if ( !bAlreadyChanged[1] )
							{
							Changed->Coord[sCount] = OpenPos[dir][1];
							Changed->wBoard[sCount][0] = *(pwBoard+((OpenPos[dir][1].x<<BoardXExp)+(OpenPos[dir][1].y<<BoardYExp)));
							Changed->wBoard[sCount++][1] = *(pwBoard+((OpenPos[dir][1].x<<BoardXExp)+(OpenPos[dir][1].y<<BoardYExp)+1));
							}
						bAlreadyChanged[1] = TRUE;
						*(pwBoard+((OpenPos[dir][1].x<<BoardXExp)+(OpenPos[dir][1].y<<BoardYExp)+tempPiece)) += Change2;
						}

					}
				}

			if ( bOpen1_0)
				{
				Change1=0;
				CountValue(     sN0_0,
						sN1_0,
						sN1_1,
						sN1_2,
						bOpen0_0,
						TRUE,
						bOpen1_1,
						bOpen1_2,
						&Change1);
				Change1*=-1;
				CountValue(
					sN0_1*(NewPiece==tempPiece),
						(sN0_0
						+1)*(NewPiece==tempPiece)
						+sN1_0,
					sN1_1,
					sN1_2,
					bOpen0_1&&(NewPiece==tempPiece),
					bOpen0_0&&(NewPiece==tempPiece),
					bOpen1_1,
					bOpen1_2,
					&Change1);
				if ( Change1 != 0 )
					{
					if ( !bAlreadyChanged[2] )
						{
						Changed->Coord[sCount]=OpenPos[dir+1][0];
						Changed->wBoard[sCount][0]=*(pwBoard+((OpenPos[dir+1][0].x<<BoardXExp)+(OpenPos[dir+1][0].y<<BoardYExp)));
						Changed->wBoard[sCount++][1]=*(pwBoard+((OpenPos[dir+1][0].x<<BoardXExp)+(OpenPos[dir+1][0].y<<BoardYExp)+1));
						}
					bAlreadyChanged[2] = TRUE;
					*(pwBoard+((OpenPos[dir+1][0].x<<BoardXExp)+(OpenPos[dir+1][0].y<<BoardYExp)+tempPiece)) += Change1;
					}

				if ( bOpen1_1)
					{
					Change2=0;
					CountValue(
						sN1_0,
						sN1_1,
						sN1_2,
						sN1_3,
						TRUE,
						bOpen1_0,
						bOpen1_2,
						bOpen1_3,
						&Change2);
					Change2*=-1;
					CountValue(
						(sN0_0+1)*(NewPiece==tempPiece)+sN1_0,
						sN1_1,
						sN1_2,
						sN1_3,
						bOpen0_0&&(NewPiece==tempPiece),
						bOpen1_0,
						bOpen1_2,
						bOpen1_3,
						&Change2);
					
	
	
					if ( Change2 != 0 )
						{
						if ( !bAlreadyChanged[3] )
							{
							Changed->Coord[sCount] = OpenPos[dir+1][1];
							Changed->wBoard[sCount][0] = *(pwBoard+((OpenPos[dir+1][1].x<<BoardXExp)+(OpenPos[dir+1][1].y<<BoardYExp)));
							Changed->wBoard[sCount++][1] = *(pwBoard+((OpenPos[dir+1][1].x<<BoardXExp)+(OpenPos[dir+1][1].y<<BoardYExp)+1));
							}
						bAlreadyChanged[3] = TRUE;
						*(pwBoard+((OpenPos[dir+1][1].x<<BoardXExp)+(OpenPos[dir+1][1].y<<BoardYExp)+tempPiece)) += Change2;
						}

					}
				}
			}
		}



	Changed->Coord[0]=*pptNewPos;
	Changed->wBoard[0][0]=*(pwBoard+((pptNewPos->x<<BoardXExp)+(pptNewPos->y<<BoardYExp)));
	Changed->wBoard[0][1]=*(pwBoard+((pptNewPos->x<<BoardXExp)+(pptNewPos->y<<BoardYExp)+1));

	if ( sCount < 17 )
		Changed->Coord[sCount++].x = 0;

		/* Mark the Square Occupied */
		/* 0x8FFC means that the piece on the board is just speculation and not real */
	*(pwBoard+((pptNewPos->x<<BoardXExp)+(pptNewPos->y<<BoardYExp)+NewPiece)) = 0x8FFC;
	*(pwBoard+((pptNewPos->x<<BoardXExp)+(pptNewPos->y<<BoardYExp)+1-NewPiece)) = 0;


	}

/*
 *
 * TranslateBoardSkillLevel
 *
 * if the algorithms skill level is changed maybe the rules of how the values of
 * empty squares are scored are changed, so all empty squares need their values
 * updated.
 *
*/




void 
#ifdef MSWINDOWS
FAR  
#endif
TranslateBoardSkillLevel(void)
	{
	short   dir;
	BOOL    Open[8][4];
	POINT   OpenPos[8][4];
	short   N[8][4];
	short   NotPiece[8];
	short   tempPiece;
	short   Change1;
	short   NewValue[2];
	POINT   ptI;

	UpdateValueArray();
	for( ptI.x=rectUsed.left;ptI.x<=rectUsed.right;ptI.x++)
	for( ptI.y=rectUsed.top;ptI.y<=rectUsed.bottom;ptI.y++)
		{
		if ( !bEmptySpec(ptI) ) continue;
	
		NewValue[0]=0;
		NewValue[1]=0;
	
		FindNeighbours( &ptI,Open,OpenPos,N,NotPiece );
	
	
		for(dir=0;dir<8;dir=dir+2)
			{
			for ( tempPiece = 0 ; tempPiece < 2 ; tempPiece++ )
				{
					{
					Change1=0;
					CountValue(
								N[dir+1][1]*(NotPiece[dir+1]!=tempPiece),
								N[dir+1][0]*(NotPiece[dir+1]!=tempPiece),
								N[dir][0]*(NotPiece[dir]!=tempPiece),
								N[dir][1]*(NotPiece[dir]!=tempPiece),
								Open[dir+1][1]&&(NotPiece[dir+1]!=tempPiece),
								Open[dir+1][0]&&((N[dir+1][0]==0)||(NotPiece[dir+1]!=tempPiece)),
								Open[dir][0]&&((N[dir][0]==0)||(NotPiece[dir]!=tempPiece)),
								Open[dir][1]&&(NotPiece[dir]!=tempPiece),
								&Change1);
					NewValue[tempPiece]+=Change1;
					}
				}
			}

		*(pwBoard+((ptI.x<<BoardXExp)+(ptI.y<<BoardYExp)+0)) = NewValue[0];
		*(pwBoard+((ptI.x<<BoardXExp)+(ptI.y<<BoardYExp)+1)) = NewValue[1];
		}
	}


/*
 *
 * RecordLastPieces()
 *
 * POINT *pptLast			-- position
 * short Piece			-- nought or a cross
 *
 * this function is called from outside to indicate a new piece on the board
 *
 *
 *
*/


void 
#ifdef MSWINDOWS
FAR  
#endif
RecordLastPieces(POINT *pptLast,short Piece,BOARDCHANGE *bcLastPutPiece)
	{

	UpdateValueArray();
	ptLastPutPiece=*pptLast;
	
	PutPiece(pptLast,Piece,bcLastPutPiece);

		/* putpiece does mark the position occupied, but because this time */
		/* the new piece is real and just not speculation. we must change */
		/* the way, how the square is marked */
	*(pwBoard+((pptLast->x<<BoardXExp)+(pptLast->y<<BoardYExp)+Piece)) = 0x8FFF;
	*(pwBoard+((pptLast->x<<BoardXExp)+(pptLast->y<<BoardYExp)+1-Piece)) = 0;

	if ( sGameCount == 0 )
		{
		rectUsed.left = max(pptLast->x-3,rectBoard.left);
		rectUsed.right = min(pptLast->x+3,rectBoard.right);
		rectUsed.top = max(pptLast->y-3,rectBoard.top);
		rectUsed.bottom = min(pptLast->y+3,rectBoard.bottom);
		}       
	else
		{
		if ( rectUsed.left > ( pptLast->x-3) ) rectUsed.left = max(pptLast->x-3,rectBoard.left);
		if ( rectUsed.right < ( pptLast->x+3) ) rectUsed.right = min(pptLast->x+3,rectBoard.right);
		if ( rectUsed.top > ( pptLast->y-3) ) rectUsed.top = max(pptLast->y-3,rectBoard.top);
		if ( rectUsed.bottom < ( pptLast->y+3) ) rectUsed.bottom = min(pptLast->y+3,rectBoard.bottom);

		}
	}



/*
 *
 * GameOver()
 * POINT *pptNewPos 
 * short Piece
 * 
 * a) returns the information is the game ended or not if someone puts a Piece on NewPos
 * b) updates the scores
 * c) if a new row is found this function flashes the row
 * 
*/

BOOL  
#ifdef MSWINDOWS
FAR  
#endif
GameOver(POINT *pptNewPos, short Piece)
	{
	short   dir,i,j,sDifferenceToOldRow;
	short   xMul=0;
	short   yMul=0;
	BOOL    bGameEndFound=FALSE;
	BOOL    Open[8][4];
	POINT   OpenPos[8][4];
	short   N[8][4];
	short   NotPiece[8];
	POINT   ptFoundRow[32];
	short   sNewRowLength;
	POINT	ptTemp;
	
	FindNeighbours( pptNewPos,Open,OpenPos,N,NotPiece );

	for(dir=0;dir<8;dir=dir+2)
		{
		switch (dir)
			{
			case 0: xMul=0;
				yMul=1;
				break;
			case 2: xMul=1;
				yMul=0;
				break;
			case 4: xMul=1;
				yMul=1;
				break;
			case 6: xMul=1;
				yMul=-1;
				break;
			}
					/* delete N if it is of wrong Piece */
		N[dir  ][0] *= (NotPiece[dir  ]!=Piece);
		N[dir+1][0] *= (NotPiece[dir+1]!=Piece);
		if ( (sNewRowLength=(N[dir][0]+N[dir+1][0]+1)) >= sCreditRowLength )
			{
					/* find out all squares in the newly found row */
			for(i=0;i<sNewRowLength;i++)
				{
				ptFoundRow[i].x=pptNewPos->x+(i-N[dir][0])*xMul;
				ptFoundRow[i].y=pptNewPos->y+(i-N[dir][0])*yMul;
				}
					/* is newly found row an old one so forget it */
			for(i=sAlreadyFoundRows[Piece]-1;i>=0;i--)
				{
				sDifferenceToOldRow=0;
				for(j=0;j<2;j++)
					{
					ptTemp=ptFoundRow[j*(sNewRowLength-1)];
					while ( ( sDifferenceToOldRow < sNewRowLength ) 
					  && ( ( ptTemp.x != ptAlreadyFoundRowEnds[i][Piece][j].x ) 
				   	    || ( ptTemp.y != ptAlreadyFoundRowEnds[i][Piece][j].y ) ) ) 
						{
						sDifferenceToOldRow++;
						ptTemp.x+=xMul;
						ptTemp.y+=yMul;
						}
					xMul*=-1;
					yMul*=-1;
					}
				if ( sDifferenceToOldRow == 0 )
					{
					sNewRowLength=0;
					break;
					}
				if ( sDifferenceToOldRow < sNewRowLength ) 
					{
					break;
					}
				}
			if ( i < 0 ) sDifferenceToOldRow=sNewRowLength-sCreditRowLength+1;
			if ( sNewRowLength )
				{
				ptAlreadyFoundRowEnds[sAlreadyFoundRows[Piece]][Piece][0].x =ptFoundRow[0].x  ;
				ptAlreadyFoundRowEnds[sAlreadyFoundRows[Piece]][Piece][0].y =ptFoundRow[0].y;
				ptAlreadyFoundRowEnds[sAlreadyFoundRows[Piece]][Piece][1].x =ptFoundRow[sNewRowLength-1].x;
				ptAlreadyFoundRowEnds[sAlreadyFoundRows[Piece]][Piece][1].y =ptFoundRow[sNewRowLength-1].y;
				sAlreadyFoundRows[Piece]+=sDifferenceToOldRow;
				if ( sAlreadyFoundRows[Piece] >= sGameEndRows )
					bGameEndFound=TRUE;
				for(i=0;i<sNewRowLength;i++)
					{
					*(pwBoard+((ptFoundRow[i].x<<BoardXExp)+(ptFoundRow[i].y<<BoardYExp)+Piece) ) = 0x8FFE;
#ifdef MSWINDOWS
					FlashBoard(ptFoundRow[i],8,1);
#endif
					}
				}
			}
		}

	return(bGameEndFound);
	}




/*
 *
 * IsThereValuablePointsForBoth()
 *
 * short Piece
 * short PieceMinValue
 * short PieceMaxValue
 * short OppositeMinValue
 * short OppositeMaxValue
 * POINT *ptCenter
 * short sRadius
 * 
 * returns the information are there points whis satisfy the rules
 * a) it must be near ptCenter, max distance sRadius
 * b) it must have a value between Min and Max values of both kinds simultaniusly
 * 
 * 
*/


BOOL 
#ifdef MSWINDOWS
__near __fastcall 
#endif
IsThereValuablePointsForBoth( 
			     short Piece ,
			     short PieceMinValue ,
			     short PieceMaxValue ,
			     short OppositeMinValue ,
			     short OppositeMaxValue ,
			     POINT *ptCenter ,
			     short sRadius )
{
	POINT ptI;
	RECT rectBounds;

	rectBounds.left=max(ptCenter->x-sRadius,rectBoard.left);
	rectBounds.right=min(ptCenter->x+sRadius,rectBoard.right);
	rectBounds.top=max(ptCenter->y-sRadius,rectBoard.top);
	rectBounds.bottom=min(ptCenter->y+sRadius,rectBoard.bottom);

	for ( ptI.x=rectBounds.left ; ptI.x<=rectBounds.right ; ptI.x++ )
		for ( ptI.y=rectBounds.top ; ptI.y<=rectBounds.bottom ; ptI.y++ )
			{
			if (( ptI.x == ptCenter->x ) && ( ptI.y == ptCenter->y )) continue;
			if ( bEmptySpec(ptI) 
			&& ( BoardValue(ptI,Piece) >= PieceMinValue )
			&& ( BoardValue(ptI,Piece) <= PieceMaxValue )
			&& ( BoardValue(ptI,1-Piece) >= OppositeMinValue )
			&& ( BoardValue(ptI,1-Piece) <= OppositeMaxValue ) )
				return(TRUE);
			}
	return(FALSE);
}


/*
 * 
 * IsThereTrivialMove()
 * 
 * short Piece
 * short sLevelCount			-- how deep are we in the swamp of recursion
 * VALUEPOINT vpPoints[2][UPDATECOUNT]	-- input list of highest valued empty squares
 * VALUEPOINT *pvpResult			-- returns information if there is a trivial move to be made or not
 *
 * 
 *
 *
 *
*/


void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
IsThereTrivialMove(short Piece, short sLevelCount,VALUEPOINT vpPoints[2][UPDATECOUNT],VALUEPOINT *pvpResult)
	{
	short BestPieceValue;
	short BestOppositePieceValue;

	BestPieceValue=vpPoints[Piece][0].value;
	BestOppositePieceValue=vpPoints[1-Piece][0].value;
        pvpResult->value=0;

	if ( BestPieceValue == 0 )
		pvpResult->value=LoserValue;
	else if ( BestOppositePieceValue == 0 ) 
		pvpResult->value=WinValue;
	else  if ( ( BestPieceValue >= 0x1000 ) && ( BestOppositePieceValue >= 0x1000 ) )
		{
		if ( ( sAlreadyFoundRows[Piece] >= sAlreadyFoundRows[1-Piece] ) 
		  || ( sLevelCount >= 1 ) )
			pvpResult->value=WinValue;
		else pvpResult->value=LoserValue;
		}
	else if ( BestPieceValue >= 0x1000 )
		pvpResult->value=WinValue;
	else if ( BestOppositePieceValue >= 0x1000 )
		pvpResult->value=LoserValue;

	else if ( ( BestPieceValue >= 0x170 ) /* 0xD0 + 0xA0 */
               &&(!IsThereValuablePointsForBoth(Piece,0xA0,0x7FFF,0xD0,0x7FFF,
                             &vpPoints[Piece][0].point, 1) ) 
               &&(IsThereValuablePointsForBoth(Piece,0xD0,0x13F+(sLevelCount>=6)*0x1000,0x00,0x7FFF,
                             &vpPoints[Piece][0].point, 1) ) )
	        pvpResult->value=WinValue;
	else if (( BestPieceValue >= 0x140 ) /* 0xA0 + 0xA0 */
              && ( BestOppositePieceValue < /* 0x170 */ 0xE8) /* 0xD0 + 0xA0 */
              &&(!IsThereValuablePointsForBoth(Piece,0xA0,0x7FFF,0xD0,0x7FFF,
			     &vpPoints[Piece][0].point, 1) ) )
		pvpResult->value=WinValue;


	if ( pvpResult->value <= 0 )
		pvpResult->value=0;
	else
		pvpResult->point=vpPoints[Piece][0].point;
	}


/*
 *
 *
 * FindMasterMove()
 *
 * short Piece					-- what kind of piece are we putting on the board
 * short sLevelCount				-- how deep are we in the recursion
 * VALUEPOINT vpSelectedPoints[2][UPDATECOUNT]	-- list of highest valued empty squares
 * 
 * 
 * the heart of the algorithm
 * recursively traverses the tree of possible new pieces
 * and returns the best move found
 *
 * 
*/


void 
#ifdef MSWINDOWS
__near __fastcall 
#endif
FindMasterMove(short Piece,short sLevelCount,VALUEPOINT vpSelectedPoints[2][UPDATECOUNT],VALUEPOINT *pvpResult)
	{
	short           sPointsAlreadyChecked=0;
	short           sDefencePointsAlreadyChecked=0;
	POINT           ptAlreadyChecked[16];
	VALUEPOINT         vpSelectedPointsForNextLevel[2][UPDATECOUNT];
	BOARDCHANGE     BoardChanged;
	short           i;
	POINT           ptI;
	VALUEPOINT         vpBestResultSoFar;
	VALUEPOINT         vpTemp;


	if ( ( sLevelCount & 0x0001 ) == 1 ) lCounter ++;





	IsThereTrivialMove(Piece,sLevelCount,vpSelectedPoints,&vpTemp);
	if ( vpTemp.value != 0 )
		{
		pvpResult->value=vpTemp.value;
		pvpResult->point.x=vpTemp.point.x;
		pvpResult->point.y=vpTemp.point.y;
		return;
		}


	if ( sLevelCount > (sDepth-1) )
		{
		if ( ( vpSelectedPoints[Piece][0].value > 0x100)
		   && ( ( vpSelectedPoints[1-Piece][0].value < 0x170)
                   || ( vpSelectedPoints[1-Piece][1].value < 0xF0 ) ) )
			{
			vpTemp.value = WinValue;


			pvpResult->value=vpTemp.value;
			pvpResult->point.x=vpTemp.point.x;
			pvpResult->point.y=vpTemp.point.y;
			return;
			}
		if ( ( vpSelectedPoints[Piece][0].value >= 0xD1)
		   && ( vpSelectedPoints[1-Piece][0].value < 0x100) )
			{
			vpTemp.value = 300+min(500,max(0,vpSelectedPoints[Piece][0].value-vpSelectedPoints[1-Piece][0].value));


			pvpResult->value=vpTemp.value;
			pvpResult->point.x=vpTemp.point.x;
			pvpResult->point.y=vpTemp.point.y;
			return;
			}
		if ( sLevelCount > sDepth )
			{
			pvpResult->value=0;
			pvpResult->point.x=vpTemp.point.x;
			pvpResult->point.y=vpTemp.point.y;
			return;
			}
		}

	if ( vpSelectedPoints[1-Piece][0].value > 0x1000 )
		{
		vpBestResultSoFar.point=vpSelectedPoints[1-Piece][0].point;
		vpBestResultSoFar.value=-2000*(vpSelectedPoints[1-Piece][1].value > 0x1000);
		if ( ( sLevelCount == 0 ) || (vpSelectedPoints[1-Piece][1].value > 0x1000) ) 
			{
			pvpResult->value=vpBestResultSoFar.value;
			pvpResult->point.x=vpBestResultSoFar.point.x;
			pvpResult->point.y=vpBestResultSoFar.point.y;
			return;
			}
		PutPiece(&(vpBestResultSoFar.point),Piece,&BoardChanged);
		UpdateBest(&BoardChanged,Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);
		UpdateBest(&BoardChanged,1-Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);
		FindMasterMove(1-Piece,sLevelCount+1,vpSelectedPointsForNextLevel,&vpTemp);
		TurnAroundResult(vpTemp);
		TakeAwayPiece(&BoardChanged);
		vpBestResultSoFar.value=vpTemp.value;


		pvpResult->value=vpBestResultSoFar.value;
		pvpResult->point.x=vpBestResultSoFar.point.x;
		pvpResult->point.y=vpBestResultSoFar.point.y;
		return;
		}

	if ( PeekForAWindowsMessage() ) return;

	vpBestResultSoFar.point=vpSelectedPoints[1-Piece][0].point;
	vpBestResultSoFar.value=LoserValue;

	for(i=0;i<UPDATECOUNT;i++)
		{
		if ( sPointsAlreadyChecked >= sAttackEvaluateMax[sLevelCount] ) break;
		ptI=vpSelectedPoints[Piece][i].point;
		if ( ptI.x == 0 )       break;
		if ( ( sPointsAlreadyChecked >= 1 ) && ( BoardValue(ptI,Piece) < sAttackValueMin[sLevelCount] )) break;
		if ( ( vpBestResultSoFar.value < -1000 ) && (sPointsAlreadyChecked >= sLosersEvaluateMax[sLevelCount]) )
			break;
		if ((sPointsAlreadyChecked >= sAttackEvaluateMin[sLevelCount]) 
		   && ( vpBestResultSoFar.value > 1000 ) ) 
				break; 

		if (  ( sPointsAlreadyChecked >= 1 ) 
/*                        &&  ( sLevelCount >= 1 )    */
			&&  ( BoardValue(ptI,Piece) <= 0xD1 )
                        && ( BoardValue(ptI,Piece) >= 0xD0 )  )continue;

		if ( CheckIfTestedBefore(&ptI, &ptAlreadyChecked[0],&sPointsAlreadyChecked) ) 
			continue;

		PutPiece(&ptI,Piece,&BoardChanged);
		UpdateBest(&BoardChanged,Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);
		UpdateBest(&BoardChanged,1-Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);

		FindMasterMove(1-Piece,sLevelCount+1,vpSelectedPointsForNextLevel,&vpTemp);
		TakeAwayPiece(&BoardChanged);
		
		TurnAroundResult(vpTemp);
	
		if ( vpTemp.value > vpBestResultSoFar.value )
			{
			vpBestResultSoFar.value=vpTemp.value;
			vpBestResultSoFar.point=ptI;
			}


		if ( bInterruptBackgroundProcessing )
			return;
		}

	if ( vpBestResultSoFar.value > -1000)
		{
		pvpResult->value=vpBestResultSoFar.value;
		pvpResult->point.x=vpBestResultSoFar.point.x;
		pvpResult->point.y=vpBestResultSoFar.point.y;
		return;
		}

	for(i=0;i<UPDATECOUNT;i++)
		{
		if ( sDefencePointsAlreadyChecked >= sDefenceEvaluateMax[sLevelCount] ) break;
		ptI=vpSelectedPoints[1-Piece][i].point;
		if ( ptI.x == 0 )
			break;
		if ( BoardValue(ptI,1-Piece) < sDefenceValueMin[sLevelCount] )
			break;

		if ( CheckIfTestedBefore(&ptI, &ptAlreadyChecked[0],&sPointsAlreadyChecked) ) 
			continue;

		sDefencePointsAlreadyChecked++;

		PutPiece(&ptI,Piece,&BoardChanged);
		UpdateBest(&BoardChanged,Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);
		UpdateBest(&BoardChanged,1-Piece,vpSelectedPoints,vpSelectedPointsForNextLevel);

		FindMasterMove(1-Piece,sLevelCount+1,vpSelectedPointsForNextLevel,&vpTemp);
		TakeAwayPiece(&BoardChanged);

		TurnAroundResult(vpTemp);

		if ( vpTemp.value > vpBestResultSoFar.value )
			{
			vpBestResultSoFar.value=vpTemp.value;
			vpBestResultSoFar.point=ptI;
			}

   if ( sAlgTest && ( vpTemp.value == vpBestResultSoFar.value )
   && ( BoardValue(ptI,Piece) > ( BoardValue(vpBestResultSoFar.point,Piece)+0x10) )
   && ( BoardValue(ptI,1-Piece) > (vpSelectedPoints[1-Piece][0].value-0x04) ) 
      )
			{
			vpBestResultSoFar.value=vpTemp.value;
			vpBestResultSoFar.point=ptI;
			}


		if ( vpBestResultSoFar.value > -1000 )
			break;

		if ( bInterruptBackgroundProcessing )
			return;
		}


	pvpResult->value=vpBestResultSoFar.value;
	pvpResult->point.x=vpBestResultSoFar.point.x;
	pvpResult->point.y=vpBestResultSoFar.point.y;
	return;

	}

/*
 *
 * FindMyNextPoint()
 *
 * short Piece
 *
 * the function somebody calls from outside to find what is the best move around
 *
 *
*/




void 
#ifdef MSWINDOWS
FAR  
#endif
FindMyNextPoint(short Piece,VALUEPOINT *pvpResult)
	{
	short i,dy,dx;
	VALUEPOINT vpTemp[2][UPDATECOUNT];

	UpdateValueArray();
	if ( sGameCount == 1 )
		{
		i=GetRandom(0,19);
		if ( i < 4 ) 
			{ 
			dx=1; 
			dy=0; 
			}
		else if ( i < 8 ) 
			{ 
			dx=-1; 
			dy=0; 
			}
		else if ( i < 12 ) 
			{ 
			dx=0; 
			dy=1; 
			}
		else if ( i < 16 ) 
			{ 
			dx=0; 
			dy=-1; 
			}
		else if ( i < 17 ) 
			{ 
			dx=1; 
			dy=1; 
			}
		else if ( i < 18 ) 
			{ 
			dx=1; 
			dy=-1; 
			}
		else if ( i < 19 ) 
			{ 
			dx=-1; 
			dy=1; 
			}
		else 
			{ 
			dx=-1; 
			dy=-1; 
			}
		if (ptLastPutPiece.x <= (rectBoard.left+3) ) dx=1;
		if (ptLastPutPiece.x >= (rectBoard.right-3) ) dx=-1;
		if (ptLastPutPiece.y <= (rectBoard.top+3) ) dy=1;
		if (ptLastPutPiece.y >= (rectBoard.bottom-3) ) dy=-1;
		pvpResult->point.x=ptLastPutPiece.x+dx;
		pvpResult->point.y=ptLastPutPiece.y+dy;
		pvpResult->value=0;
		return;
		}

	lCounter=0;
	switch ( sSkillLevel )
		{
		case 0: /* BEGINNER */
		case 1: /* not used */
		case 2: /* NOVICE */  
			sDepth=1;
			break;
		case 3: /* EXPERT */
			sDepth=3;
			break;
		case 4: /* not used */
			sDepth=5;
			break;
		case 5: /* MASTER */
    		 	sDepth=9;
			break;
		}

	for(i=0;i<32;i++)
		{
	 if ( i < 3 )
	   {
           sAttackValueMin[i]=0xA2;
	   sDefenceValueMin[i]=0xA2;
           }
	 else if( i < 7 )
	   {
           sAttackValueMin[i]=0xA9;
	   sDefenceValueMin[i]=0xA9;
	   }
	 else 
	   {
           sAttackValueMin[i]=0xD0;
	   sDefenceValueMin[i]=0xD0;
	   }

		sAttackEvaluateMin[i]=sMasterAttackEvaluateMin[min(i,7)];
		sAttackEvaluateMax[i]=sMasterAttackEvaluateMax[min(i,7)];
		sLosersEvaluateMax[i]=sMasterLosersEvaluateMax[min(i,7)];
		sDefenceEvaluateMax[i]=sMasterDefenceEvaluateMax[min(i,7)];
		}
	sUpdateValueMin=0xA0;

	FindSimplyBest(Piece,UPDATECOUNT,vpTemp);
	if ( bInterruptBackgroundProcessing ) return;
	FindSimplyBest(1-Piece,UPDATECOUNT,vpTemp);
	if ( bInterruptBackgroundProcessing ) return;
        FindMasterMove(Piece,0,vpTemp,pvpResult);

	return;		  
       }


#ifdef MSWINDOWS

/*
 *
 * FindBackgroundResponse()
 *
 * POINT *pptEnemy 		-- the move the enemy made just before this function was called
 * short Piece			-- what kind of piece I have
 *
 * Check if a response was found to this enemys move during backgroundprocessing
 *
 *
*/

void 
#ifdef MSWINDOWS
FAR  
#endif
FindBackgroundResponse(POINT *pptEnemy , short Piece,VALUEPOINT *pvpResult)
	{
	VALUEPOINT vpDummy;
	short i;
	

	for(i=0;i<32;i++)
		{
		if (    ( BackgroundResult[i][1-Piece].point.x == pptEnemy->x ) 
			&& ( BackgroundResult[i][1-Piece].point.y == pptEnemy->y ) )
			{
			*pvpResult=BackgroundResult[i][Piece];
			return;
			}
		}

	vpDummy.point.x=0;
	vpDummy.point.y=0;
	*pvpResult=vpDummy;
	return;
	}

/*
 *
 * ProcessBackground()
 *
 * short Piece			-- my piece
 *
 * during the enemy is thinking of her move we this responces to her
 *
 *
*/



void 
#ifdef MSWINDOWS
FAR  
#endif
ProcessBackground(short Piece)
	{
	POINT ptEnemy;
	VALUEPOINT vpResponse;
	VALUEPOINT vpEnemyBest[2][UPDATECOUNT];
	short sEvalOrder[32];
	BOARDCHANGE EnemyChanged;
	short i,sTempPiece;
	short j=0;
	short k=0;


	UpdateValueArray();
	FindSimplyBest(Piece,UPDATECOUNT,vpEnemyBest);
	if ( bInterruptBackgroundProcessing ) return;
	FindSimplyBest(1-Piece,UPDATECOUNT,vpEnemyBest);
	if ( bInterruptBackgroundProcessing ) return;

	if  ( vpEnemyBest[1-Piece][0].value < 0x1000) 
	for(i=0;i<32;i++)
		{
		if ( j>= UPDATECOUNT ) j= -1;
		if ( k>= UPDATECOUNT ) k= -1;
		if  ( vpEnemyBest[0][j].point.x == 0) j= -1;
		if  ( vpEnemyBest[0][j].value <= 0x20) j = -1;
		if  ( vpEnemyBest[1][k].point.x == 0) k= -1;
		if  ( vpEnemyBest[1][k].value <= 0x20) k = -1;
		if  ( ( j >= 0 ) && ( ( k < 0 ) || (vpEnemyBest[0][j].value >= vpEnemyBest[1][k].value)))
			pointBackgroundProcessing=vpEnemyBest[0][j++].point;
		if  ( ( k >= 0 ) && ( ( j < 0 ) || (vpEnemyBest[1][k].value >= vpEnemyBest[0][j].value)))
			pointBackgroundProcessing=vpEnemyBest[1][k++].point;


		PutPiece(&pointBackgroundProcessing,1-Piece,&EnemyChanged);
		FindMyNextPoint(Piece,&vpResponse);
		TakeAwayPiece(&EnemyChanged);
		if ( bInterruptBackgroundProcessing )
			{ 
			pointBackgroundProcessing.x=0;
			pointBackgroundProcessing.y=0;
			return;
			}
		if ( bSendMessageBackgroundResponse )
			{
			PostMessage(hwndMain,WM_COMMAND,IDM_USERSPIECE,(((DWORD)pointBackgroundProcessing.y)<<16)+(DWORD)pointBackgroundProcessing.x);
			PostMessage(hwndMain,WM_COMMAND,IDM_COMPUTERSPIECE,(((DWORD)vpResponse.point.y)<<16)+(DWORD)vpResponse.point.x);
			pointBackgroundProcessing.x=0;
			pointBackgroundProcessing.y=0;
			return;
			}
		bSendMessageBackgroundResponse = FALSE;
		BackgroundResult[i][1-Piece].point=pointBackgroundProcessing;
		BackgroundResult[i][Piece]=vpResponse;
		pointBackgroundProcessing.x=0;
		pointBackgroundProcessing.y=0;
		}
	}


void
#ifdef MSWINDOWS
FAR  
#endif
EraseBackgroundResults(void)
	{
	short i;
	for(i=0;i<32;i++)
		{
		BackgroundResult[i][0].point.x=0;
		BackgroundResult[i][0].point.y=0;
		BackgroundResult[i][0].value=0;
		BackgroundResult[i][1].point.x=0;
		BackgroundResult[i][1].point.y=0;
		BackgroundResult[i][1].value=0;
		}
	}


#endif



