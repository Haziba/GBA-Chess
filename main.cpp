#include <stdint.h>
#include <stdlib.h>
#include "gba.h"
#include "objects.h"
#include "tiles.h"
#include "font.h"

#define BOARD_OFFSET_X 2
#define BOARD_OFFSET_Y 2

enum PieceColours {WHITE = 0, BLACK = 1};
enum PieceNames {KING, QUEEN, BISHOP, KNIGHT, ROOK, PAWN};
enum TileNames {WHITE_TILE, BLACK_TILE, WHITE_R_TILE, BLACK_R_TILE, WHITE_G_TILE, BLACK_G_TILE};
enum Colors {RED, GREEN};

int _pieces[2][6];
int _tileNames[6];
int _pieceLayout[8][8] = 
	{
		{10,  9,  8,  7,  6,  8,  9, 10},
		{11, 11, 11, 11, 11, 11, 11, 11},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1},
		{ 5,  5,  5,  5,  5,  5,  5,  5},
		{ 4,  3,  2,  1,  0,  2,  3,  4},
	};

int _lostPieces[2][16];

// int _pieceLayout[8][8] =
	// {
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1,  2, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
		// {-1, -1, -1, -1, -1, -1, -1, -1},
	// };

int PieceType(int x, int y);
int PieceColor(int x, int y);
bool PieceAt(int x, int y);
void ClearHighlights();
void Highlight(int x, int y, int c);
void PlacePieces();
void DrawChessBoard();
void DrawFourTilesAt(int x, int y, int sbb, int tileStart);
void EraseFourTilesAt(int x, int y, int sbb);
bool TileIsWhite(int x, int y);
void InitialiseStage();
void LoadAssets();
int AddSquareBGTile(uint8_t t[4][64], int pos);
void AvailableMoves(int x, int y, int* moves, int* takingMoves);
void MovePiece(int cx, int cy, int nx, int ny);
void TakePiece(int cx, int cy, int nx, int ny);


class Cursor
{
private:
	int _x, _y, _id;
public:
	Cursor(int objId)
	{
		_id = objId;
		
		SetObject(_id,
				  ATTR0_SHAPE(0) | ATTR0_8BPP | ATTR0_REG | ATTR0_Y(0),
				  ATTR1_SIZE(1) | ATTR1_X(0),
				  ATTR2_ID8(0));
		
		_x = 0;
		_y = 0;
		
		SetObjectX(_id, (_x + BOARD_OFFSET_X / 2) * 16);
		SetObjectY(_id, (_y + BOARD_OFFSET_Y / 2) * 16);
	}
	int X()
	{
		return _x;
	}
	int Y()
	{
		return _y;
	}
	void Move(int xMove, int yMove)
	{
		if(_x + xMove >= 0 && _x + xMove < 8)
			_x += xMove;
		if(_y + yMove >= 0 && _y + yMove < 8)
			_y += yMove;
		
		SetObjectX(_id, (_x + BOARD_OFFSET_X / 2) * 16);
		SetObjectY(_id, (_y + BOARD_OFFSET_Y / 2) * 16);
	}
};

void DrawString(int x, int y, const char* line, int lineLength)
{
	for(int i = 0; i < lineLength; i++)
		SetTile(30, x + i, y, (int)line[i]);
}

