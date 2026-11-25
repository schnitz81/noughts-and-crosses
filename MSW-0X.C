
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
#include "vardef.h"
#include "macro.h"



LONG FAR  PASCAL WndProc(HWND,UINT,WPARAM,LPARAM);
int FAR PASCAL	AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
int FAR PASCAL	StatisticsDlgProc(HWND,UINT,WPARAM,LPARAM);


void FAR  TranslateBoardSkillLevel(void);
void FAR  UpdateGridSize(void);
void FAR  UpdateWindowSize(HWND,RECT);
void FAR  Paint(PAINTSTRUCT *);
BOOL _near  Initialize(void);
BOOL FAR InitAlgorithmNewGame(void);
void FAR  CleanUp(void);
BOOL FAR  GameOver(POINT *,short);
BOOL FAR   MakeBoardBitmap(HDC);
void FAR  FlashBoard(POINT,short,short);
void FAR  RecordLastPieces(POINT *,short,BOARDCHANGE *);
void FAR  FindMyNextPoint(short,VALUEPOINT *);
BOOL FAR  FindSimplyBest(short,short,VALUEPOINT[2][UPDATECOUNT]); 

void FAR  EraseBackgroundResults(void);
void FAR  FindBackgroundResponse(POINT *, short,VALUEPOINT *);
void FAR  ProcessBackground(short);
void FAR TakeAwayPiece(BOARDCHANGE *);
BOOL  AllocateMemory(void);
void DeAllocateMemory(void);


void FAR  WriteInitFile(void);
void FAR  ReadInitFile(HWND);
void _near GameOverInit(HWND);
void _near UpdateCaptionTexts(void);



BOOL bUserMoveEnabled=FALSE;
BOOL bInitfileChange=FALSE;
BOOL bCurrentGameStartedByComputer;
BOOL bMinimized;
BOOL bUsersTurn;

short sOldClientWidth;
short sOldClientHeight;

BOOL bCursorInClientArea;
HCURSOR hClientCursor;

BOARDCHANGE bcComputerLastChanged;
short	sBeforeComputersPieceAlreadyFoundRows[2];
BOARDCHANGE bcUserLastChanged;
short	sBeforeUsersPieceAlreadyFoundRows[2];



int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow )
	{
	MSG		msg ;
	WNDCLASS	wndclass ;
	HMENU		hMenu;
	

	wndclass.style		= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.cbClsExtra	= 0;
	wndclass.cbWndExtra	= 0;
	wndclass.hInstance	= hInstance;
	wndclass.hIcon		= LoadIcon ( hInstance, szAppShortName );
	wndclass.hCursor	= NULL;
	wndclass.hbrBackground	= GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName	= szAppLongName;
	wndclass.lpszClassName	= szAppLongName; 

	bCursorInClientArea=FALSE;
	hClientCursor=LoadCursor (NULL, IDC_ARROW );

	RegisterClass (&wndclass);
		
	hMenu = LoadMenu(hInstance,szAppShortName);
	hInst=hInstance;


	hwndMain = CreateWindow (szAppLongName,szAppLongName,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
		CW_USEDEFAULT,CW_USEDEFAULT,
		CW_USEDEFAULT,CW_USEDEFAULT,
		NULL, hMenu, hInstance, NULL );



	ShowWindow (hwndMain, nCmdShow);
	UpdateWindow (hwndMain);   
	hAccelMain = LoadAccelerators ( hInstance, szAppShortName );


	while ( GetMessage (&msg, NULL, 0 , 0 ))
		{
		if (!TranslateAccelerator (hwndMain, hAccelMain, &msg ))
			{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
			}
		}

	return msg.wParam;
	}



