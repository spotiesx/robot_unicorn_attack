#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH		640 	// screen width
#define SCREEN_HEIGHT		480		// screen height
#define JUMP_HEIGHT			80		// jump height
#define JUMP_MAX            40		// height difference
#define OBSTACLES_NUMBER	5		// number of obstacles
#define PLATFORMS_NUMBER    20		// number of platforms
#define STAR_NUMBER         4		// number of stars
#define MAX_LIFES           3   	// max number of lifes
#define MAX_DASH_TIME       100 	// max dash time
#define BASIC_UNICORN_SPEED 0.2		// starting unicorn speed
#define BASIC_STAR_POINTS   100 	// starting star points
#define BASIC_FAIRY_POINTS  10  	// starting fairy points
#define PLATFORM_HEIGHT     64  	// platform height
#define MIN_PLATFORM_WIDTH  1280	// minimum platform width
#define MAX_PLATFORM_WIDTH  2560	// maximum platform width
#define MIN_OBSTACLE_WIDTH	100		// minimum obstacle width
#define MAX_OBSTACLE_HEIGHT 50		// maximum obstacle height
#define MIN_OBSTACLE_HEIGHT 20		// minimum obstacle height
#define FAIRY_MOVE			10		// pixels that fairy can move in each side

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
};

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

// randomize a number between min and max
int random(int min, int max)
{
    int tmp;
    if (max>=min)
        max-= min;
    else
    {
        tmp= min - max;
        min= max;
        max= tmp;
    }
    return max ? (rand() % max + min) : min;
}

// checking collision of obstacles and platforms
int CheckCollision( SDL_Surface* a, int positionAX, int positionAY, SDL_Surface* b, int positionBX, int positionBY){
	int option = 0;

    //The sides of the rectangles
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    //Calculate the sides of rect A
    leftA = positionAX;
    rightA = positionAX + a->w;
    topA = positionAY;
    bottomA = positionAY + a->h;

    //Calculate the sides of rect B
    leftB = positionBX;
    rightB = positionBX + b->w;
    topB = positionBY;
    bottomB = positionBY + b->h;

	if(bottomA >= topB && rightA < leftB){
		option = 4;
	}

	if(leftB > rightA && bottomA >= bottomB){
		option = 5;
	}

	// unicorn on the obstacle
    if( rightA >= leftB && leftA <= rightB && bottomA == topB ){
		option = 1;
	}

	// unicorn stops on the left side of the obstacle
	if( rightA == leftB && bottomA > topB ){
		option = 2;
	}

	// unicorn fall of the platform or obstacle
	if( leftA > rightB ){
		option = 3;
	}

	return option;
}

// check collision of faires and stars
int CheckCollision2( SDL_Surface* a, int positionAX, int positionAY, SDL_Surface* b, int positionBX, int positionBY){
	int option = 0;

    //The sides of the rectangles
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    //Calculate the sides of rect A
    leftA = positionAX;
    rightA = positionAX + a->w;
    topA = positionAY;
    bottomA = positionAY + a->h;

    //Calculate the sides of rect B
    leftB = positionBX;
    rightB = positionBX + b->w;
    topB = positionBY;
    bottomB = positionBY + b->h;

	if( (rightA == leftB && topB < bottomA) || (topB == bottomA && rightA > rightB) ){
		option = 1;
	}

	if( leftA >= rightB && topB > bottomA ){
		option = 2;
	}

	return option;
}