int main()
{
	LoadAssets();
	ClearObjects();
	InitialiseStage();
	
	uint16_t oldKeys = REG_P1;
	
	Cursor cursor = Cursor(0);
	
	bool pieceSelected = false;
	int currentPiece = 0;
	int* moves = new int[28];
	int* takingMoves = new int[16];
	
	int currentTurnColor = WHITE;
	for(int i = 0; i < 2; i++)
		for(int j = 0; j < 16; j++)
			_lostPieces[i][j] = -1;
	
	
	while (true)
	{
		if(!(REG_P1 & KEY_RIGHT) && (oldKeys & KEY_RIGHT))
			cursor.Move(1,  0);
		if(!(REG_P1 & KEY_LEFT) && (oldKeys & KEY_LEFT))
			cursor.Move(-1, 0);
		if(!(REG_P1 & KEY_DOWN) && (oldKeys & KEY_DOWN))
			cursor.Move(0,  1);
		if(!(REG_P1 & KEY_UP) && (oldKeys & KEY_UP))
			cursor.Move(0, -1);
		
		if(!(REG_P1 & KEY_A) && (oldKeys & KEY_A))
		{
			if(PieceAt(cursor.X(), cursor.Y()))
			{
				pieceSelected = true;
				for(int i = 0; i < 16 && takingMoves[i] >= 0; i++)
					if(takingMoves[i] % 8 == cursor.X() && takingMoves[i] / 8 == cursor.Y())
					{
						TakePiece(currentPiece % 8, currentPiece / 8, cursor.X(), cursor.Y());
						currentTurnColor = (currentTurnColor == WHITE ? BLACK : WHITE);
						pieceSelected = false;
						ClearHighlights();
						takingMoves[0] = -1;
						break;
					}
				
				if(pieceSelected && PieceColor(cursor.X(), cursor.Y()) == currentTurnColor)
				{
					ClearHighlights();
					
					AvailableMoves(cursor.X(), cursor.Y(), moves, takingMoves);
					
					currentPiece = cursor.Y() * 8 + cursor.X();
					
					for(int i = 0; i < 28 && moves[i] >= 0; i++)
						Highlight(moves[i] % 8, moves[i] / 8, GREEN);
					
					for(int i = 0; i < 16 && takingMoves[i] >= 0; i++)
						Highlight(takingMoves[i] % 8, takingMoves[i] / 8, RED);
				}
			}
			else if(pieceSelected)
			{
				ClearHighlights();
				
				if(!PieceAt(cursor.X(), cursor.Y()))
				{
					for(int i = 0; i < 28 && moves[i] >= 0; i++)
						if(moves[i] % 8 == cursor.X() && moves[i] / 8 == cursor.Y())
						{
							MovePiece(currentPiece % 8, currentPiece / 8, cursor.X(), cursor.Y());
							currentTurnColor = (currentTurnColor == WHITE ? BLACK : WHITE);
							pieceSelected = false;
							break;
						}
				}
			}
		}
		
		oldKeys = REG_P1;
		
		if(currentTurnColor == WHITE)
			DrawString(20, 1, "WHITE", 5);
		else
			DrawString(20, 1, "BLACK", 5);
	
		WaitVSync();
		UpdateObjects();
	}

	return 0;
}

void MovePiece(int cx, int cy, int nx, int ny)
{
	DrawFourTilesAt(nx * 2 + BOARD_OFFSET_X, ny * 2 + BOARD_OFFSET_Y, 29, _pieces[PieceColor(cx, cy)][PieceType(cx, cy)]);
	EraseFourTilesAt(cx * 2 + BOARD_OFFSET_X, cy * 2 + BOARD_OFFSET_Y, 29);
	
	_pieceLayout[ny][nx] = _pieceLayout[cy][cx];
	_pieceLayout[cy][cx] = -1;
}

void TakePiece(int cx, int cy, int nx, int ny)
{
	MovePiece(cx, cy, nx, ny);
}

void HorizontalMovesFrom(int x, int y, int* moves, int &nextMoveSlot, int* takingMoves, int &nextTMoveSlot)
{
	for(int i = x + 1; i < 8; i++)
		if(!PieceAt(i, y))
			moves[nextMoveSlot++] = y * 8 + i;
		else if(PieceColor(i, y) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = y * 8 + i;
			break;
		}
		else
			break;
	for(int i = x - 1; i >= 0; i--)
		if(!PieceAt(i, y))
			moves[nextMoveSlot++] = y * 8 + i;
		else if(PieceColor(i, y) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = y * 8 + i;
			break;
		}
		else 
			break;
	for(int i = y + 1; i < 8; i++)
		if(!PieceAt(x, i))
			moves[nextMoveSlot++] = i * 8 + x;
		else if(PieceColor(x, i) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = i * 8 + x;
			break;
		}
		else
			break;
	for(int i = y - 1; i >= 0; i--)
		if(!PieceAt(x, i))
			moves[nextMoveSlot++] = i * 8 + x;
		else if(PieceColor(x, i) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = i * 8 + x;
			break;
		}
		else
			break;
}

