// Made by Louis Boursier
// GitHub: https://github.com/loutouk

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_V_PLAYER 0
#define PLAYER_V_COMPUTER 1
#define COMPUTER_V_COMPUTER 2
#define LOAD 3

#define DEFAULT_GRID_HEIGHT 3
#define DEFAULT_GRID_WIDTH 3
#define DEFAULT_WIN_NUMBER 3

#define MIN_GRID_SIZE 3
#define MAX_GRID_SIZE 40

#define CELL_EMPTY 0
#define CELL_PAWN_A 1
#define CELL_PAWN_B 2

void mainMenu();

struct DoubleLinkedNode {
	struct DoubleLinkedNode* right;
	struct DoubleLinkedNode* left;
	struct Content* content;
};

struct Position {
	int x;
	int y;
};

struct Content {
	struct Position* position;
	struct Player* player; // We don't really need the player because the game is played turn by turn, but it will make the code easier to implement and read
};

struct Player {
	int id;
};

void cleanStd(){
	fflush(stdout);
	fflush(stdin);
}

// Inserts a new node in a LIFO stack way
void pushNodeTop(struct DoubleLinkedNode **node, struct Content *newValue){
	struct DoubleLinkedNode *newNode = malloc(sizeof(struct DoubleLinkedNode));
	newNode->content = newValue;
	newNode->right = NULL;
	newNode->left = NULL;
	if(*node != NULL){
		newNode->right = *node; // We connect the new element to the old one
		(*node)->left = newNode; // And we connect the old one to the new one
	}
	*node = newNode; // The pointer now points to the last element
}

// Inserts a new node in a FIFO stack way
void pushNodeBottom(struct DoubleLinkedNode **node, struct Content *newValue){
	struct DoubleLinkedNode *newNode = malloc(sizeof(struct DoubleLinkedNode));
	newNode->content = newValue;
	newNode->right = NULL;
	newNode->left = NULL;
	if(*node != NULL){
		newNode->left = *node; // We connect the new element to the old one
		(*node)->right = newNode; // And we connect the old one to the new one
	}
	*node = newNode; // The pointer now points to the last element
}

// Moves the pointer of the head of the stack to the previous element without removing any node
void previousNode(struct DoubleLinkedNode **node){
	if(*node != NULL && (*node)->right != NULL){
		*node = (*node)->right;
	}
}

// Moves the pointer of the head of the stack to the next element without removing any node
void nextNode(struct DoubleLinkedNode **node){
	if(*node != NULL && (*node)->left != NULL){
		*node = (*node)->left;
	}
}

// Remove and return the first node in a LIFO stack way
struct DoubleLinkedNode* pollNode(struct DoubleLinkedNode **node){
	if(*node != NULL){
		struct DoubleLinkedNode* polledNode = malloc(sizeof(struct DoubleLinkedNode));
		// Making a copy of the polled node to return it
		polledNode->right = (*node)->right;
		polledNode->left = (*node)->left;
		polledNode->content = (*node)->content; 
		if((*node)->right != NULL){
			// We remove the first element and move the top of the queue on the right
			*node = (*node)->right;
			(*node)->left = NULL;
		}else{
			*node = NULL;
		}
		return polledNode;
	}else{
		return NULL;
	}
}

void displayAll(struct DoubleLinkedNode *node){
	struct DoubleLinkedNode *browser = node;
	while(browser != NULL){
		printf("Move by %d on %d %d\n", browser->content->player->id, browser->content->position->x,  browser->content->position->y);
		browser = browser->right;
	}
}

int saveGame(struct DoubleLinkedNode* moves, int height, int width, int consecutivePawns){
	FILE *file = fopen("save.txt", "w"); // Creates or erase existing file for writing only
	if (file == NULL){
	    return 0; // Failure
	}
	while(moves != NULL){  // Saves line by line as: posX,posY,playerId
		fprintf(file, "%d,%d,%d\n", moves->content->position->x, moves->content->position->y, moves->content->player->id);
		moves = moves->right;
	}
	fclose(file);

	FILE *fileB = fopen("parameters.txt", "w"); // Creates or erase existing file for writing only
	if (fileB == NULL){
	    return 0; // Failure
	}
	fprintf(fileB, "%d,%d,%d\n", height, width, consecutivePawns); // Saves the grid size and the consecutive pawns nb for winning
	fclose(fileB);
	return 1; // Success
}