long FAR PASCAL WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	FARPROC		lpfnDlgProc;
	HDC 		hdc;
	PAINTSTRUCT	ps ;
	short		i;
	short		*pts;
	HMENU		hMenu;
	POINT		point;
	VALUEPOINT		vpCompuMove;
	short		sOldGridSize;
	VALUEPOINT		vpDummy[2][UPDATECOUNT];


	switch (message)
		{
		case WM_TIMER :
			if ( wParam == TIMER_FLASHING )
				{
				FlashBoard(point,0,0);
				}
			else
				{
				bUserMoveEnabled=TRUE;
				KillTimer (hwnd,TIMER_USER_MOVE_ENABLE);
				}
			return(0);
  
		case WM_HSCROLL:
		case WM_VSCROLL:
			if ( message == WM_HSCROLL ) pts = &sXClientOrigin;
			else pts = &sYClientOrigin;
			switch(wParam)
				{
				case SB_PAGEDOWN : *pts += (rectView.bottom-rectView.top)/3; break;
				case SB_LINEDOWN : *pts += (rectView.bottom-rectView.top)/8; break;
				case SB_PAGEUP : *pts -= (rectView.bottom-rectView.top)/3; break;
				case SB_LINEUP : *pts -= (rectView.bottom-rectView.top)/8; break;
				case SB_TOP: *pts = 0; break;
				case SB_BOTTOM: *pts = 32767; break;
				case SB_THUMBPOSITION: *pts = LOWORD(lParam); break;
				default:return 0;
				}
			SendMessage(hwnd,WM_SIZE,0,0);
			return 0;



		case WM_LBUTTONUP:
			if (  bGameGoing && !bProcessing && bUsersTurn && bUserMoveEnabled && !bSendMessageBackgroundResponse)
				{
				point.x=LOWORD(lParam);
				point.y=HIWORD(lParam);

				if (  ( point.x <= rectClientView.left )
				   || ( point.x >= rectClientView.right )
				   || ( point.y <= rectClientView.top ) 
				   || ( point.y >= rectClientView.bottom )
				   ) return(0); 

				point.x=(point.x+sXClientOrigin)/sGridSize;
				point.y=(point.y+sYClientOrigin)/sGridSize;

				if (  ( point.x < rectBoard.left )
				   || ( point.x > rectBoard.right )
				   || ( point.y < rectBoard.top ) 
				   || ( point.y > rectBoard.bottom )
				   || ( !bEmptyReal(point) )
				   ) return(0); 


				if ( bBackgroundProcessing )
					{
					if	(	( pointBackgroundProcessing.x == point.x )
						&&	( pointBackgroundProcessing.y == point.y ) )
						{
						bSendMessageBackgroundResponse=TRUE;
						bUsersTurn=FALSE;
						SetWindowText(hwnd,szThinking+(sGameEndRows==1)*7);
						hMenu = GetMenu(hwnd);
						for(i=0;i<6;i++)
							EnableMenuItem( hMenu , IDM_SKILL0+i , MF_GRAYED);
						EnableMenuItem( hMenu , IDM_UNDO , MF_GRAYED);
						EnableMenuItem( hMenu , IDM_HINT , MF_GRAYED);
						hClientCursor=LoadCursor(NULL,IDC_WAIT);
						if ( bCursorInClientArea )
							SetCursor(hClientCursor);
						ptUserNew=point;
						FlashBoard(point,2,0);
						}
					else
						{
						bInterruptBackgroundProcessing =TRUE;
						PostMessage(hwnd,message,wParam,lParam);
						}
					}
				else
					{
					bUsersTurn = FALSE;
					PostMessage(hwnd,WM_COMMAND,IDM_USERSPIECE,(((DWORD)point.y)<<16)+(DWORD)point.x);
					}
				}

			return 0;

		case WM_PAINT :
			hdc=BeginPaint(hwnd, &ps);
			Paint(&ps);
			EndPaint(hwnd, &ps) ;
			return 0 ;

		case WM_CREATE :
			ReadInitFile(hwnd);
			hMenu = GetMenu(hwnd);
			if ( !AllocateMemory() )
				{
				MessageBox ( hwnd ,"Not Enough Memory" , szAppLongName ,MB_ICONEXCLAMATION );
				SendMessage(hwnd,WM_CLOSE,0,0);
				return(0);
				}
			Initialize();
			EnableMenuItem( hMenu , IDM_UNDO , MF_GRAYED);
			EnableMenuItem( hMenu , IDM_HINT , MF_GRAYED);
			CheckMenuItem( hMenu , IDM_SKILL0+sSkillLevel , MF_CHECKED);
			CheckMenuItem( hMenu , IDM_NOUGHT+UsersPiece , MF_CHECKED);
			CheckMenuItem( hMenu , IDM_FIRST_USER+bComputersFirstMove , MF_CHECKED);
			CheckMenuItem( hMenu , IDM_ROWS_1+sGameEndRows-1 , MF_CHECKED);
			if (bBackgroundProcessingEnabled) 
				CheckMenuItem( hMenu , IDM_BACKGROUNDPROCESSING , MF_CHECKED);
			UpdateGridSize();
			hdc = GetDC ( hwnd ) ;
			MakeBoardBitmap(hdc);
			ReleaseDC ( hwnd , hdc );
			bCurrentGameStartedByComputer = (GetRandom(0,9) > 5 );
			sUserWins=0;
			sUserLosses=0;
			return (0) ;

		case WM_KEYDOWN:
			switch (wParam)
				{
				case VK_ESCAPE:
					ShowWindow(hwnd,SW_MINIMIZE);
					return 0;
				case VK_LEFT:
				case VK_RIGHT:
				case VK_UP:
				case VK_DOWN:
					GetCursorPos(&point);
					ScreenToClient(hwnd,&point);
					point.x=(point.x+sXClientOrigin)/sGridSize;
					point.y=(point.y+sYClientOrigin)/sGridSize;
					switch (wParam)
						{
						case VK_LEFT:
							point.x--;
							break;
						case VK_RIGHT:
							point.x++;
							break;
						case VK_UP:
							point.y--;
							break;
						case VK_DOWN:
							point.y++;
							break;
						}
					point.x=max(point.x,rectBoard.left);
					point.x=min(point.x,rectBoard.right);
					point.y=max(point.y,rectBoard.top);
					point.y=min(point.y,rectBoard.bottom);
					point.x=point.x*sGridSize+sGridSize/2;
					point.y=point.y*sGridSize+sGridSize/2;
					if ( point.x < rectView.left   ) SendMessage(hwnd,WM_HSCROLL,SB_LINEUP,0);
					if ( point.x > rectView.right  ) SendMessage(hwnd,WM_HSCROLL,SB_LINEDOWN,0);
					if ( point.y < rectView.top    ) SendMessage(hwnd,WM_VSCROLL,SB_LINEUP,0);
					if ( point.y > rectView.bottom ) SendMessage(hwnd,WM_VSCROLL,SB_LINEDOWN,0);
					point.x -= sXClientOrigin;
					point.y -= sYClientOrigin;
					ClientToScreen(hwnd,&point);
					SetCursorPos(point.x,point.y);
					return(0);


				case VK_RETURN:
					GetCursorPos(&point);
					ScreenToClient(hwnd,&point);
					PostMessage(hwnd,WM_LBUTTONUP,0,(((DWORD)point.y)<<16)+(DWORD)point.x);
					return(0);
				}							
			return(0);
			
		case WM_SIZE:
			GetClientRect(hwnd, &rectClient);
			sXClientOrigin -=(rectClient.right-rectClient.left - sOldClientWidth)/2;
			sYClientOrigin -=(rectClient.bottom-rectClient.top - sOldClientHeight)/2;
			sOldClientWidth=rectClient.right-rectClient.left;
			sOldClientHeight=rectClient.bottom-rectClient.top;
			UpdateWindowSize(hwnd, rectClient);
			InvalidateRect(hwnd,NULL,TRUE);
			bMinimized = ( wParam == SIZEICONIC );
			bUserMoveEnabled=( !SetTimer ( hwnd , TIMER_USER_MOVE_ENABLE , 600 , NULL ) );
			return 0 ;

		case WM_KILLFOCUS:
		case WM_NCMOUSEMOVE:
			bCursorInClientArea=FALSE;
			return 0;

		case WM_SETFOCUS:
			bUserMoveEnabled=( !SetTimer ( hwnd , TIMER_USER_MOVE_ENABLE , 600 , NULL ) );
			/* FALL THROUGH */
		case WM_MOUSEMOVE:
			SetCursor(hClientCursor);
			bCursorInClientArea=TRUE;
			return 0;

		
		case WM_COMMAND:
			hMenu = GetMenu(hwnd);
			switch (wParam)
				{
				case IDM_USERSPIECE:
					if ( bProcessing || bBackgroundProcessing ) return 0;
					point.x=LOWORD(lParam);
					point.y=HIWORD(lParam);
					ptUserNew.x=0;
					ptUserNew.y=0;
					if (   ( point.x >= rectBoard.left )&& ( point.x <= rectBoard.right )
						&& ( point.y >= rectBoard.top ) && ( point.y <= rectBoard.bottom ) 
						&&  ( bEmptyReal(point) ) && bGameGoing)
						{
						for(i=0;i<2;i++)
							sBeforeUsersPieceAlreadyFoundRows[i]=sAlreadyFoundRows[i];
						RecordLastPieces(&point,UsersPiece,&bcUserLastChanged);
						bcComputerLastChanged.Coord[0].x = 0;
						EnableMenuItem( hMenu , IDM_HINT, MF_GRAYED);
						if ( (!FindSimplyBest(0,1,vpDummy)) && (!FindSimplyBest(1,1,vpDummy))) 
							{
							GameOverInit(hwnd);
							SetWindowText(hwnd,szDraw+(sGameEndRows==1)*7);
							FlashBoard(point,2,0);
							}
						else if ( GameOver(&point,UsersPiece) ) 
							{
							sUserWins++;
							GameOverInit(hwnd);
							UpdateCaptionTexts();
							SetWindowText(hwnd,szYouWon+(sGameEndRows==1)*7);
							}
						else
							{
							sGameCount++;
							EnableMenuItem( hMenu , IDM_UNDO , MF_ENABLED);
							UpdateCaptionTexts();
							if ( !bSendMessageBackgroundResponse )
								{							
								bUsersTurn=FALSE;
								SetWindowText(hwnd,szThinking+(sGameEndRows==1)*7);
								FlashBoard(point,2,0);
								hClientCursor=LoadCursor(NULL,IDC_WAIT);
								if ( bCursorInClientArea )
									SetCursor(hClientCursor);
								for(i=0;i<6;i++)
									EnableMenuItem( hMenu , IDM_SKILL0+i , MF_GRAYED);
								bProcessing=TRUE;
								vpCompuMove.point.x=0;
								if ( !bBackgroundProcessingEnabled )
									FindBackgroundResponse(&point,ComputersPiece,&vpCompuMove);
								if ( vpCompuMove.point.x == 0)
									FindMyNextPoint(ComputersPiece,&vpCompuMove);
								if(!bInterruptBackgroundProcessing)
									PostMessage(hwnd,WM_COMMAND,IDM_COMPUTERSPIECE,(((DWORD)vpCompuMove.point.y)<<16)+(DWORD)vpCompuMove.point.x);
								bInterruptBackgroundProcessing =FALSE;
								bProcessing=FALSE;
								}
							for(i=0;i<6;i++)
								EnableMenuItem( hMenu , IDM_SKILL0+i , MF_ENABLED);
							bSendMessageBackgroundResponse=FALSE;
							}
						}
					return 0;
				case IDM_COMPUTERSPIECE:
					if ( bProcessing || bBackgroundProcessing ) return 0;
					point.x=LOWORD(lParam);
					point.y=HIWORD(lParam);
					if (   ( point.x >= rectBoard.left )&& ( point.x <= rectBoard.right )
						&& ( point.y >= rectBoard.top ) && ( point.y <= rectBoard.bottom ) 
						&&  ( bEmptyReal(point) )
						&&  bGameGoing )
						{
						EraseBackgroundResults();
						for(i=0;i<2;i++)
							sBeforeComputersPieceAlreadyFoundRows[i]=sAlreadyFoundRows[i];
						RecordLastPieces(&point,ComputersPiece,&bcComputerLastChanged);
						hClientCursor=LoadCursor(NULL,IDC_ARROW);
						EnableMenuItem( hMenu , IDM_HINT, MF_ENABLED);
						if ( bCursorInClientArea )
							SetCursor(hClientCursor);
						if ( (!FindSimplyBest(0,1,vpDummy)) && (!FindSimplyBest(1,1,vpDummy))) 
							{
							GameOverInit(hwnd);
							SetWindowText(hwnd,szDraw+(sGameEndRows==1)*7);
							FlashBoard(point,3,0);
							}
						else if (GameOver(&point,ComputersPiece) ) 
							{
							sUserLosses++;
							GameOverInit(hwnd);
							UpdateCaptionTexts();
							SetWindowText(hwnd,szIWon+(sGameEndRows==1)*7);
							}
						else 
							{
							bUsersTurn=TRUE;
							UpdateCaptionTexts();
							SetWindowText(hwnd,szYourTurn+(sGameEndRows==1)*7);
							FlashBoard(point,3,0);
							if ( ( bBackgroundProcessingEnabled ) && (!bSendMessageBackgroundResponse) )
								{
								bBackgroundProcessing = TRUE;
								ProcessBackground(ComputersPiece);
								bInterruptBackgroundProcessing =FALSE;
								bBackgroundProcessing = FALSE;
								}
							}
						}
					return 0;

				case IDM_NEW:
					if ( (!bProcessing) && (!bBackgroundProcessing) )
						{
						Initialize();
						EraseBackgroundResults();
						FlashBoard(point,-1,-1);
						bGameGoing=TRUE;

						for(i=0;i<9;i++)
							EnableMenuItem( hMenu , IDM_ROWS_1+i , MF_GRAYED);
						EnableMenuItem( hMenu , IDM_NOUGHT , MF_GRAYED);
						EnableMenuItem( hMenu , IDM_CROSS , MF_GRAYED);
						EnableMenuItem( hMenu , IDM_UNDO , MF_GRAYED);
						SendMessage(hwnd,WM_COMMAND,IDM_CENTERIZE,0);
						bUserMoveEnabled=( !SetTimer ( hwnd , TIMER_USER_MOVE_ENABLE , 600 , NULL ) );

						if ( bComputersFirstMove == 2  ) 
							bCurrentGameStartedByComputer = 1- bCurrentGameStartedByComputer ;
						else if ( bComputersFirstMove == 3  ) 
							bCurrentGameStartedByComputer = (GetRandom(0,9) > 5 );
						else
							bCurrentGameStartedByComputer = bComputersFirstMove;

						if ( bCurrentGameStartedByComputer )
							{  
							point.x=(rectBoard.left+rectBoard.right)/2;
							point.y=(rectBoard.top+rectBoard.bottom)/2;
							RecordLastPieces(&point,ComputersPiece,&bcComputerLastChanged);
							FlashBoard(point,3,0);
							sGameCount=1;
							}
						bUsersTurn=TRUE;
						SetWindowText(hwnd,szYourTurn+(sGameEndRows==1)*7);
						}
					else
						{
						bInterruptBackgroundProcessing =TRUE;
						PostMessage(hwnd,message,wParam,lParam);
						}
					return 0;
			case IDM_EXIT:
				PostMessage (hwnd, WM_CLOSE, 0, 0L);
				return 0;
			case IDM_STATISTICS:
				lpfnDlgProc = MakeProcInstance(StatisticsDlgProc,hInst);
				DialogBox(hInst, "StatisticsDlg", hwnd,lpfnDlgProc);
				FreeProcInstance(lpfnDlgProc);
				return 0;
			case IDM_ABOUT:
				lpfnDlgProc = MakeProcInstance(AboutDlgProc,hInst);
				DialogBox(hInst, "AboutDlg", hwnd,lpfnDlgProc);
				FreeProcInstance(lpfnDlgProc);
				return 0;
			case IDM_SKILL0:
			case IDM_SKILL1:
			case IDM_SKILL2:
			case IDM_SKILL3:
			case IDM_SKILL4:
			case IDM_SKILL5:
				if ((bGameGoing)&&(!bUsersTurn))
					return(0);
				if (bBackgroundProcessing)
					{
					bInterruptBackgroundProcessing =TRUE;
					PostMessage(hwnd,message,wParam,lParam);
					return(0);
					}
				bInitfileChange=TRUE;
				CheckMenuItem( hMenu , IDM_SKILL0+sSkillLevel , MF_UNCHECKED);
				if ( sSkillLevel != (wParam-IDM_SKILL0) )
					{
					sUserWins=0;
					sUserLosses=0;
					}
				if ( ( sSkillLevel < 2 ) || ( (wParam-IDM_SKILL0) < 2 ))
					{
					sSkillLevel=wParam-IDM_SKILL0;
					EraseBackgroundResults();
					TranslateBoardSkillLevel();
					}
				sSkillLevel=wParam-IDM_SKILL0;
				CheckMenuItem( hMenu , IDM_SKILL0+sSkillLevel , MF_CHECKED);
				return 0;
			case IDM_ZOOMOUT:
			case IDM_ZOOMIN:
				bInitfileChange=TRUE;
				sOldGridSize=sGridSize;
				if ( wParam == IDM_ZOOMOUT)
					sGridSize=max(MIN_GRID_SIZE,sGridSize*3/4);
				else
					sGridSize=min(MAX_GRID_SIZE,sGridSize*4/3);
				sXClientOrigin=(short)(((float)sXClientOrigin*(float)sGridSize/(float)sOldGridSize) + ((float)(rectView.right-rectView.left)/2.0)*((float)sGridSize/(float)sOldGridSize-1.0));
				sYClientOrigin=(short)(((float)sYClientOrigin*(float)sGridSize/(float)sOldGridSize) + ((float)(rectView.bottom-rectView.top)/2.0)*((float)sGridSize/(float)sOldGridSize-1.0));
				UpdateGridSize();
				hdc = GetDC ( hwnd ) ;
				MakeBoardBitmap(hdc);
				ReleaseDC ( hwnd , hdc );
				PostMessage(hwnd,WM_SIZE,0,0);
				return 0;
			case IDM_CENTERIZE:
				if ( sGameCount == 0 )
					{
	 				sXClientOrigin = rectBoard.right+rectBoard.left;
	 				sYClientOrigin = rectBoard.bottom+rectBoard.top;
					}
				else
					{
	 				sXClientOrigin = rectUsed.right+rectUsed.left;
	 				sYClientOrigin = rectUsed.bottom+rectUsed.top;
					}
	 			sXClientOrigin =(sXClientOrigin+1)*sGridSize/2-(rectClientView.right-rectClientView.left)/2;
	 			sYClientOrigin =(sYClientOrigin+1)*sGridSize/2-(rectClientView.bottom-rectClientView.top)/2;
				PostMessage(hwnd,WM_SIZE,0,0);
				return 0;

			case IDM_COMPUTERSLAST:
				if (bcComputerLastChanged.Coord[0].x != 0)
					FlashBoard(bcComputerLastChanged.Coord[0],3,0);
				return(0);
			case IDM_HINT:
				if ( bGameGoing && bUsersTurn && bUserMoveEnabled && !bSendMessageBackgroundResponse && !bProcessing)
					{
					if ( !bBackgroundProcessing )
						{
						hClientCursor=LoadCursor(NULL,IDC_WAIT);
						FindMyNextPoint(ComputersPiece,&vpCompuMove);
						point.x=vpCompuMove.point.x*sGridSize+sGridSize/2;
						point.y=vpCompuMove.point.y*sGridSize+sGridSize/2;
						point.x -= sXClientOrigin;
						point.y -= sYClientOrigin;
						ClientToScreen(hwnd,&point);
						hClientCursor=LoadCursor(NULL,IDC_ARROW);
						SetCursorPos(point.x,point.y);
						}
					else
						{
						bInterruptBackgroundProcessing =TRUE;
						PostMessage(hwnd,message,wParam,lParam);
						}
					}
				return 0;
			case IDM_UNDO:
				if (!bSendMessageBackgroundResponse)
					{
					if ( (!bProcessing) && (!bBackgroundProcessing) )
						{
						EraseBackgroundResults();
						if (bcComputerLastChanged.Coord[0].x != 0)
							{
							TakeAwayPiece(&bcComputerLastChanged);
							for(i=0;i<2;i++)
								sAlreadyFoundRows[i]=sBeforeComputersPieceAlreadyFoundRows[i];
							}
						else
							{
							for(i=0;i<2;i++)
								sAlreadyFoundRows[i]=sBeforeUsersPieceAlreadyFoundRows[i];
							}
						TakeAwayPiece(&bcUserLastChanged);
						bcComputerLastChanged.Coord[0].x = 0;
						bcUserLastChanged.Coord[0].x = 0;
						bUsersTurn=TRUE;
						UpdateCaptionTexts();
						SetWindowText(hwnd,szYourTurn+(sGameEndRows==1)*7);
						hClientCursor=LoadCursor(NULL,IDC_ARROW);
						InvalidateRect(hwnd,NULL,TRUE);
						bcUserLastChanged.Coord[0].x == 0;
						bcComputerLastChanged.Coord[0].x == 0;
						EnableMenuItem( hMenu , IDM_UNDO , MF_GRAYED);
						if ( ( bBackgroundProcessingEnabled ) && (!bSendMessageBackgroundResponse) )
							{
							bBackgroundProcessing = TRUE;
							ProcessBackground(ComputersPiece);
							bInterruptBackgroundProcessing =FALSE;
							bBackgroundProcessing = FALSE;
							}
						}
					else
						{
						bInterruptBackgroundProcessing =TRUE;
						PostMessage(hwnd,message,wParam,lParam);
						}
					}
				return 0;

			case IDM_NOUGHT:
			case IDM_CROSS:
			if ( !bGameGoing )
				{
				bInitfileChange=TRUE;
				CheckMenuItem( hMenu , IDM_NOUGHT+UsersPiece , MF_UNCHECKED);
				UsersPiece = wParam-IDM_NOUGHT;
				ComputersPiece = 1-UsersPiece;
				CheckMenuItem( hMenu , IDM_NOUGHT+UsersPiece , MF_CHECKED);
				}
				return 0;
			case IDM_FIRST_USER:
			case IDM_FIRST_COMPUTER:
			case IDM_FIRST_ALTERNATING:
			case IDM_FIRST_RANDOM:
				bInitfileChange=TRUE;
				CheckMenuItem( hMenu , IDM_FIRST_USER+bComputersFirstMove , MF_UNCHECKED);
				bComputersFirstMove= wParam-IDM_FIRST_USER;
				CheckMenuItem( hMenu , IDM_FIRST_USER+bComputersFirstMove , MF_CHECKED);
				return 0;

			case IDM_ROWS_1:
			case IDM_ROWS_2:
			case IDM_ROWS_3:
			case IDM_ROWS_4:
			case IDM_ROWS_5:
			case IDM_ROWS_6:
			case IDM_ROWS_7:
			case IDM_ROWS_8:
			case IDM_ROWS_9:
				bInitfileChange=TRUE;
				CheckMenuItem( hMenu , IDM_ROWS_1+sGameEndRows-1 , MF_UNCHECKED);
				sGameEndRows= wParam-IDM_ROWS_1+1;
				CheckMenuItem( hMenu , IDM_ROWS_1+sGameEndRows-1 , MF_CHECKED);
				return 0;

			case IDM_BACKGROUNDPROCESSING:
				bInitfileChange=TRUE;
				bBackgroundProcessingEnabled= 1-bBackgroundProcessingEnabled;
				if (bBackgroundProcessingEnabled)
					CheckMenuItem( hMenu , IDM_BACKGROUNDPROCESSING , MF_CHECKED);
				else	CheckMenuItem( hMenu , IDM_BACKGROUNDPROCESSING , MF_UNCHECKED);
				return 0;
			case IDM_HELP:
				WinHelp(hwnd,(LPSTR)szHelpFileName,HELP_INDEX,0);
				return 0;
			case IDM_HELPHOW:
				WinHelp(hwnd,(LPSTR)szHelpFileName,HELP_KEY , (DWORD)((LPSTR) "PlayingTheGame"));
				return 0;
			case IDM_HELPCOMMANDS:
				WinHelp(hwnd,(LPSTR)szHelpFileName,HELP_KEY , (DWORD)((LPSTR) "Commands"));
				return 0;
			case IDM_HELPKEYBOARD:
				WinHelp(hwnd,(LPSTR)szHelpFileName, HELP_KEY , (DWORD)((LPSTR) "Keyboard") );
				return 0;
			case IDM_HELPCOPYRIGHT:
				WinHelp(hwnd,(LPSTR)szHelpFileName, HELP_KEY , (DWORD)((LPSTR) "Copyright") );
				return 0;
			case IDM_HELPLICENSE:
				WinHelp(hwnd,(LPSTR)szHelpFileName, HELP_KEY , (DWORD)((LPSTR) "License") );
				return 0;
			case IDM_HELPUSING:
				WinHelp(hwnd,(LPSTR)szHelpFileName,HELP_HELPONHELP,0);
				return 0;
			}
		break;
				

		case WM_CLOSE:
			if (( !bProcessing ) && (!bBackgroundProcessing ))
				DestroyWindow ( hwnd );
			else
				{
				bInterruptBackgroundProcessing =TRUE;
				PostMessage(hwnd,message,wParam,lParam);
				}
			return 0;
			
		case WM_QUERYENDSESSION:
			if ( bProcessing || (bBackgroundProcessing ))
				{
				bInterruptBackgroundProcessing =TRUE;
				}
			return 1L;
		case WM_DESTROY:
			CleanUp();
			if ( bInitfileChange )
				WriteInitFile();
			DeAllocateMemory();
			PostQuitMessage (0);
			return 0;
		}
	return DefWindowProc ( hwnd, message, wParam, lParam);
	}