void DiagonalMovesFrom(int x, int y, int* moves, int &nextMoveSlot, int* takingMoves, int &nextTMoveSlot)
{
	for(int i = 0; i < x && i < y; i++)
	{
		int nx = x - i - 1, ny = y - i - 1;
		if(!PieceAt(nx, ny))
			moves[nextMoveSlot++] = ny * 8 + nx;
		else if(PieceColor(nx, ny) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = ny * 8 + nx;
			break;
		}
		else
			break;
	}
	for(int i = 0; i < x && y + i < 8; i++)
	{
		int nx = x - i - 1, ny = y + i + 1;
		if(ny < 8)
		{
			if(!PieceAt(nx, ny))
				moves[nextMoveSlot++] = ny * 8 + nx;
			else if(PieceColor(nx, ny) != PieceColor(x, y))
			{
				takingMoves[nextTMoveSlot++] = ny * 8 + nx;
				break;
			}
			else
				break;
		}
	}
	for(int i = x + 1; i < 8; i++)
	{
		int nx = i, ny = y + (i - x);
		if(ny < 8)
		{
			if(!PieceAt(nx, ny))
				moves[nextMoveSlot++] = ny * 8 + nx;
			else if(PieceColor(nx, ny) != PieceColor(x, y))
			{
				takingMoves[nextTMoveSlot++] = ny * 8 + nx;
				break;
			}
			else
				break;
		}
	}
	for(int i = x + 1; i < 8; i++)
	{
		int nx = i, ny = y - (i - x);
		if(!PieceAt(nx, ny))
			moves[nextMoveSlot++] = ny * 8 + nx;
		else if(PieceColor(nx, ny) != PieceColor(x, y))
		{
			takingMoves[nextTMoveSlot++] = ny * 8 + nx;
			break;
		}
		else
			break;
	}
}

