// game.cpp

#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "glut.h"
#include "Game.h"

using namespace std;

/*
每當計時器到期時調用，但遊戲沒有結束或暫停
*/
void Game::update() {
	// Check pieces collision
	if (moveCollision(0)) {					// 如果發生碰撞
		if (activePiece.y <= 2) {			// 檢查它是否結束遊戲
			killed = true;
		}
		else {								// 如果遊戲仍然活躍
			updateActiveAfterCollision();	// 人物固定代替碰撞
			linesCleared_score = linesCleared;
			checkLine();					// 檢查填充的行

			/* 
				分數計算 
				公式 mark = 10 * Combo * Combo
				Combo = number of rows disappeared for a single block
			*/
			if (linesCleared - linesCleared_score >=1 )
			{
				// temp用來計算消塊的數量
				int temp_linesCleared = linesCleared - linesCleared_score;
				// 公式
				linesCleared += (linesCleared - linesCleared_score) * (linesCleared - linesCleared_score) * 10;
				// 剪掉checkLine會多加
				linesCleared -= temp_linesCleared;
			}
				
			/*
			如果分數提升則等級提升速度變快
			公式  the falling speed equals to [1000msec –(level – 1) * 100] per grid-square
			*/
			if(linesCleared / 30 >= 1)
				level = linesCleared / 30;
			
			timer = 1000 - (level - 1) * 100;
			// level最大為10，完成則結束
			if (level == 10)
				restart();

				
			if (deleteLines)				// 如果發現行被刪除，
				clearLine();				// 填充的線被刪除
			genNextPiece();					// 產生新piece

			clearNextPieceGrid();
			updateNextPieceGrid();

			updateActivePiece();			// 它在遊戲網格中的更新
		}
	}
	else {									// 如果沒有碰撞，這塊會掉到下面
		fixActivePiece();
		activePiece.y++;
		updateActivePiece();
	}
}

/*
開始一個新遊戲並初始化所需的元素
*/
void Game::restart()
{
	clearMainGrid();		// Cleaning the main grid
	clearNextPieceGrid();	// Clearing the grid with the next figure
	linesCleared = 0;		// The player's score is zero
	linesCleared_score = 0;
	level = 1;
	shapesCount = 1;		// The counter of pieces per game is equal to one 每場比賽的棋子數等於一
	killed = false;
	paused = false;
	deleteLines = false;

	// Generating the current random piece 生成當前的隨機片段
	activePiece = Piece(rand() % numPieces);
	/*
	作業要求 
		The starting position of a block is at the middle of the third top row.
		Suppose the width of a row is A, the x coordinate of the center of the new block is either floor[A / 2] or ceiling[A / 2].
	*/
	activePiece.x = COLS/2;
	activePiece.y = 0;
	updateActivePiece();

	// Generating the next piece 生成下一塊
	nextPiece = Piece(rand() % numPieces);
	nextPiece.x = COLS/2;
	nextPiece.y = 0;
	updateNextPieceGrid();
}

/*
Updating the game grid and the correct display of the active figure when falling 更新遊戲網格和掉落時活動人物的正確顯示
*/
void Game::fixActivePiece() {
	// Determining the data of the current piece by its type and position 通過其類型和位置確定當前片斷的數據
	const int* trans = activePiece.rotations();
	for(int i = 0; i < 8; i += 2){
		Square &square = mainGrid[activePiece.y + trans[i + 1]][activePiece.x + trans[i]];
		// Setting active and inactive cells 設置活動和非活動單元格
		square.isFilled = false;
		square.isActive = false;
	}
}

// 創建新的寶石
void Game::genNextPiece() {
	activePiece = nextPiece;
	nextPiece = Piece(rand() % numPieces);
	nextPiece.x = COLS/2;
	nextPiece.y = 0;
	// Increase the piece counter per game
	shapesCount++;
}

/*
Move the active piece left and right 左右移動活動塊
*/
void Game::move(int dir)
{
	if(moveCollision(dir))	// If there is a collision with one of the borders, 如果與其中一個邊界發生碰撞，
		return;				// nothing happens 沒發生什麼事
	fixActivePiece();
	activePiece.x += dir;
	updateActivePiece();
}

/*
Cleaning the main grid 
清潔主電網
*/
void Game::clearMainGrid()
{
	for (int r=0; r<ROWS; r++) {
		for (int c=0; c<COLS; c++) {
			mainGrid[r][c].isFilled = false;
			mainGrid[r][c].isActive = false;
		}
	}
}

/*
Clearing the grid with the next piece 
用下一塊清除網格
*/
void Game::clearNextPieceGrid()
{
	for (int r = 0; r < 5; r++) {	
		for (int c = 0; c < 5; c++) {
			nextPieceGrid[r][c].isFilled = false;
			nextPieceGrid[r][c].isActive = false;
		}
	}
}

/*
Updating the position of the active piece with rotation 通過旋轉更新活動塊的位置
*/
void Game::updateActivePiece() {
	// Pointer to an array that stores all conversions 指向存儲所有轉換的數組的指針
	const int* trans = activePiece.rotations();
	for(int i = 0; i < 8; i += 2){
		// Find the active piece in the game grid 在遊戲網格中找到活動的棋子
		Square &square = mainGrid[activePiece.y + trans[i + 1]][activePiece.x + trans[i]];
		// Convert the active piece to filled grid cells 將活動塊轉換為填充的網格單元
		square.isFilled = true;
		square.isActive = true;
		square.red = activePiece.redVal;
		square.green = activePiece.blueVal;
		square.blue = activePiece.greenVal;
	}	
}