void _near UpdateCaptionTexts(void)
	{
				/* update numbers to the caption bar strings */
				szThinking[0]='0'+sAlreadyFoundRows[UsersPiece];
				szYourTurn[0]='0'+sAlreadyFoundRows[UsersPiece];
				szIWon[0]='0'+sAlreadyFoundRows[UsersPiece];
				szYouWon[0]='0'+sAlreadyFoundRows[UsersPiece];
				szDraw[0]='0'+sAlreadyFoundRows[UsersPiece];
				szThinking[4]='0'+sAlreadyFoundRows[ComputersPiece];
				szYourTurn[4]='0'+sAlreadyFoundRows[ComputersPiece];
				szIWon[4]='0'+sAlreadyFoundRows[ComputersPiece];
				szYouWon[4]='0'+sAlreadyFoundRows[ComputersPiece];
				szDraw[4]='0'+sAlreadyFoundRows[ComputersPiece];
	}




BOOL _near  Initialize(void)
	{
	InitAlgorithmNewGame();

	bcUserLastChanged.Coord[0].x == 0;
	bcComputerLastChanged.Coord[0].x == 0;
	sBeforeComputersPieceAlreadyFoundRows[0]=0;
	sBeforeComputersPieceAlreadyFoundRows[1]=0;
	sBeforeUsersPieceAlreadyFoundRows[0]=0;
	sBeforeUsersPieceAlreadyFoundRows[1]=0;
	UpdateCaptionTexts();

	ptUserNew.x=0;
	ptUserNew.y=0;

	bProcessing=FALSE;
	bBackgroundProcessing=FALSE;
	bSendMessageBackgroundResponse=FALSE;
	bInterruptBackgroundProcessing=FALSE;
	hClientCursor=LoadCursor(NULL,IDC_ARROW);
	if ( bCursorInClientArea )
		SetCursor(hClientCursor);
	}