void Jump(int* isDashing, int* isJumping, int* jumpTime, bool* isFalling, SDL_Surface* unicorn, SDL_Surface* platform, int* platformY, int hiddenPlatforms, int* jumpHeight, double* distance, 
			bool controlMode, double unicornSpeed, int* jumpingControl, int unicornPositionY, int* basicPlatformY, int* distanceY, int* jumpMode){
		if(jumpMode != 0){
			if( *isJumping != 0){
				(*jumpTime)++;

				// lifting
				if( (*isFalling == false) && (platformY[hiddenPlatforms] + *distanceY < basicPlatformY[hiddenPlatforms] + *jumpHeight ) ){
					if( *jumpTime % 5 == 0){
						(*distanceY)++;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}

				// highest position
				else if( (*isFalling == false) && (platformY[hiddenPlatforms] + *distanceY >= basicPlatformY[hiddenPlatforms] + *jumpHeight) ){
					if( *jumpTime % 10 == 0){
						*isFalling = true;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}
				
				// falling
				else if( (*isFalling == true) && (platformY[hiddenPlatforms] + *distanceY > basicPlatformY[hiddenPlatforms]) ){
					if( *jumpTime % 5 == 0 ){
						(*distanceY)--;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}

				// landing
				else if( (platformY[hiddenPlatforms] + *distanceY == basicPlatformY[hiddenPlatforms]) && (*isFalling == true) ){
					*isDashing = 0;
					*isJumping = 0;
					*isFalling = false;
					*jumpHeight = JUMP_HEIGHT;
					*jumpTime = 0;
					*jumpingControl = 0;
				}
			}
		}
		else{
			if( *isJumping != 0){
				(*jumpTime)++;

				// lifting
				if( (*isFalling == false) && (platformY[hiddenPlatforms] + *distanceY + *jumpHeight > unicornPositionY ) ){
					if( *jumpTime % 5 == 0){
						unicornPositionY++;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}

				// highest position
				else if( (*isFalling == false) && (platformY[hiddenPlatforms] + *distanceY + *jumpHeight <= unicornPositionY) ){
					if( *jumpTime % 10 == 0){
						*isFalling = true;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}
				
				// falling
				else if( (*isFalling == true) && (platformY[hiddenPlatforms] + *distanceY + *jumpHeight < unicornPositionY) ){
					if( *jumpTime % 5 == 0 ){
						unicornPositionY--;
						if(!controlMode){
							*distance += unicornSpeed;
						}
					}
				}

				// landing
				else if( (platformY[hiddenPlatforms] + *distanceY == unicornPositionY) && (*isFalling == true) ){
					*isDashing = 0;
					*isJumping = 0;
					*isFalling = false;
					*jumpHeight = JUMP_HEIGHT;
					*jumpTime = 0;
					*jumpingControl = 0;
				}
			}
		}
}



// clearing whole map and randomizing
void ClearGame( double* worldTime, double* unicornSpeed, double* distance, int* distanceY, bool* controlMode, bool* isFalling, int* isJumping, int* isDashing, int* jumpTime, int* jumpHeight,
				double* dashTime, int* hiddenPlatforms, int* hiddenObstacles, int* hiddenStars, int* jumpingControl, SDL_Surface** platform, SDL_Surface** obstacle, SDL_Surface** star,
				int* platformX, int* platformY, int* obstacleX, int* obstacleY, int* starX, int* starY, int* basicPlatformY, int* k, int* p, int* firstHeight, int* secondHeight, int* actualStarPoints, 
				int* t, int* hiddenFairys, SDL_Surface** fairy, int* fairyX, int* fairyY, int* maxFairyX, int* minFairyX, int* actualFairyPoints, 
				SDL_Surface* screen, SDL_Event event, SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* scrtex){
					
	*k = 0;
	*p = 0;
	*t = 0;
	*worldTime = 0;
	*distance = 0;
	*distanceY = 0;
	*controlMode = false;
	*isFalling = false;
	*unicornSpeed = BASIC_UNICORN_SPEED;
	*isJumping = 0;
	*isDashing = 0;
	*jumpTime = 0;
	*jumpHeight = JUMP_HEIGHT;
	*dashTime = 0;
	*hiddenPlatforms = 0;
	*hiddenStars = 0;
	*hiddenObstacles = 0;
	*hiddenFairys = 0;
	*jumpingControl = 0;
	*firstHeight = 0;
	*secondHeight = 0;
	*actualStarPoints = BASIC_STAR_POINTS;
	*actualFairyPoints = BASIC_FAIRY_POINTS;

	platform[0] = SDL_LoadBMP("./base.bmp");
	platform[0]->w = SCREEN_WIDTH;
	platform[0]->h = PLATFORM_HEIGHT;
	platformX[0] = 0;
	platformY[0] = SCREEN_HEIGHT/2;
	basicPlatformY[0] = platformY[0];

	for (int i = 1; i < PLATFORMS_NUMBER; i++) {
		platform[i] = SDL_LoadBMP("./base.bmp");
		if(platform[i] == NULL) {
			printf("SDL_LoadBMP(base.bmp) error: %s\n", SDL_GetError());
			SDL_FreeSurface(screen);
			SDL_DestroyTexture(scrtex);
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
		};
		basicPlatformY[i] = platformY[0];
		platform[i]->h = PLATFORM_HEIGHT;
		platform[i]->w = random(MIN_PLATFORM_WIDTH, MAX_PLATFORM_WIDTH);
		
		if( i == 3 ){
			platformX[i] = platformX[i-1] + platform[i-1]->w - 100;
			platformY[i] = platformY[i-1] - 50 - PLATFORM_HEIGHT;
		}
		else{
			platformX[i] = random(platformX[i-1] + platform[i-1]->w + 100, platformX[i-1] + platform[i-1]->w + 200);
			platformY[i] = random(platformY[i-1] - 50, platformY[i-1] + 20);
		}

		if( random(1, 2) == 1 && *p <= OBSTACLES_NUMBER && i != 3){
			obstacle[*p] = SDL_LoadBMP("./base.bmp");
			if(obstacle[*p] == NULL) {
				printf("SDL_LoadBMP(base.bmp) error: %s\n", SDL_GetError());
				SDL_FreeSurface(screen);
				SDL_DestroyTexture(scrtex);
				SDL_DestroyWindow(window);
				SDL_DestroyRenderer(renderer);
				SDL_Quit();
			};
			obstacle[*p]->h = random(MIN_OBSTACLE_HEIGHT, MAX_OBSTACLE_HEIGHT);
			obstacle[*p]->w = random(MIN_OBSTACLE_WIDTH, platform[i]->w/2);
			obstacleX[*p] = random(platformX[i], platformX[i] + platform[i]->w - obstacle[*p]->w);
			obstacleY[*p] = platformY[i] - obstacle[*p]->h;
			(*p)++;
		}

		if( random(1, 2) == 1 && *k <= STAR_NUMBER ){
			if( i != 3 ){
				star[*k] = SDL_LoadBMP("./star.bmp");
				if(star[*k] == NULL) {
					printf("SDL_LoadBMP(star.bmp) error: %s\n", SDL_GetError());
					SDL_FreeSurface(screen);
					SDL_DestroyTexture(scrtex);
					SDL_DestroyWindow(window);
					SDL_DestroyRenderer(renderer);
					SDL_Quit();
				};
				starX[*k] = random(platformX[i], platformX[i] + platform[i]->w - star[*k]->w);
				starY[*k] = platformY[i] - star[*k]->h - 5;
				for(int j=0; j < *p; j++){
					if(starX[*k] + star[*k]->w*2 >= obstacleX[j] && obstacleX[j]+obstacle[j]->w + star[*k]->w >= starX[*k] ){
						starY[*k] -= obstacle[j]->h;
					}
				}
				(*k)++;
			}
		}
		else{
			fairy[*t] = SDL_LoadBMP("./fairy.bmp");
			if(fairy[*t] == NULL) {
				printf("SDL_LoadBMP(fairy.bmp) error: %s\n", SDL_GetError());
				SDL_FreeSurface(screen);
				SDL_DestroyTexture(scrtex);
				SDL_DestroyWindow(window);
				SDL_DestroyRenderer(renderer);
				SDL_Quit();
			};
			fairyX[*t] = random(platformX[i], platformX[i] + platform[i]->w - fairy[*t]->w);
			fairyY[*t] = platformY[i] - fairy[*t]->h - 5;
			for(int j=0; j < *p; j++){
				if(fairyX[*t] + fairy[*t]->w*2 >= obstacleX[j] && obstacleX[j]+obstacle[j]->w + fairy[*t]->w >= fairyX[*t] ){
					fairyY[*t] -= obstacle[j]->h;
				}
			}
			maxFairyX[*t] = fairyX[*t] + FAIRY_MOVE;
			minFairyX[*t] = fairyX[*t] - FAIRY_MOVE;
			(*t)++;
		}	
	}
}

// void SaveScore(FILE* scoreboard, int* score){
// 	char const* nick = "Spoties";
// 	char temp_score = (char)score;
// 	char* score_line = nick + ' ' + temp_score;
// 	fscanf(scoreboard, "%d\n", score);
// }

// void DrawScore(FILE* scoreboard){
// 	while(getline()){
// 		DrawRectangle(screen, SCREEN_WIDTH/2-80, 20, 80, 20, blue, blue);
// 		sprintf(text, " %.1lf s ", worldTime);
// 		DrawString(screen, SCREEN_WIDTH/2-80, 20, text, charset);
// 	}
// }

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc, jumpTime, isJumping, isDashing, hiddenPlatforms, hiddenObstacles, hiddenStars, jumpHeight, firstHeight, secondHeight, jumpingControl, lifeNumber, game, 
	k, p, t, distanceY, actualStarPoints, starPoints, hiddenFaires, actualFairyPoints, jumpMode, loopCounter;
	double delta, worldTime, dashTime, distance, unicornSpeed, score;
	bool controlMode, isFalling, menuScene, gameScene, scoreboardScene, instructionScene, continueScene, loseScene, isSavingScore;


	SDL_Surface** fairy = (SDL_Surface**)malloc( (PLATFORMS_NUMBER+1) * sizeof(SDL_Surface*) );
	int* fairyX = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );
	int* fairyY = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );
	int* maxFairyX = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );
	int* minFairyX = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );

	SDL_Surface** platform = (SDL_Surface**)malloc( (PLATFORMS_NUMBER+1) * sizeof(SDL_Surface*) );
	int* platformX = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );
	int* platformY = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );
	int* basicPlatformY = (int*)malloc( (PLATFORMS_NUMBER + 1) * sizeof(int) );

	SDL_Surface** obstacle = (SDL_Surface**)malloc( (OBSTACLES_NUMBER+1) * sizeof(SDL_Surface*) );
	int* obstacleX = (int*)malloc( (OBSTACLES_NUMBER + 1) * sizeof(int) );
	int* obstacleY = (int*)malloc( (OBSTACLES_NUMBER + 1) * sizeof(int) );

	SDL_Surface** star = (SDL_Surface**)malloc( (STAR_NUMBER + 1) * sizeof(SDL_Surface*) );
	int* starX = (int*)malloc( (STAR_NUMBER + 1) * sizeof(int) );
	int* starY = (int*)malloc( (STAR_NUMBER + 1) * sizeof(int) );

	int showText = 0;
	char* inputText;
	int inputSize = 0;
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *renderer;

	FILE *scoreboard;
	scoreboard = fopen( "scoreboard.txt", "a+");

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Robot Unicorn Attack");

	SDL_Surface *screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	SDL_Texture *scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// upload cs8x8.bmp
	SDL_Surface *charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	// upload unicorn.bmp
	SDL_Surface *unicorn = SDL_LoadBMP("./unicorn.bmp");
	if(unicorn == NULL) {
		printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload background.bmp
	SDL_Surface *background = SDL_LoadBMP("./background.bmp");
	if(background == NULL) {
		printf("SDL_LoadBMP(background.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload empty_life.bmp
	SDL_Surface *emptyLife = SDL_LoadBMP("./empty_life.bmp");
	if(emptyLife == NULL) {
		printf("SDL_LoadBMP(empty_life.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload life.bmp
	SDL_Surface *life = SDL_LoadBMP("./life.bmp");
	if(life == NULL) {
		printf("SDL_LoadBMP(life.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload menu.bmp
	SDL_Surface *menu = SDL_LoadBMP("./menu.bmp");
	if(menu == NULL) {
		printf("SDL_LoadBMP(menu.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload instruction.bmp
	SDL_Surface *instruction = SDL_LoadBMP("./instruction.bmp");
	if(instruction == NULL) {
		printf("SDL_LoadBMP(instruction.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload lose_scene.bmp
	SDL_Surface *lose_scene = SDL_LoadBMP("./lose_scene.bmp");
	if(lose_scene == NULL) {
		printf("SDL_LoadBMP(lose_scene.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// upload continue.bmp
	SDL_Surface *continue_scene = SDL_LoadBMP("./continue.bmp");
	if(continue_scene == NULL) {
		printf("SDL_LoadBMP(continue.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	// generate all positions
	ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
				&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
				platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
				fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );



	// generate colors
	char text[128];
	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int orange = SDL_MapRGB(screen->format, 0xff, 0x7c, 0x1f);
	int pink = SDL_MapRGB(screen->format, 0xFF, 0xB6, 0xC1);
	t1 = SDL_GetTicks();

	// start values
	showText = 0;
	inputText;
	inputSize = 0;
	quit = 0;
	lifeNumber = MAX_LIFES;
	score = 0;
	starPoints = 0;
	jumpMode = 0;
	loopCounter = 0;

	// standard scenes value
	menuScene = false;
	gameScene = false;
	scoreboardScene = false;
	loseScene = false;
	continueScene = false;
	instructionScene = true;
	isSavingScore = false;

	// unicorn position
	int unicornPositionX = 8;
	int unicornPositionY = platformY[0] - unicorn->h;

	// disable a coursor
	SDL_ShowCursor(SDL_DISABLE);	

	while(!quit) {

		// menu
		if( menuScene ){
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, menu, 0, 0);
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_KEYDOWN:
						if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
						else if(event.key.keysym.sym == SDLK_p){
							gameScene = true;
							menuScene = false;
							lifeNumber = MAX_LIFES;
							score = 0;
						}
						else if(event.key.keysym.sym == SDLK_s){
							scoreboardScene = true;
							menuScene = false;
						}
						break;
					case SDL_KEYUP:
						break;
					case SDL_QUIT:
						quit = 1;
						break;
				};
			};
		}

		// instruction
		else if( instructionScene ){
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, menu, 0, 0);
			DrawSurface(screen, instruction, 0, 0);
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_KEYDOWN:
						if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
						else if(event.key.keysym.sym == SDLK_m){
							instructionScene = false;;
							menuScene = true;
						}
						break;
					case SDL_KEYUP:
						break;
					case SDL_QUIT:
						quit = 1;
						break;
				};
			};
		}

		// continue
		else if( continueScene ){
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, menu, 0, 0);
			DrawSurface(screen, continue_scene, 0, 0);
			sprintf(text, " %.0lf ", score);
			DrawRectangle(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 50, strlen(text)*8, 20, black, red);
			DrawString(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 55, text, charset);
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_KEYDOWN:
						if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
						else if(event.key.keysym.sym == SDLK_c){
							continueScene = false;
							gameScene = true;
							score = 0;
						}
						else if(event.key.keysym.sym == SDLK_m){
							continueScene = false;
							menuScene = true;
							score = 0;
						}						
						break;
					case SDL_KEYUP:
						break;
					case SDL_QUIT:
						quit = 1;
						break;
				};
			};
		}

		// lose
		else if( loseScene ){
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, background, 0, 0);
			if( isSavingScore ){
				while(SDL_PollEvent(&event)) {
					switch(event.type) {
						case SDL_KEYDOWN:
							if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
							else if(event.key.keysym.sym == SDLK_KP_ENTER){
								fprintf(scoreboard, "%s\t", inputText);
								fprintf(scoreboard, "%lf\n", score);
								score = 0;
								isSavingScore = false;
								loseScene = false;
								menuScene = true;
							}
							else{
								sprintf(inputText, "%s", event.text.text);
							}
							break;
						case SDL_KEYUP:
							break;
						case SDL_QUIT:
							quit = 1;
							break;
					};
				};
			}
			else{
				DrawSurface(screen, lose_scene, 0, 0);
				while(SDL_PollEvent(&event)) {
					switch(event.type) {
						case SDL_KEYDOWN:
							if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
							else if(event.key.keysym.sym == SDLK_s){
								menuScene = true;
								loseScene = false;
							}
							else if(event.key.keysym.sym == SDLK_m){
								menuScene = true;
								loseScene = false;
							}
							break;
						case SDL_KEYUP:
							break;
						case SDL_QUIT:
							quit = 1;
							break;
					};
				};
			}
		}

		// scoreboard
		else if( scoreboardScene ){
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, background, 0, 0);

			while(SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_KEYDOWN:
						if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
						else if(event.key.keysym.sym == SDLK_m){
							scoreboardScene = false;
							menuScene = true;
						}
						break;
					case SDL_KEYUP:
						break;
					case SDL_QUIT:
						quit = 1;
						break;
				};
			};
		}

		// game
		else if( gameScene ){
			loopCounter++;
			// calculate a time
			t2 = SDL_GetTicks();
			delta = (t2 - t1) * 0.001;
			t1 = t2;
			worldTime += delta;

			SDL_FillRect(screen, NULL, black);
			DrawSurface(screen, background, 0, 0);

			if(loopCounter%10000==0){
				unicornSpeed += BASIC_UNICORN_SPEED;
				loopCounter = 0;
			}

			// draw lifes
			if( lifeNumber == MAX_LIFES ){
				DrawSurface(screen, life, 10, 20);
				DrawSurface(screen, life, 30, 20);
				DrawSurface(screen, life, 50, 20);
			}
			else if( lifeNumber == MAX_LIFES - 1 ){
				DrawSurface(screen, life, 10, 20);
				DrawSurface(screen, life, 30, 20);
				DrawSurface(screen, emptyLife, 50, 20);
			}
			else if( lifeNumber == MAX_LIFES - 2 ){
				DrawSurface(screen, life, 10, 20);
				DrawSurface(screen, emptyLife, 30, 20);
				DrawSurface(screen, emptyLife, 50, 20);
			}

			// draw a time
			sprintf(text, " %.1lf ", worldTime);
			DrawRectangle(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 20, strlen(text)*8, 20, black, red);
			DrawString(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 25, text, charset);

			// draw a score
			sprintf(text, " %.0lf ", score);
			DrawRectangle(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 50, strlen(text)*8, 20, black, red);
			DrawString(screen, SCREEN_WIDTH/2-(strlen(text)/2)*8, 55, text, charset);

			// dash handling
			if( isDashing == 1 && isJumping == 0 && dashTime < MAX_DASH_TIME ){
				isJumping = 0;
				dashTime++;
				if(dashTime == 1) unicornSpeed += 1;
				if( dashTime == MAX_DASH_TIME ){
					isDashing = 0;
					dashTime = 0;
					unicornSpeed -= 1;
				}
			}
			else if( isDashing == 1 && isJumping == 1 && dashTime < MAX_DASH_TIME ){
				isFalling = false;
				dashTime++;
				if(dashTime == 1) unicornSpeed += 1;
				if( dashTime == MAX_DASH_TIME ){
					isFalling = true;
					unicornSpeed -= 1;
				}
			}
			else if( isDashing == 1 && isJumping == 3 && dashTime < MAX_DASH_TIME ){
				isFalling = false;
				dashTime++;
				if(dashTime == 1) unicornSpeed += 1;
				if( dashTime == MAX_DASH_TIME ){
					firstHeight = secondHeight;
					isJumping = 2;
					jumpingControl = 1;
					isFalling = true;
					unicornSpeed -= 1;
				}
			}
			else if( isDashing == 0 ){
				dashTime = 0;
			}

			// automatic move in second mode
			if( controlMode ){
				score += unicornSpeed;
				distance += unicornSpeed;
			}

			// restart the map
			if( platformX[PLATFORMS_NUMBER-1] + platform[PLATFORMS_NUMBER-1]->w - distance == SCREEN_WIDTH ){
				distance = 0;
				distanceY = 0;
				hiddenFaires = 0;
				hiddenObstacles = 0;
				hiddenPlatforms = 0;
				hiddenStars = 0;
			}

			// handling platforms
			for(int i = hiddenPlatforms; i < PLATFORMS_NUMBER; i++){
				if( i == 3 ){
					if(platformX[i] - distance - unicorn->w <= 0 && hiddenPlatforms == 2 ){
						hiddenPlatforms++;
					}
					DrawSurface(screen, platform[2], platformX[2] - distance, platformY[2] + distanceY);
				}

				if( i <= hiddenPlatforms + 2 ){
					Jump(&isDashing, &isJumping, &jumpTime, &isFalling, unicorn, *platform, platformY, hiddenPlatforms, &jumpHeight, &distance, 
					controlMode, unicornSpeed, &jumpingControl, unicornPositionY, basicPlatformY, &distanceY, &jumpMode);
				}

				// unicorn on platform
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[hiddenPlatforms], platformX[hiddenPlatforms] - distance, platformY[hiddenPlatforms] + distanceY) == 1 ){
					basicPlatformY[hiddenPlatforms] = unicornPositionY + unicorn->h;
				}

				// unicorn stops on left side of platform
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[i], platformX[i] - distance, platformY[i] + distanceY) == 2 ){
					if( lifeNumber == 1 ){
						// score += starPoints;
						starPoints = 0;
						loseScene = true;
						gameScene = false;
					}
					else{
					ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
								&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
								platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
								fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );
						continueScene = true;
						gameScene = false;
						lifeNumber -= 1;			
					}
				}

				// unicorn falling of platform
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[i], platformX[i] - distance, platformY[i] + distanceY) == 3 ){
					if( isJumping == 0 ){	
						isJumping = 1;
						isFalling = true;
					}
					basicPlatformY[hiddenPlatforms] = 0;
				}

				// unicorn falling of platforms
				if( hiddenPlatforms != 0 ){
					if( platformY[hiddenPlatforms-1] > platformY[hiddenPlatforms] ){
						if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[hiddenPlatforms-1], platformX[hiddenPlatforms-1] - distance, platformY[hiddenPlatforms-1] + distanceY) == 3 && 
						CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[hiddenPlatforms], platformX[hiddenPlatforms] - distance, platformY[hiddenPlatforms]) == 5 + distanceY ){
							isJumping = 1;
							isFalling = 1;
							basicPlatformY[hiddenPlatforms] = 0;
						}
					}
					else if( platformY[hiddenPlatforms-1] <= platformY[hiddenPlatforms] ){
						if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[hiddenPlatforms-1], platformX[hiddenPlatforms-1] - distance, platformY[hiddenPlatforms-1] + distanceY) == 3 && 
						CheckCollision(unicorn, unicornPositionX, unicornPositionY, platform[hiddenPlatforms], platformX[hiddenPlatforms] - distance, platformY[hiddenPlatforms] + distanceY) == 4 ){
							isJumping = 1;
							isFalling = 1;
							basicPlatformY[hiddenPlatforms] = 0;
						}
					}
				}

				// draw platform
				DrawSurface(screen, platform[i], platformX[i] - distance, platformY[i] + distanceY);

				if( platformX[i] + platform[i]->w - distance <= 0 ){
					hiddenPlatforms++;
				}
			}
			
			// handling stars
			for(int i = hiddenStars; i < k; i++){

				// drawing star
				DrawSurface(screen, star[i], starX[i] - distance, starY[i] + distanceY);

				// unicorn go into star
				if( CheckCollision2(unicorn, unicornPositionX, unicornPositionY, star[hiddenStars], starX[hiddenStars] - distance, starY[hiddenStars] + distanceY) == 1 && isDashing != 0 ){
					if(hiddenStars != k-1) hiddenStars++;
					score += actualStarPoints;
					actualStarPoints += BASIC_STAR_POINTS;
				}
				else if( CheckCollision2(unicorn, unicornPositionX, unicornPositionY, star[hiddenStars], starX[hiddenStars] - distance, starY[hiddenStars] + distanceY) == 1 && isDashing == 0 ){
					if( lifeNumber == 1 ){
						starPoints = 0;
						loseScene = true;
						gameScene = false;
					}
					else{
					ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
								&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
								platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
								fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );
						continueScene = true;
						gameScene = false;
						lifeNumber -= 1;			
					}
				}

				// unicorn pass star
				if( CheckCollision2(unicorn, unicornPositionX, unicornPositionY, star[hiddenStars], starX[hiddenStars] - distance, starY[hiddenStars] + distanceY) == 2 ){
					actualStarPoints = BASIC_STAR_POINTS;
				}		

				if( starX[i] + star[i]->w - distance <= 0 ){
					hiddenStars++;
				}
			}

			// handling obstacles
			for(int i = hiddenObstacles; i < p; i++){
				
				// unicorn on obstacle
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, obstacle[hiddenObstacles], obstacleX[hiddenObstacles] - distance, obstacleY[hiddenObstacles] + distanceY) == 1 ){
					basicPlatformY[hiddenPlatforms] = unicornPositionY + unicorn->h + obstacle[hiddenObstacles]->h;
				}

				// unicorn stops on left side of platform
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, obstacle[hiddenObstacles], obstacleX[hiddenObstacles] - distance, obstacleY[hiddenObstacles] + distanceY) == 2 ){
					if( lifeNumber == 1 ){
						starPoints = 0;
						loseScene = true;
						gameScene = false;
					}
					else{
					ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
								&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
								platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
								fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );
						continueScene = true;
						gameScene = false;
						lifeNumber -= 1;			
					}
				}

				// unicorn falling of the obstacle
				if( CheckCollision(unicorn, unicornPositionX, unicornPositionY, obstacle[hiddenObstacles], obstacleX[hiddenObstacles] - distance, obstacleY[hiddenObstacles] + distanceY) == 3 ){
					if( isJumping == 0 ){	
						isJumping = 1;
						isFalling = true;
					}
					basicPlatformY[hiddenPlatforms] = unicornPositionY + unicorn->h;
				}

				// drawing obstacle
				DrawSurface(screen, obstacle[i], obstacleX[i] - distance, obstacleY[i] + distanceY);

				if( obstacleX[i] + obstacle[i]->w - distance <= 0 ){
					hiddenObstacles++;
				}
			}

			// fairy handling
			for(int i = hiddenFaires; i < t; i++){
				
				// draw faires
				DrawSurface(screen, fairy[i], fairyX[i] - distance, fairyY[i] + distanceY);

				// random faires move
				if( random(1,3)==1 && fairyX[i] < maxFairyX[i] ){
					fairyX[i]++;
				}
				if( random(1,3)==1 && fairyX[i] > minFairyX[i] ){
					fairyX[i]--;
				}

				// unicorn in fairy
				if( CheckCollision2(unicorn, unicornPositionX, unicornPositionY, fairy[hiddenFaires], fairyX[hiddenFaires] - distance, fairyY[hiddenFaires] + distanceY) == 1 ){
					if( hiddenFaires != t - 1 ) hiddenFaires++;
					score += actualFairyPoints;
					actualFairyPoints += BASIC_FAIRY_POINTS;
				}

				// unicorn pass fairy
				if( CheckCollision2(unicorn, unicornPositionX, unicornPositionY, fairy[hiddenFaires], fairyX[hiddenFaires] - distance, fairyY[hiddenFaires] + distanceY) == 2 ){
					actualFairyPoints = BASIC_FAIRY_POINTS;
				}		

				if( fairyX[i] + fairy[i]->w - distance <= 0 ){
					hiddenFaires++;
				}
			}

			// drawing unicorn
			DrawSurface(screen, unicorn, unicornPositionX, unicornPositionY);

			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			// handling of events (if there were any)
			while(SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_KEYDOWN:
						if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
						else if(event.key.keysym.sym == SDLK_n){
						ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
									&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
									platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
									fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );
							lifeNumber = MAX_LIFES;
							controlMode = false;
						}
						else if(event.key.keysym.sym == SDLK_m){
						ClearGame( &worldTime, &unicornSpeed, &distance, &distanceY, &controlMode, &isFalling, &isJumping, &isDashing, &jumpTime, &jumpHeight,
									&dashTime, &hiddenPlatforms, &hiddenObstacles, &hiddenStars, &jumpingControl, platform, obstacle, star,
									platformX, platformY, obstacleX, obstacleY, starX, starY, basicPlatformY, &k, &p, &firstHeight, &secondHeight, &actualStarPoints, &t, &hiddenFaires, 
									fairy, fairyX, fairyY, maxFairyX, minFairyX, &actualFairyPoints, screen, event, window, renderer, scrtex );
							menuScene = true;
							gameScene = false;
						}
						else if(event.key.keysym.sym == SDLK_RIGHT){	
							if( !controlMode ){
								distance += unicornSpeed;
							}
						}
						else if(event.key.keysym.sym == SDLK_UP){
							if( controlMode == false ){
								if( isJumping == 0 ){
									isJumping = 1;
								}
							}
						}
						else if(event.key.keysym.sym == SDLK_z){
							if( controlMode == true ){
								if( isJumping == 0 && jumpingControl == 0 ){
									isJumping = 1;
									jumpingControl = 1;
								}
								if( isJumping == 1 && jumpingControl == 1 ){
									if( jumpHeight < JUMP_HEIGHT + JUMP_MAX ){
										jumpHeight += 2;
									}
								}
								if( isJumping == 2 && jumpingControl == 1 ){
									isFalling = false;
									jumpHeight += JUMP_HEIGHT;
									isJumping = 3;
									jumpingControl = 2;
								}
								if( isJumping == 3 && jumpingControl == 2 ){
									isFalling = false;
									if( jumpHeight < firstHeight + JUMP_HEIGHT + JUMP_MAX ){
										jumpHeight += 2;
									}
								}
							}	
						}
						else if(event.key.keysym.sym == SDLK_d){
							controlMode = !controlMode;
						}
						else if(event.key.keysym.sym == SDLK_x){
							if( (controlMode == true) && (isDashing == 0) ){
								isDashing = 1;
							}
						}
						break;
					case SDL_KEYUP:
						if( isJumping == 1 ){
							firstHeight = jumpHeight;
							isJumping = 2;
						}
						if( isJumping == 3 ){
							jumpingControl = 0;
							secondHeight = jumpHeight;
						}
						break;
					case SDL_QUIT:
						quit = 1;
						break;
				};
			};
		};
	};

	// free memory and textures
	fclose(scoreboard);
	free(fairy);
	free(fairyX);
	free(fairyY);
	free(minFairyX);
	free(maxFairyX);
	free(platform);
	free(platformX);
	free(platformY);
	free(basicPlatformY);
	free(obstacle);
	free(obstacleX);
	free(obstacleY);
	free(star);
	free(starX);
	free(starY);
	SDL_FreeSurface(*fairy);
	SDL_FreeSurface(*star);
	SDL_FreeSurface(*platform);
	SDL_FreeSurface(*obstacle);
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
	};