void AvailableMoves(int x, int y, int* moves, int* takingMoves)
{
	// Maximum number of moves available at one time is 28 (Queen at C3)
	int kPTC[8][2] = {{-2,-1},{-2,1},{-1,-2},{1,-2},{2,-1},{2,1},{1,2},{-1,2}};
	int kingPTC[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
	for(int i = 0; i < 28; i++)
		moves[i] = -1;
	for(int i = 0; i < 16; i++)
		takingMoves[i] = -1;
	if(PieceAt(x, y))
	{
		int colour = PieceColor(x, y);
		int type = PieceType(x, y);
		int nextMoveSlot = 0;
		int nextTMoveSlot = 0;
		
		switch(type)
		{
			case KING:
				for(int i = 0; i < 8; i++)
				{
					int nx = x + kingPTC[i][0], ny = y + kingPTC[i][1];
					if(nx >= 0 && ny >= 0 && nx < 8 && ny < 8)
					{
						if(!PieceAt(nx, ny))
							moves[nextMoveSlot++] = ny * 8 + nx;
						else if(PieceColor(nx, ny) != colour)
							takingMoves[nextTMoveSlot++] = ny * 8 + nx;
					}
				}
				break;
			case QUEEN:
				HorizontalMovesFrom(x, y, moves, nextMoveSlot, takingMoves, nextTMoveSlot);
				DiagonalMovesFrom(x, y, moves, nextMoveSlot, takingMoves, nextTMoveSlot);
				break;
			case BISHOP:
				DiagonalMovesFrom(x, y, moves, nextMoveSlot, takingMoves, nextTMoveSlot);
				break;
			case ROOK:
				HorizontalMovesFrom(x, y, moves, nextMoveSlot, takingMoves, nextTMoveSlot);
				break;
			case KNIGHT:
				for(int i = 0; i < 8; i++)
				{
					int nx = x + kPTC[i][0], ny = y + kPTC[i][1];
					if(nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
					{
						if(!PieceAt(nx, ny))
							moves[nextMoveSlot++] = ny * 8 + nx;
						else if(PieceColor(nx, ny) != colour)
							takingMoves[nextTMoveSlot++] = ny * 8 + nx;
					}
				}
				break;
			case PAWN:
				if(colour == WHITE)
				{
					for(int i = 0; i < (y == 6 ? 2 : 1); i++)
						if(!PieceAt(x, y - 1 - i))
							moves[nextMoveSlot++] = (y - 1 - i) * 8 + x;
						else
							break;
					if(x > 0)
						if(PieceAt(x - 1, y - 1) && PieceColor(x - 1, y - 1) != colour)
							takingMoves[nextTMoveSlot++] = (y - 1) * 8 + x - 1;
					if(x < 7)
						if(PieceAt(x + 1, y - 1) && PieceColor(x + 1, y - 1) != colour)
							takingMoves[nextTMoveSlot++] = (y - 1) * 8 + x + 1;
				}
				else
				{
					for(int i = 0; i < (y == 1 ? 2 : 1); i++)
						if(!PieceAt(x, y + 1 + i))
							moves[nextMoveSlot++] = (y + 1 + i) * 8 + x;
						else
							break;
					if(x > 0)
						if(PieceAt(x - 1, y + 1) && PieceColor(x - 1, y + 1) != colour)
							takingMoves[nextTMoveSlot++] = (y + 1) * 8 + x - 1;
					if(x < 7)
						if(PieceAt(x + 1, y + 1) && PieceColor(x + 1, y + 1) != colour)
							takingMoves[nextTMoveSlot++] = (y + 1) * 8 + x + 1;
				}
				break;
		}
	}
}

int AddSquareBGTile(uint8_t t[4][64], int pos)
{
	for(int i = 0; i < 4; i++)
		LoadTile8(1, 1 + (pos * 4) + i, t[i]);
	return 1 + (pos * 4);
}

void LoadAssets()
{
	SetPaletteObj(0, RGB(0, 0, 0));
	for(int i = 0; i < OBJ_PALETTE_SIZE; i++)
		SetPaletteObj(1 + i, _objPalette[i]);
	
	SetPaletteBG(0, RGB(0, 0, 0));
	for(int i = 0; i < TILE_PALETTE_SIZE; i++)
		SetPaletteBG(1 + i, _tilePalette[i]);
		
		
	int tileCount = 0;
	_tileNames[WHITE_TILE] = AddSquareBGTile(_whiteTile, tileCount);
	tileCount++;
	_tileNames[BLACK_TILE] = AddSquareBGTile(_blackTile, tileCount);
	tileCount++;
	_tileNames[WHITE_R_TILE] = AddSquareBGTile(_whiteRedTile, tileCount);
	tileCount++;
	_tileNames[BLACK_R_TILE] = AddSquareBGTile(_blackRedTile, tileCount);
	tileCount++;
	_tileNames[WHITE_G_TILE] = AddSquareBGTile(_whiteGreenTile, tileCount);
	tileCount++;
	_tileNames[BLACK_G_TILE] = AddSquareBGTile(_blackGreenTile, tileCount);
	tileCount++;
	_pieces[WHITE][KING] = AddSquareBGTile(_whiteKingTile, tileCount);
	tileCount++;
	_pieces[WHITE][QUEEN] = AddSquareBGTile(_whiteQueenTile, tileCount);
	tileCount++;
	_pieces[WHITE][BISHOP] = AddSquareBGTile(_whiteBishopTile, tileCount);
	tileCount++;
	_pieces[WHITE][KNIGHT] = AddSquareBGTile(_whiteKnightTile, tileCount);
	tileCount++;
	_pieces[WHITE][ROOK] = AddSquareBGTile(_whiteRookTile, tileCount);
	tileCount++;
    _pieces[WHITE][PAWN] = AddSquareBGTile(_whitePawnTile, tileCount);
	tileCount++;
	_pieces[BLACK][KING] = AddSquareBGTile(_blackKingTile, tileCount);
	tileCount++;
	_pieces[BLACK][QUEEN] = AddSquareBGTile(_blackQueenTile, tileCount);
	tileCount++;
	_pieces[BLACK][BISHOP] = AddSquareBGTile(_blackBishopTile, tileCount);
	tileCount++;
	_pieces[BLACK][KNIGHT] = AddSquareBGTile(_blackKnightTile, tileCount);
	tileCount++;
	_pieces[BLACK][ROOK] = AddSquareBGTile(_blackRookTile, tileCount);
	tileCount++;
    _pieces[BLACK][PAWN] = AddSquareBGTile(_blackPawnTile, tileCount);
	
	for(int i = 0; i < 4; i++)
		LoadTile8(4, 0 + i, _cursorTile[i]);
	
	for(int i = 0; i < 128; i++)
		LoadTile8(0, i, font_bold[i]);
}

void InitialiseStage()
{
	REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3;
	
	
	REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32;
	REG_BG1CNT = BG_CBB(1) | BG_SBB(29) | BG_8BPP | BG_REG_32x32;
	REG_BG2CNT = BG_CBB(1) | BG_SBB(28) | BG_8BPP | BG_REG_32x32;
	REG_BG3CNT = BG_CBB(1) | BG_SBB(27) | BG_8BPP | BG_REG_32x32;
	
	DrawChessBoard();
	PlacePieces();
}

bool TileIsWhite(int x, int y)
{
	bool t = false;
	if(y & 1)
	{
		if(x & 1)
			t = true;
	}
	else
		if(!(x & 1))
			t = true;
	return t;
}

void DrawFourTilesAt(int x, int y, int sbb, int tileStart)
{
	SetTile(sbb, x, y, tileStart);
	SetTile(sbb, x + 1, y, tileStart + 1);
	SetTile(sbb, x, y + 1, tileStart + 2);
	SetTile(sbb, x + 1, y + 1, tileStart + 3);
}

void EraseFourTilesAt(int x, int y, int sbb)
{
	SetTile(sbb, x, y, 0);
	SetTile(sbb, x + 1, y, 0);
	SetTile(sbb, x, y + 1, 0);
	SetTile(sbb, x + 1, y + 1, 0);
}

void DrawChessBoard()
{
	for(int i = 0; i < 64; i++)
		DrawFourTilesAt((i % 8) * 2 + BOARD_OFFSET_X, (i / 8) * 2 + BOARD_OFFSET_Y, 27, (TileIsWhite(i % 8, i / 8) ? _tileNames[WHITE_TILE] : _tileNames[BLACK_TILE]));
}

void PlacePieces()
{
	for(int i = 0; i < 8; i++)
		for(int j = 0; j < 8; j++)
			if(PieceAt(i, j))
				DrawFourTilesAt((i * 2) + BOARD_OFFSET_X, (j * 2) + BOARD_OFFSET_Y, 29, _pieces[PieceColor(i, j)][PieceType(i, j)]);
}

void Highlight(int x, int y, int c)
{
	int tileToDraw = 0;
	if(TileIsWhite(x, y))
	{
		if(c == RED)
			tileToDraw = _tileNames[WHITE_R_TILE];
		else if (c == GREEN)
			tileToDraw = _tileNames[WHITE_G_TILE];
	}
	else
	{
		if(c == RED)
			tileToDraw = _tileNames[BLACK_R_TILE];
		else if(c == GREEN)
			tileToDraw = _tileNames[BLACK_G_TILE];
	}
	
	DrawFourTilesAt(x * 2 + BOARD_OFFSET_X, y * 2 + BOARD_OFFSET_Y, 28, tileToDraw);
}

void ClearHighlights()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			SetTile(28, i + BOARD_OFFSET_X, j + BOARD_OFFSET_Y, 0);
}

bool PieceAt(int x, int y)
{
	return _pieceLayout[y][x] >= 0;
}

int PieceColor(int x, int y)
{
	return (_pieceLayout[y][x] > 5 ? BLACK : WHITE);
}

int PieceType(int x, int y)
{
	return PieceNames(_pieceLayout[y][x] % 6);
}