void _near GameOverInit(HWND hwnd)
	{
	HMENU	hMenu;
	short i;

	bGameGoing=FALSE;
	hMenu=GetMenu(hwnd);
	EnableMenuItem( hMenu , IDM_NOUGHT , MF_ENABLED);
	EnableMenuItem( hMenu , IDM_CROSS , MF_ENABLED);
	for(i=0;i<9;i++)
		EnableMenuItem( hMenu , IDM_ROWS_1+i , MF_ENABLED);
	EnableMenuItem( hMenu , IDM_UNDO , MF_GRAYED);
	EnableMenuItem( hMenu , IDM_HINT , MF_GRAYED);
	}


int FAR PASCAL	 AboutDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
	{
	switch ( message )
		{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			if ( wParam == IDD_OK )
				{
				EndDialog(hDlg,TRUE);
				return TRUE;
				}
			else return FALSE;
		default:
			return FALSE;
		}
	return FALSE;
	}


int FAR PASCAL	 StatisticsDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
	{
	short i;
	char buffer[5];
	switch ( message )
		{
		case WM_INITDIALOG:
            		SetDlgItemInt(hDlg,IDD_WINS,sUserWins,FALSE);
           		SetDlgItemInt(hDlg,IDD_LOSSES,sUserLosses,FALSE);
			if( (sUserWins+sUserLosses)==0 ) 
				i=0; 
			else
				i=max(0,min(100,(short)(0.5+100.0*(float)sUserWins/(float)(sUserWins+sUserLosses))));
			*_itoa(i,buffer,10);
			i=0;
			while (( i<3 ) && (buffer[i]!=0)) i++; 
			buffer[i]='%';
			buffer[i+1]=0;
            		SetDlgItemText(hDlg,IDD_PERCENT,buffer);
			return FALSE;
		case WM_COMMAND:
			if ( wParam == IDD_OK )
				{
				EndDialog(hDlg,TRUE);
				return TRUE;
				}
			else return FALSE;
		default:
			return FALSE;
		}
	return FALSE;
	}