struct DoubleLinkedNode* loadGame(){
	struct DoubleLinkedNode* newNode = NULL;
	struct Player* playerA = malloc(sizeof(struct Player));
	struct Player* playerB = malloc(sizeof(struct Player));
	playerA->id = CELL_PAWN_A;
	playerB->id = CELL_PAWN_B;
	FILE * file;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    file = fopen("save.txt", "r"); // Read only
    if (file == NULL){
    	return NULL;
    }
    while ((read = getline(&line, &len, file)) != -1) {
        //printf("%s", line);
		char delim[] = ",";
		char *ptr = strtok(line, delim);
		struct Content* content = NULL;
		struct Position* position = NULL;
		for(int i=0 ; i<3 ; i++){ // We have 3 data on each line 
			int playerId;
			switch(i){
				case 0:
					content = malloc(sizeof(struct Content));
					position = malloc(sizeof(struct Position));
					position->x = atoi(ptr);
					break;
				case 1:
					position->y = atoi(ptr);
					break;
				case 2:
					playerId = atoi(ptr);
					if(playerId == CELL_PAWN_A){
						content->player = playerA;
					}else if(playerId == CELL_PAWN_B){
						content->player = playerB;
					}
					content->position = position;
					break;
			}
			ptr = strtok(NULL, delim); // Moves to next char*
		}
		pushNodeBottom(&newNode, content);
    }
    fclose(file);
    if (line){
        free(line);
    }
    return newNode;
}

_Bool isMovePossible(int height, int width, char grid[height][width], int row, int col){
	return ! (row<0 || row>=height || col<0 || col>=width || grid[row][col]!=CELL_EMPTY);
}