/*
Updating the grid with the next piece

用下一塊更新網格
*/
void Game::updateNextPieceGrid() {
	// Pointer to an array that stores all conversions  指向存儲所有轉換的數組的指針
	const int* transNext = nextPiece.rotations();
	for (int i = 0; i < 8; i += 2) {
		// Find the active piece in the game grid 在遊戲網格中找到活動的棋子
		Square& squareNext = nextPieceGrid[nextPiece.y + transNext[i + 1]][nextPiece.x + transNext[i]];
		// Convert the active piece to filled grid cells 將活動塊轉換為填充的網格單元
		squareNext.isFilled = true;
		squareNext.isActive = true;
		squareNext.red = nextPiece.redVal;
		squareNext.green = nextPiece.blueVal;
		squareNext.blue = nextPiece.greenVal;
	}
}

/*
Constructor
*/
Game::Game()
{
	restart();
	timer = 1000;
	level = 1;
	start_flag = 0;
}

/*
Rotate the current piece and check if it can be rotated
旋轉當前塊並檢查它是否可以旋轉
*/
void Game::rotateShape(int dir) {
	// 隨機產生一個設定的寶石
	activePieceCopy = Piece(rand() % numPieces);
	activePieceCopy.x = activePiece.x;
	activePieceCopy.y = activePiece.y;
	activePieceCopy.rotation = activePiece.rotation;
	activePieceCopy.type = activePiece.type;
	activePieceCopy.rotatePiece(dir);
	
	// If the active piece can be rotated, it is rotated and displayed 如果活動塊可以旋轉，則旋轉並顯示
	if(canRotate(activePieceCopy)) {
		fixActivePiece();
		activePiece.rotatePiece(dir);
		updateActivePiece();
	}
}

/*
Checking whether a piece can be rotated
檢查工件是否可以旋轉
*/
bool Game::canRotate(Piece activeP) {
	if(rotationCollision()) {
		return false;
	}
	else
		return true;
}

/*
Checking for collisions when rotating a piece
旋轉工件時檢查碰撞
*/
bool Game::rotationCollision() {
	int x, y;
	const int* trans = activePieceCopy.rotations();
	for (int i = 0; i < 8; i += 2) {
		x = activePieceCopy.x + trans[i];
		y = activePieceCopy.y + trans[i + 1];

		if (x >= COLS || y >= ROWS || x < 0 || (mainGrid[y][x].isFilled && !mainGrid[y][x].isActive))
			return true;
	}
	return false;
}

/*
Checking for collisions when the piece is moving
當工件移動時檢查碰撞
*/
bool Game::moveCollision(int dir) {
	int x, y;
	const int* trans = activePiece.rotations();
	for (int i = 0; i < 8; i += 2) {
		x = activePiece.x + trans[i];
		y = activePiece.y + trans[i + 1];
		if (dir == 0)
			y += 1;
		else
			x += dir;
		if (x >= COLS || y >= ROWS || x < 0 || (mainGrid[y][x].isFilled && !mainGrid[y][x].isActive))
			return true;
	}
	return false;
}

/*
Updating the location of the active piece after a collision
碰撞後更新活動件的位置
*/
void Game::updateActiveAfterCollision() {
	const int* trans = activePiece.rotations();
	for(int i = 0; i < 8; i += 2){
		Square &square = mainGrid[activePiece.y + trans[i + 1]][activePiece.x + trans[i]];
		square.isActive = false;
	}
}

/*
Checking lines for filling and setting filled lines for deleting 
檢查填充行和設置刪除填充行
*/
void Game::checkLine() {
	int fullRows = 0;
	for (int r=0; r<ROWS; r++) {
		bool fullRow = false;
			for (int c=0; c<COLS; c++) {
				Square &square = mainGrid[r][c];
				if (square.isFilled){
					fullRow = true;
					
				}
				else {
					fullRow = false;
					break;
				}
			}
		if(fullRow) {
			for ( int c =0; c < COLS; c++){
				mainGrid[r][c].toBeDeleted = true;
				//linesCleared_score++;
				
			}
			deleteLines = true;
			linesCleared++;
		}
	}
}

/*
Remove a filled row and move all pieces up one cell down
刪除填充的行並將所有部分向上移動一個單元格
*/
void Game::clearLine() {
	for (int r = ROWS-1; r > 0; r--){ // Checking each line
		int linesDeleted = 0;
		if (mainGrid[r][0].toBeDeleted){
			for (int r2 = r; r2>0; r2--){ // Move all rows down one cell
				for (int c = 0; c < COLS; c++){
					mainGrid[r2][c].isFilled=mainGrid[r2-1][c].isFilled;
					mainGrid[r2][c].isActive=mainGrid[r2-1][c].isActive;
					mainGrid[r2][c].toBeDeleted=mainGrid[r2-1][c].toBeDeleted;
					mainGrid[r2][c].red=mainGrid[r2-1][c].red;
					mainGrid[r2][c].green=mainGrid[r2-1][c].green;
					mainGrid[r2][c].blue=mainGrid[r2-1][c].blue;
				}
			}
			r++;
		}
	}
	deleteLines = false;

}