int getWinner(int height, int width, int winNumber, char grid[height][width]){
	
	int lastPawnPlayer = -1; // Stores the id of the current player pawns in sequence
	int consecutivePawns = 0; // Stores the number of pawns in sequence
	int totalPawnsToFinish = height * width; // The max number of pawns that can be played to detect a draw
	int pawnsCount = 0; // The counter of pawns played to detect a draw

	for(int i=0 ; i<height ; i++){ // Check the horizontal wins
		consecutivePawns = 0; // reset to 0 on each new lines
		lastPawnPlayer = -1;
		for(int j=0 ; j<width ; j++){
			int currentPlayer = grid[i][j];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}
			if(currentPlayer != CELL_EMPTY && ++pawnsCount == totalPawnsToFinish){
				return 0; // Draw
			}
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}

	for(int i=0 ; i<width ; i++){ // Check the vertical wins
		consecutivePawns = 0;
		lastPawnPlayer = -1;
		for(int j=0 ; j<height ; j++){
			int currentPlayer = grid[j][i];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}
	
	for(int i=0 ; i<height ; i++){ // Check the crossed wins bottom left to top right, upper part
		consecutivePawns = 0;
		lastPawnPlayer = -1;
		for(int j=i ; j>=0 ; j--){
			int currentPlayer = grid[j][i-j];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}
	
	for(int i=width-1 ; i>=0 ; i--){ // Check the crossed wins bottom left top right, lower part
		consecutivePawns = 0;
		lastPawnPlayer = -1;
		for(int j=0 ; j<height && j+width-1-i<width; j++){
			int currentPlayer = grid[height-1-j][j+width-1-i];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}

	for(int i=0 ; i<width ; i++){ // Check the crossed wins top left to bottom right, upper part
		consecutivePawns = 0;
		lastPawnPlayer = -1;
		for(int j=0 ; j<height && j<=i ; j++){
			int currentPlayer = grid[j][width-i+j-1];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}
	
	for(int i=0 ; i<height-1  ; i++){ // Check the crossed wins top left to bottom right, lower part
		consecutivePawns = 0;
		lastPawnPlayer = -1;
		for(int j=0 ; j<width && j<=i ; j++){
			int currentPlayer = grid[height-i+j-1][j];
			if(currentPlayer != lastPawnPlayer){
				lastPawnPlayer = currentPlayer;
				consecutivePawns = 0;
			}	
			if(currentPlayer != CELL_EMPTY && ++consecutivePawns == winNumber){
				return currentPlayer;
			}
		}
	}
	return -1; // No winner and no draw
}

void displayGrid(int height, int width, char grid[height][width]){
	for(int i=0 ; i<height ; i++){
		if(i==0){
			printf("\n\t");
			for(int j=0 ; j<width ; j++){
				printf("%d ", j%10); // prints with mod 10 to not have printing offset with number > 10
			}
			printf("\n\n");
		}
		for(int j=0 ; j<width ; j++){
			if(j==0){
				printf("%d\t", i);
			}
			printf("%d ", grid[i][j]);
		}
		printf("\n");
	}
}

int minMax(int consecutivePawnsForWin, int height, int width, char grid[height][width], int depth, _Bool isMax){
	int winnerId = getWinner(height, width, consecutivePawnsForWin, grid);
	int moveValue = 0;
	if(depth == 0 || winnerId != -1){ // If we reach the max depth search or a final game state
		if(winnerId == 0){
			return - depth; // Draw
		}else if(winnerId == CELL_PAWN_B){
			return (-100 - depth); // Opponent's win, we prefer losing later than sooner

		}else{
			return (100 + depth); // Current player's win (AI), we prefer wining sooner than later
			
		}
	}
	char newGrid[height][width]; // Clones the existing grid to a new one
	for(int line=0 ; line<height ; line++){ 
		for(int col=0 ; col<width ; col++){
			newGrid[line][col] = grid[line][col];
		}
	}
	if(isMax){ // Max
		moveValue = -200; // Ensures that a move will be selected
		for(int line=0 ; line<height ; line++){
			for(int col=0 ; col<width ; col++){
				if(isMovePossible(height, width, newGrid, line, col)){ // Tests all possible moves
					newGrid[line][col] = CELL_PAWN_A; // Player's turn
					int tmp = minMax(consecutivePawnsForWin, height, width, newGrid, depth-1, 0);
					if(tmp>moveValue){ // Selects the max value because we want to play the best move
						moveValue = tmp;
					}
					newGrid[line][col] = CELL_EMPTY; // Undo the move
				}
			}
		}
		return moveValue;
	}else{ // Min
		moveValue = 200; // Ensures that a move will be selected
		for(int line=0 ; line<height ; line++){
			for(int col=0 ; col<width ; col++){
				if(isMovePossible(height, width, newGrid, line, col)){ // Tests all possible moves
					newGrid[line][col] = CELL_PAWN_B; // Opponent's turn
					int tmp = minMax(consecutivePawnsForWin, height, width, newGrid, depth-1, 1);
					if(tmp<moveValue){ // Selects the min value because the opponent will play the worst move for us
						moveValue = tmp;
					}
					newGrid[line][col] = CELL_EMPTY; // Undo the move
				}
			}
		}
		return moveValue;
	}
	return 0; // Never reached but here for the int return of the function
}

void play(int mode){
	// Creates game data structures
	int aiDepthMinMax;
	int height = DEFAULT_GRID_HEIGHT;
	int width = DEFAULT_GRID_WIDTH;
	int consecutivePawnsForWin = DEFAULT_WIN_NUMBER;
	int initStateRedone = 0; // For the redo operation, see further down explanations
	// This will store all the moves done by the players in a queue
	struct DoubleLinkedNode* moves = NULL;
	struct Player* playerA = malloc(sizeof(struct Player));
	struct Player* playerB = malloc(sizeof(struct Player));
	playerA->id = CELL_PAWN_A;
	playerB->id = CELL_PAWN_B;
	_Bool currentPlayer = 0;
	int winner = -1;
	int row, col;
	char input[2];

	if(mode == LOAD){
		initStateRedone = 1; // When we load the file, we start with the empty board and the moves in the memory
		moves = loadGame();
		if(moves == NULL){
			printf("%s\n", "Error while loading the game. Is the file 'save.txt' present in the directory?");
		}else{
			FILE * file;
		    char * line = NULL;
		    size_t len = 0;
		    ssize_t read;
		    file = fopen("parameters.txt", "r"); // Read only
		    if (file == NULL){
		    	printf("%s\n", "Error while loading the parameters file 'parameters.txt'");
		    	exit(1);
		    }
		    while ((read = getline(&line, &len, file)) != -1) {
				char delim[] = ",";
				char *ptr = strtok(line, delim);
				for(int i=0 ; i<3 ; i++){ // We have 3 data on each line 
					switch(i){
						case 0:
							height = atoi(ptr);
							break;
						case 1:
							width = atoi(ptr);
							break;
						case 2:
							consecutivePawnsForWin = atoi(ptr);
							break;
					}
					ptr = strtok(NULL, delim); // Moves to next char*
				}
		    }
		    fclose(file);
			printf("\n%s\n", "Game loaded. Use undo and redo to browse it.");
			printf("%s\n", "You can also play and override it.");
		}
	}else{ // Ask for grid size of winning conditions
		do{
			printf("Select a grid width between %d and %d...", MIN_GRID_SIZE, MAX_GRID_SIZE);
			cleanStd();
		}while(scanf("%d", &width) != 1 || width < MIN_GRID_SIZE || width > MAX_GRID_SIZE);
		do{
			printf("Select a grid height between %d and %d...", MIN_GRID_SIZE, MAX_GRID_SIZE);
			cleanStd();
		}while(scanf("%d", &height) != 1 || height < MIN_GRID_SIZE || height > MAX_GRID_SIZE);
		do{
			printf("Select the number of consecutive pawns to win between %d and %d...", MIN_GRID_SIZE,  width<height?height:width);
			cleanStd();
		}while(scanf("%d", &consecutivePawnsForWin) != 1 || consecutivePawnsForWin < MIN_GRID_SIZE || consecutivePawnsForWin > (width<height?height:width));
	}

	// Use char for less memory usage because we will instantiate lots of grids for the AI
	char grid[height][width];

	// Set the AI depth search according to the size of the grid so that the player does not wait too long
	int cellNumber = height * width;
	if(cellNumber <= 9){
		aiDepthMinMax = 8;
	}
	else if(cellNumber <= 16){
		aiDepthMinMax = 5;
	}
	else if(cellNumber <= 25){
		aiDepthMinMax = 4;
	}
	else if(cellNumber <= 36){
		aiDepthMinMax = 3;
	}
	else if(cellNumber <= 100){
		aiDepthMinMax = 2;
	}
	else{
		aiDepthMinMax = 1;
	}

	// Init the grid with empty cells
	for(int i=0 ; i<height ; i++){
		for(int j=0 ; j<width ; j++){
			grid[i][j] = CELL_EMPTY;
		}
	}

	displayGrid(height, width, grid);

	do{
		_Bool play = 0;

		if(mode == PLAYER_V_COMPUTER && currentPlayer == 0){ // Computer's turn

			char newGrid[height][width];
			int maxValue = -1000;
			// Clone the existing grid to a new one
			for(int i=0 ; i<height ; i++){
				for(int j=0 ; j<width ; j++){
					newGrid[i][j] = grid[i][j];
				}
			}

			for(int i=0 ; i<height ; i++){
				for(int j=0 ; j<width ; j++){
					if(isMovePossible(height, width, newGrid, i, j)){
						newGrid[i][j] = CELL_PAWN_A;
						int tmp = minMax(consecutivePawnsForWin, height, width, newGrid, aiDepthMinMax, 0);
						if(tmp>maxValue){
							maxValue = tmp;
							row = i;
							col = j;
						}
						newGrid[i][j] = CELL_EMPTY; // Undo the move
					}
				}
			}

		}else{
			do{
				printf("%s\n", "\nPress P to play - Z to undo - Y to redo - Q to quit - S to save");
				cleanStd();
				fgets(input, 2, stdin);
				cleanStd();
				switch(input[0]){
					case 'p':
					case 'P':
						play = 1;
						break;
					case 'z':
					case 'Z':
						printf("%s\n", "Undo...");
						if(moves != NULL){
							grid[moves->content->position->y][moves->content->position->x] = CELL_EMPTY;
							struct DoubleLinkedNode* tmp = moves;
							previousNode(&moves);
							if(tmp == moves){ // Identifies if we just undo the first move, it means that we did not go further back in the doubly linked list
								initStateRedone = 1;
							}else{
								initStateRedone = 0; // For the redo operation
							}
						}
						displayGrid(height, width, grid);
						break;
					case 'y':
					case 'Y':
						printf("%s\n", "Redo...");
						if(moves != NULL){
							if(initStateRedone != 1){
								nextNode(&moves);
							}else{
								initStateRedone = 0; // At the init state, we did not go to the previous node, so we should not do nextNode() for the first move
							}
							grid[moves->content->position->y][moves->content->position->x] = moves->content->player->id;
						}
						displayGrid(height, width, grid);
						break;
					case 'q':
					case 'Q':
						mainMenu();
						break;
					case 's':
					case 'S':
						printf("%s\n", "Saving...");
						if(!saveGame(moves, height, width, consecutivePawnsForWin)){
							printf("%s\n", "Error while saving.");
						}else{
							printf("%s\n", "Saving successful.");
						}
						break;
				}
			}while(!play);
			
			do{
				cleanStd();
				printf("\nPlayer %d turn.\n", currentPlayer);
				printf("%s", "Select row...");
				cleanStd();
				scanf("%d", &row);
				cleanStd();
				printf("%s", "Select column...");
				cleanStd();
				scanf("%d", &col);
			}while(!isMovePossible(height, width, grid, row, col));
		}
		

		// Place the pawn
		grid[row][col] = currentPlayer == 0 ? CELL_PAWN_A : CELL_PAWN_B;

		// Save the move in our queue
		struct Content* content = malloc(sizeof(struct Content));
		struct Position* position = malloc(sizeof(struct Position));
		position->x = col;
		position->y = row;
		content->position = position;
		content->player = currentPlayer == 0 ? playerA : playerB;
		pushNodeTop(&moves, content);
		currentPlayer = ! currentPlayer; // Changes player's turn
		displayGrid(height, width, grid);
		winner = getWinner(height, width, consecutivePawnsForWin, grid); // Returns the winner's id or -1 if there is no winner
	}while(winner == -1);
	if(winner == 0){
		printf("\n%s\n", "It's a draw!");
	}else{
		printf("\nPlayer %d win!\n\n", winner);
	}

}

void mainMenu(){

	_Bool validInput = 0;
	int mode;

	do{
		printf("%s\n", "\n\n##########################");
		printf("%s\n\n", "###### ~TIC TAC TOE~ #####");
		printf("%s\t%d\n", "PLAYER VS PLAYER", PLAYER_V_PLAYER);
		printf("%s\t%d\n", "COMPUTER VS PLAYER", PLAYER_V_COMPUTER);
		printf("%s\t%d\n", "COMPUTER VS COMPUTER", COMPUTER_V_COMPUTER);
		printf("%s\t\t%d\n", "LOAD AND REPLAY", LOAD);
		printf("\n%s\n\n", "##########################");
		printf("%s", "Choose a mode... ");
		cleanStd();
		scanf("%d", &mode);

    	switch(mode){
    		case PLAYER_V_PLAYER:
    			play(PLAYER_V_PLAYER);
    			break;
    		case LOAD:
    			play(LOAD);
    			break;
    		case PLAYER_V_COMPUTER:
    			play(PLAYER_V_COMPUTER);
    			break;
    		case COMPUTER_V_COMPUTER:
    			printf("%s\n", "Not implemented yet.");
    		default:
    			validInput = 0;
    			break;
    	}
	
	}while(!validInput);
}

int main(int argc, char const *argv[]){
	mainMenu();
	return 0;
}