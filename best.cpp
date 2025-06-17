
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;
using namespace sf;

const int M = 25;
const int N = 40;

int grid[M][N] = {0};                   //when grid  = 1 then it colide with wall //2 means trail of the player
int ts = 18; //tile size


//for the scores
int score = 0;
int tilesCapturedInMove = 0; // Tracks tiles captured in current move
int lastMoveTiles = 0; // Tracks tiles from previous move for scoring


//for the power up
int powerUps = 0;          // Number of power-ups available
bool freezeEnemies = false; 
float freezeTime = 0;       


// Scoreboard struct and functions
struct HighScore {
    int score;
    float time;
};

const int MAX_SCORES = 5;
HighScore scoreboard[MAX_SCORES];


// Initialize scoreboard with default values
void initializeScoreboard() {
    for (int i = 0; i < MAX_SCORES; i++) {
        scoreboard[i].score = 0;
        scoreboard[i].time = 0.0f;
    }
}

// Load scores from file
void loadScores(){
    FILE* file = fopen("scores.txt", "r");             //to open the file in read mode 'r'
    if (!file){
        initializeScoreboard();
        return;
    }

    for (int i = 0; i < MAX_SCORES; i++) {
        if (fscanf(file, "%d %f\n", &scoreboard[i].score, &scoreboard[i].time) != 2) {         // fscanf to read the data form the ptr file and to check if it read the exactly two values
                                                                                                       //%d will read the inputs and %f will read the float
            scoreboard[i].score = 0;
            scoreboard[i].time = 0.0f;
        }
    }
    fclose(file);
}

// Save scores to file
void saveScores() {
    FILE* file = fopen("scores.txt", "w");
    if (!file) return;

    for (int i = 0; i < MAX_SCORES; i++) {
        fprintf(file, "%d %f\n", scoreboard[i].score, scoreboard[i].time);
    }
    fclose(file);
}

// Update scoreboard with new score if it qualifies
void updateScoreboard(int newScore, float newTime) {
    loadScores(); // Ensure we have current scores
    
    // Check if new score qualifies for the top 5
    for (int i = 0; i < MAX_SCORES; i++) {
        if (newScore > scoreboard[i].score) {
            // Shift lower scores down
            for (int j = MAX_SCORES - 1; j > i; j--) {
                scoreboard[j] = scoreboard[j-1];
            }
            // Insert new score
            scoreboard[i].score = newScore;
            scoreboard[i].time = newTime;
            saveScores();
            break;
        }
    }
}

// Display the scoreboard
void displayScoreboard(RenderWindow& window, Font& font) {
    loadScores(); // Ensure we have current scores

    // Background rectangle for scoreboard
    RectangleShape background(Vector2f(300, 300));
    background.setFillColor(Color(50, 50, 50, 200));
    background.setPosition(50, 50);
    window.draw(background);

    Text title;
    title.setFont(font);
    title.setString("TOP 5 SCORES");
    title.setCharacterSize(30);
    title.setFillColor(Color::Yellow);
    title.setPosition(100, 60);
    window.draw(title);

    Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(Color::White);

    for (int i = 0; i < MAX_SCORES; i++) {
        char buffer[100];
        sprintf(buffer, "%d. %d pts (%.1f sec)", i+1, scoreboard[i].score, scoreboard[i].time);
        scoreText.setString(buffer);
        scoreText.setPosition(80, 120 + i * 40);
        window.draw(scoreText);
    }

    // Back button
    Text backText;
    backText.setFont(font);
    backText.setString("Press any key to return");
    backText.setCharacterSize(20);
    backText.setFillColor(Color::Green);
    backText.setPosition(100, 320);
    window.draw(backText);
}




struct Enemy
{
    int x, y, dx, dy;                           //position and velocities
    float baseSpeed;  // Store base speed
    float speedMultiplier;  // Current speed multiplier


    bool geometricMode = false;
    float angle = 0;
    int direction = 1;
    int moveTimer = 0;

    // For geometric movement
    float patternTime = 0;
    float patternSpeed = 1;
    int centerX, centerY;
    bool initialized = false;

    Enemy()
    {
        //x = y = 300;
        baseSpeed = 4.0f;  // Base speed value
        speedMultiplier = 1.0f;  // Start with normal speed
        // Random initial direction with base speed
        dx = (baseSpeed - rand() % (int)(baseSpeed * 2)) * speedMultiplier;
        dy = (baseSpeed - rand() % (int)(baseSpeed * 2)) * speedMultiplier;

        x = (rand() % (N - 2) + 1) * ts;
        y = (rand() % (M - 2) + 1) * ts;
        
        if (dx == 0) dx = 1;
        if (dy == 0) dy = 1;
    }

    void circularMovement() {
        if (!initialized) {
            centerX = x;                                   //are the points where they will come
            centerY = y;
            initialized = true;
        }
        angle += 0.05;          //angle is incremented to change the position
        int radius = 25;                         //radius is fixed as 25px
        x = centerX + radius * cos(angle);                                       //parametric equation of the circle
        y = centerY + radius * sin(angle);
    }

    void moveFigure8() {
        if (!initialized) {
            centerX = x;
            centerY = y;
            initialized = true;
        }
        patternTime += 0.05f * patternSpeed;
        x = centerX + 50 * sin(patternTime);
        y = centerY + 30 * sin(2 * patternTime);                   //for the movement like infinity
    }


    void move() {
        if (!geometricMode) {
            x += dx;
            if (grid[y / ts][x / ts] == 1) {
                 dx = -dx; x += dx; 
                }
            y += dy;
            if (grid[y / ts][x / ts] == 1) {
                 dy = -dy; y += dy; 
                }
        } else {
            if (direction % 2 == 0)                                        //for the even numbers of the remnining enemies
                circularMovement();
            else
                moveFigure8();
        }
    }

    void increaseSpeed(float amount)
    {
        speedMultiplier += amount;
        // Update current velocities with new multiplier
        dx = (dx > 0) ? baseSpeed * speedMultiplier : -baseSpeed * speedMultiplier;
        dy = (dy > 0) ? baseSpeed * speedMultiplier : -baseSpeed * speedMultiplier;
    }
};

//to increASE the speed of the enemies

float speedIncreaseTimer = 0.0f;
const float speedIncreaseInterval = 20.0f; // 20 seconds
const float speedIncreaseAmount = 0.18f; // for the a8 percent increase in speed

void drop(int y,int x)
{
  if (grid[y][x]==0) grid[y][x]=-1;
  if (grid[y-1][x]==0) drop(y-1,x);
  if (grid[y+1][x]==0) drop(y+1,x);
  if (grid[y][x-1]==0) drop(y,x-1);
  if (grid[y][x+1]==0) drop(y,x+1);
}





//for the menu

void handleMenu(RenderWindow& window, int& screenState, Text menuItems[], Text levelItems[], Text& backButton, Font& font, Sprite& backgroundSprite) {
    window.clear();

    window.draw(backgroundSprite); 

    Vector2i mouse = Mouse::getPosition(window);

    if (screenState == 0){ // Main Menu
        for (int i = 0; i < 5; i++) {
            if (menuItems[i].getGlobalBounds().contains(mouse.x, mouse.y))
                menuItems[i].setFillColor(Color::Yellow);
            else
                menuItems[i].setFillColor(Color::White);
            window.draw(menuItems[i]);
        }
    }
    else if (screenState == 1) { // Level Menu
        for (int i = 0; i < 4; i++) {
            if (levelItems[i].getGlobalBounds().contains(mouse.x, mouse.y))
                levelItems[i].setFillColor(Color::Red);
            else
                levelItems[i].setFillColor(Color::Yellow);
            window.draw(levelItems[i]);
        }

        if (backButton.getGlobalBounds().contains(mouse.x, mouse.y))
            backButton.setFillColor(Color::Red);
        else
            backButton.setFillColor(Color::Yellow);

        window.draw(backButton);
    }
    else if (screenState == 3) { // Scoreboard
        window.clear();
        window.draw(backgroundSprite);
        displayScoreboard(window, font);
        window.display();
    
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::KeyPressed || 
                (e.type == Event::MouseButtonPressed && e.mouseButton.button == Mouse::Left)) {
                screenState = 0; // Return to main menu
            }
            if (e.type == Event::Closed) {
                window.close();
            }
             continue;
        }
       
    }

    window.display();
}


//for the end menu
void endMenu(RenderWindow& window, int& screenState, Text endItems[], Font& font, Sprite& backgroundSprite, int finalScore){
    window.clear();

    window.draw(backgroundSprite); // background image


    Text scoreDisplay;
    scoreDisplay.setFont(font);
    scoreDisplay.setCharacterSize(24);
    scoreDisplay.setFillColor(Color::Yellow);
    scoreDisplay.setString("Final Score: " + to_string(finalScore));
    scoreDisplay.setPosition(120, 150);
    window.draw(scoreDisplay);

    Vector2i mouse = Mouse::getPosition(window);

    for (int i = 0; i < 3; i++) {
        if (endItems[i].getGlobalBounds().contains(mouse.x, mouse.y))
            endItems[i].setFillColor(Color::Yellow);
        else
            endItems[i].setFillColor(Color::White);

        window.draw(endItems[i]);
    }

    window.display();
}

 //for the reset of the whole game
void resetGame(bool& Game, int& x, int& y, int& dx, int& dy, int& moveCount, 
    int grid[M][N], float& enemyTimer, Clock& gameClock, Enemy a[] , bool switched )
{


    powerUps = 0;
freezeEnemies = false;
freezeTime = 0;


    enemyTimer = 0.0f;
    speedIncreaseTimer = 0.0f;
    Game = true;
    x = y = dx = dy = moveCount = 0;
    score = 0;
    tilesCapturedInMove = 0;
    lastMoveTiles = 0;

    switched = false;            //for the switching of the movement


// Reset grid
for (int i = 0; i < M; i++)
for (int j = 0; j < N; j++)
 grid[i][j] = (i == 0 || j == 0 || i == M - 1 || j == N - 1) ? 1 : 0;


for (int i = 0; i < 25; i++) {
a[i] = Enemy();    //for the reconstruction of the enemies
}

gameClock.restart();
}



//for the continous mode
void addEnemies(int &enemyCount, float &enemyTimer, float &time) {
    enemyTimer += time;
    if (enemyTimer >= 20.0f){  
        enemyCount += 2;
        enemyTimer = 0;  // Reset the timer after adding enemies
    }
}


void setLevel(int levelChoice, int& enemyCount, float& delay)       
{
    if (levelChoice == 0) { 
        enemyCount = 2;
        delay = 0.1;
    }
    else if(levelChoice == 1) { 
        enemyCount = 4;
        delay = 0.1;
    }
    else if(levelChoice == 2){ 
        enemyCount = 6;
        delay = 0.1;
    }
    else if(levelChoice == 3){ 
        enemyCount = 2; 
        delay = 0.1;
    }
}



int rewardCounter = 0;
int threshold = 10;   

void updateScore(){
    // First check if we qualify for any bonus
    if (lastMoveTiles > threshold){
        rewardCounter++; // Increment whenever we get a bonus
        
        // Apply the appropriate multiplier based on rewardCounter
        if(rewardCounter >= 5){
            score += lastMoveTiles * 4; // ×4 points after 5 bonuses
        } 
        else if(rewardCounter >= 3){
            score += lastMoveTiles * 2; // ×2 points after 3 bonuses
        }
        else{
            score += lastMoveTiles * 2; // ×2 points for first 2 bonuses
        }
        
        // Update threshold after 3 bonuses
        if (rewardCounter == 3){
            threshold = 5;
        }
    } 
    else {
        // No bonus, just add normal points
        score += lastMoveTiles;
    }
    
    lastMoveTiles = 0; // Reset for next move
}




void checkPowerUps() {
    if (score >= 50 && powerUps == 0) powerUps++;
    else if (score >= 70 && powerUps == 1){
        powerUps++;
    }
    else if (score >= 100 && powerUps == 2){
        powerUps++;
    }
    else if (score >= 130 && powerUps == 3){
        powerUps++;
    }
    else if (score >= 130 + (powerUps-3)*30 && powerUps >= 3){
        powerUps++;
    }
}



float  enemyTimer = 0; 
bool isContinuousMode = false;


bool keyPressed = false; // to detect manual key press
int moveCount = 0;


Clock gameClock; // This clock tracks game time


Clock moveClock;           //for changing the movement
bool switched = false;


struct Player {
    int score = 0;
    int tilesCapturedThisMove = 0;
    int bonusCounter = 0;
    int multiplier = 1;
    int powerUps = 0; 
    bool usedPowerUp = false; 
};


float enemyFreezeTimer = 0;
bool enemiesFrozen = false;
const float freezeDuration = 1.0f; 



//for the multiplayer
void multiplayerMode(RenderWindow& window , Font&  font)
{
    srand(time(0));

 

    
   

//to set the size of the background image 

Texture multiplayerTexture;
multiplayerTexture.loadFromFile("images/multiplayer.png");
// Create the sprite and scale it to window size
Sprite multiplayerSprite(multiplayerTexture);

// Get window and texture dimensions
Vector2u windowSize = window.getSize();
Vector2u textureSize = multiplayerTexture.getSize();

// Calculate scaling factors (adjust height to exclude top tile border)
float scaleX = (float)windowSize.x / textureSize.x;
float scaleY = (float)(windowSize.y - ts) / textureSize.y;  // Subtract tile height (ts) from window height

// Apply scaling and position the sprite below the top border
multiplayerSprite.setScale(scaleX, scaleY);
multiplayerSprite.setPosition(0, ts);  // Start drawing after the top tile row


    
    Player player1, player2;
    int tilesThreshold = 10; 

    // Score display texts
    Text scoreText1, scoreText2;
    scoreText1.setFont(font);
    scoreText1.setCharacterSize(16);
    scoreText1.setFillColor(Color::Red);
    scoreText1.setPosition(20, 54); 
    
    scoreText2.setFont(font);
    scoreText2.setCharacterSize(16);
    scoreText2.setFillColor(Color::Blue);
    scoreText2.setPosition((N*ts - 70) - 120, 54); 


    Text timeText;  
     
    
timeText.setFont(font);                                                //for adjusting the timer
timeText.setCharacterSize(16);
timeText.setFillColor(Color::Red);
timeText.setPosition(20, 35); 

    Texture t1,t2,t3,t4;
    t1.loadFromFile("images/tiles.png");
    t2.loadFromFile("images/gameover.png");
    t3.loadFromFile("images/enemy.png");
    t4.loadFromFile("images/redTile.png");

    Sprite sTile1(t1), sTile2(t4),  sGameover(t2), sEnemy(t3), sPlayer1Tile(t1), sPlayer2Tile(t4);
    sGameover.setPosition(100,100);
    sEnemy.setOrigin(20,20);


     // Set up tile sprites for each player
     sPlayer1Tile.setTextureRect(IntRect(54,0,ts,ts));
     sPlayer1Tile.setColor(Color::Red);
     sPlayer2Tile.setTextureRect(IntRect(54,0,ts,ts));
     sPlayer2Tile.setColor(Color::Blue);

    int enemyCount = 3;
    Enemy a[10];

    bool Game = true;

    int count = 0;

    for (int i=0 ; i<4 ;i++){

        count++;
    }
 
    int x1=10, y1=0, dx1=0, dy1=0;
    bool p1Moving = false;
    
    int x2=N-11, y2=0, dx2=0, dy2=0;
    bool p2Moving = false;
    
    float timer=0, delay=0.07;
    Clock clock;

    // Reset grid
    for (int i=0;i<M;i++)
        for (int j=0;j<N;j++)
            if (i==0 || j==0 || i==M-1 || j==N-1) grid[i][j]=1;

    
              //moves for the player1
    Text moveText1;
    moveText1.setFont(font);
    moveText1.setCharacterSize(16);
    moveText1.setFillColor(Color::Red);
    moveText1.setPosition(20, 16);

    Text moveText2;                      //moves for the player2
    moveText2.setFont(font);
    moveText2.setCharacterSize(16);
    moveText2.setFillColor(Color::Blue);
    moveText2.setPosition(N*ts - 120, 16);

    int moveCount1 = 0;
    int moveCount2 = 0;

    
int nextDx1 = 0, nextDy1 = 0;
int nextDx2 = 0, nextDy2 = 0;


Text winnerText;
winnerText.setFont(font);
winnerText.setCharacterSize(24);
winnerText.setPosition(N*ts/2 - 100, M*ts/2 + 50);

bool player1Wins = false;
bool player2Wins = false;
bool isDraw = false;


    while (window.isOpen())
    {
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;


         //for the game timer
         float elapsedTime = gameClock.getElapsedTime().asSeconds();
         int seconds = static_cast<int>(elapsedTime); 

        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed)
                window.close();
            
            // Handle key presses for Player 1 (arrow keys)
            if (e.type == Event::KeyPressed)
            {
                switch(e.key.code)
                {
                    case Keyboard::Left: 
                        dx1=-1; dy1=0; p1Moving=true; moveCount1++;
                        break;
                    case Keyboard::Right:
                        dx1=1; dy1=0; p1Moving=true; moveCount1++;
                        break;
                    case Keyboard::Up:
                        dx1=0; dy1=-1; p1Moving=true; moveCount1++;
                        break;
                    case Keyboard::Down:
                        dx1=0; dy1=1; p1Moving=true; moveCount1++;
                        break;

                         case Keyboard::Space: // Player 1 power-up
            if (player1.powerUps > 0 && !enemiesFrozen) {
                player1.powerUps--;
                enemiesFrozen = true;
                enemyFreezeTimer = 0;
                player1.usedPowerUp = true;
            }
            break;
            
        case Keyboard::LShift: // Player 2 power-up
            if (player2.powerUps > 0 && !enemiesFrozen) {
                player2.powerUps--;
                enemiesFrozen = true;
                enemyFreezeTimer = 0;
                player2.usedPowerUp = true;
            }
            break;
                    case Keyboard::Escape:
                        // Reset game
                        for (int i=1;i<M-1;i++)
                            for (int j=1;j<N-1;j++)
                                grid[i][j]=0;
                        x1=10; y1=0; dx1=0; dy1=0; p1Moving=false;
                        x2=N-11; y2=0; dx2=0; dy2=0; p2Moving=false;
                        Game = true;
                        
                        // Reset game clock
                        gameClock.restart();  // Restart the timer
    
                        // Reset move counts
                        moveCount1 = 0;
                        moveCount2 = 0;
                        
                        
                        player1.score = 0;
                        player2.score = 0;

                        player1.powerUps = 0;
                        player1.powerUps = 0;
                        break;
                }
            }

         
             if (e.type == Event::KeyPressed)
             {
                 switch(e.key.code)
                 {
                     case Keyboard::A:
                         dx2=-1; dy2=0; p2Moving=true; moveCount2++;
                         break;
                     case Keyboard::D:
                         dx2=1; dy2=0; p2Moving=true; moveCount2++;
                         break;
                     case Keyboard::W:
                         dx2=0; dy2=-1; p2Moving=true; moveCount2++;
                         break;
                     case Keyboard::S:
                         dx2=0; dy2=1; p2Moving=true; moveCount2++;
                         break;
                 }
             }
            
          
            
            // Handle key releases to stop movement
            if (e.type == Event::KeyReleased)
            {
    
                if (e.key.code == Keyboard::Left){
                     dx1=-1; dy1=0; 
                    }
                else if (e.key.code == Keyboard::Right) { 
                    dx1=1; dy1=0; 
                }
                else if (e.key.code == Keyboard::Up) {
                     dx1=0; dy1=-1; 
                    }
                else if (e.key.code == Keyboard::Down) { 
                    dx1=0; dy1=1; 
                }
                
                
                if (e.key.code == Keyboard::A) { dx2=-1; dy2=0; }
                else if (e.key.code == Keyboard::D) { dx2=1; dy2=0; }
                else if (e.key.code == Keyboard::W) { dx2=0; dy2=-1; }
                else if (e.key.code == Keyboard::S) { dx2=0; dy2=1; }
            }
        }

        if (!Game) continue;

        if (timer > delay)
        {

            // Reset captured tiles counters at start of move
            player1.tilesCapturedThisMove = 0;
            player2.tilesCapturedThisMove = 0;
            player1.multiplier = 1;
            player2.multiplier = 1;


            if (nextDx1 != 0 || nextDy1 != 0) {
                // Check if next move is valid moving towards the wall
                if (grid[y1 + nextDy1][x1 + nextDx1] != 1) {
                    dx1 = nextDx1;
                    dy1 = nextDy1;
                    p1Moving = true;
                }
                nextDx1 = nextDy1 = 0;
            }
            
            if (nextDx2 != 0 || nextDy2 != 0) {
                if (grid[y2 + nextDy2][x2 + nextDx2] != 1) {
                    dx2 = nextDx2;
                    dy2 = nextDy2;
                    p2Moving = true;
                }
                nextDx2 = nextDy2 = 0;
            }
        

// Move player 1 if they're moving
if (p1Moving)
{
    int newX1 = x1 + dx1;
    int newY1 = y1 + dy1;
    

    if (newX1 == x2 && newY1 == y2) {
        if (p2Moving) {
            // Both players are moving both die
            Game = false;
        } else {
            // Only Player 1 is moving Player 1 dies
            Game = false;
        }
    }
    
    x1 = newX1;
    y1 = newY1;
    
    // Boundary check
    if (x1<0) x1=0; 
    if (x1>N-1) x1=N-1;
    if (y1<0) y1=0; 
    if (y1>M-1) y1=M-1;

    // Collision with opponent's tiles
    if (grid[y1][x1] == 3) {  // Player 2's tiles
        Game = false;
    }
    
    // Only mark new tiles if not touching with the own tiles
    if (grid[y1][x1] == 0) {
        grid[y1][x1] = 2; // Player 1 leaves red tiles (value 2)
        player1.tilesCapturedThisMove++;
    } else if (grid[y1][x1] == 2) {
        // Collided with own trail - game over
        Game = false;
    }
}


if (p2Moving)
{
    int newX2 = x2 + dx2;
    int newY2 = y2 + dy2;
    
    // Check for player collision before moving
    if (newX2 == x1 && newY2 == y1) {
        if (p1Moving) {
            // Both players are moving - both die
            Game = false;
        } else {
            // Only Player 2 is moving - Player 2 dies
            Game = false;
        }
    }
    
    x2 = newX2;
    y2 = newY2;
    
    // Boundary check
    if (x2<0) x2=0; 
    if (x2>N-1) x2=N-1;
    if (y2<0) y2=0; 
    if (y2>M-1) y2=M-1;

    // Collision with the opponent tiles
    if (grid[y2][x2] == 2) {  
        Game = false;
    }
    
   
    if (grid[y2][x2] == 0) {
        grid[y2][x2] = 3;       //for the tail  of the player 2
        player2.tilesCapturedThisMove++;
    } else if (grid[y2][x2] == 3) {
        // Collided witht he own trail then game over
        Game = false;
    }
}


if (player1.tilesCapturedThisMove > 0) {
    player1.score += player1.tilesCapturedThisMove * player1.multiplier;
    
    
    if (player1.score >= 50 && (player1.score - 50) % 20 == 0){
        // Only grant if we have not already given one for this threshold
        int expectedPowerUps = (player1.score >= 130) ? (3 + (player1.score - 130) / 30) :(player1.score >= 100 ? 3 : player1.score >= 70 ? 2 : 1);
                               
        if (player1.powerUps < expectedPowerUps){
            player1.powerUps++;
        }
    }
}

// Do the same for Player 2
if (player2.tilesCapturedThisMove > 0) {
    player2.score += player2.tilesCapturedThisMove * player2.multiplier;
    
    if (player2.score >= 50 && (player2.score - 50) % 20 == 0) {
        int expectedPowerUps = (player2.score >= 130) ? (3 + (player2.score - 130) / 30) :(player2.score >= 100 ? 3 : player2.score >= 70 ? 2 : 1);
                               
        if (player2.powerUps < expectedPowerUps) {
            player2.powerUps++;
        }
    }
}

    
            // Wall collision for player 1
            if (grid[y1][x1] == 1 && p1Moving)
            {
                dx1 = dy1 = 0;
                p1Moving = false;
                for (int i=0; i<enemyCount; i++)
                    drop(a[i].y/ts, a[i].x/ts);
        
                for (int i=0; i<M; i++)
                    for (int j=0; j<N; j++)
                        if (grid[i][j] == -1) grid[i][j] = 0;
                        else grid[i][j] = 1;
            }
        
            // Wall collision for player 2
            if (grid[y2][x2] == 1 && p2Moving)
            {
                dx2 = dy2 = 0;
                p2Moving = false;
                for (int i=0; i<enemyCount; i++)
                    drop(a[i].y/ts, a[i].x/ts);
        
                for (int i=0; i<M; i++)
                    for (int j=0; j<N; j++)
                        if (grid[i][j] == -1) grid[i][j] = 0;
                        else grid[i][j] = 1;
            }



            if(player1.score > player2.score){
                player1Wins = true;
                player2Wins = false;
                isDraw = false;
            }
            else if(player1.score < player2.score){
                player2Wins = true;
                player1Wins = false;
                isDraw = false;
            }
            else{
                isDraw = true;
                player1Wins = false;
                player2Wins = false;
            }


            if (enemiesFrozen) {
    enemyFreezeTimer += time;
    if (enemyFreezeTimer >= freezeDuration) {
        enemiesFrozen = false;
    }
}

        
            timer = 0;
        }

        // Enemy movement
       for (int i=0;i<enemyCount;i++) {
    if (!enemiesFrozen) {
        a[i].move();
    }
}


        

        // Enemy collision checks
for (int i=0;i<enemyCount;i++)
{
    // Convert enemy position to grid coordinates
    int enemyGridX = a[i].x / ts;
    int enemyGridY = a[i].y / ts;
    
    // Check collision with Player 1's tiles
    if (grid[enemyGridY][enemyGridX] == 2) Game = false;
    
    // Check direct collision with Player 1
    if (enemyGridX == x1 && enemyGridY == y1) Game = false;
    
    // Check collision with Player 2's tiles
    if (grid[enemyGridY][enemyGridX] == 3) Game = false;
    
    // Check direct collision with Player 2
    if (enemyGridX == x2 && enemyGridY == y2) Game = false;
}

        /////////draw//////////
        window.clear();


        //for the drawing of the background image

        window.draw(multiplayerSprite);


        // Draw grid tiles
        for (int i=0;i<M;i++)
            for (int j=0;j<N;j++)
            {
                if (grid[i][j]==0) continue; // Empty space
                if (grid[i][j]==1) // Wall
                {
                    sTile1.setTextureRect(IntRect(0,0,ts,ts));
                    sTile1.setPosition(j*ts,i*ts);
                    window.draw(sTile1);
                }
                else if (grid[i][j]==2) // Player 1 (red) tiles
                {
                    sPlayer1Tile.setPosition(j*ts,i*ts);
                    window.draw(sPlayer1Tile);
                }
                else if (grid[i][j]==3) // Player 2 (blue) tiles
                {
                    sPlayer2Tile.setPosition(j*ts,i*ts);
                    window.draw(sPlayer2Tile);
                }
                
            }

            

        // Draw player 1 (red)
        sTile1.setTextureRect(IntRect(56,0,ts,ts));
        sTile1.setColor(Color::White);
        sTile1.setPosition(x1*ts,y1*ts);
        window.draw(sTile1);

        // Draw player 2 (blue)
        sTile2.setTextureRect(IntRect(36,0,ts,ts));
        sTile2.setColor(Color::Blue);
        sTile2.setPosition(x2*ts,y2*ts);
        window.draw(sTile2);
        sTile2.setColor(Color::White);

        sEnemy.rotate(10);
        for (int i=0;i<enemyCount;i++)
        {
            sEnemy.setPosition(a[i].x,a[i].y);
            window.draw(sEnemy);
        }

       if (!Game) {
    window.draw(sGameover);

    if (player1Wins) {
        winnerText.setString("Player 1 Wins!");
        winnerText.setFillColor(Color::Red);
    } 
    else if (player2Wins) {
        winnerText.setString("Player 2 Wins!");
        winnerText.setFillColor(Color::Blue);
    } 
    else if (isDraw) {
        winnerText.setString("It's a Draw!");
        winnerText.setFillColor(Color::White);
    }

    // Center the text
    FloatRect textRect = winnerText.getLocalBounds();
    winnerText.setOrigin(textRect.left + textRect.width/2.0f, 
                        textRect.top + textRect.height/2.0f);
    winnerText.setPosition(N*ts/2, M*ts/2 + 50);

    window.draw(winnerText);
}

        moveText1.setString("Moves (P1): " + to_string(moveCount1));
        window.draw(moveText1);

        moveText2.setString("Moves (P2): " + to_string(moveCount2));
        window.draw(moveText2);

       // Update and draw score/power displays
scoreText1.setString(
    "P1 - Score: " + to_string(player1.score) + 
    " | Power: " + to_string(player1.powerUps) +
    (player1.multiplier > 1 ? " (x" + to_string(player1.multiplier) + ")" : "")
);

scoreText2.setString(
    "P2 - Score: " + to_string(player2.score) + 
    " | Power: " + to_string(player2.powerUps) +
    (player2.multiplier > 1 ? " (x" + to_string(player2.multiplier) + ")" : "")
);

window.draw(scoreText1);
window.draw(scoreText2);


        timeText.setString("Time: " + to_string(seconds));
        window.draw(timeText);

        
        window.display();
    }
}



int main()
{
    srand(time(0));


    initializeScoreboard();
    loadScores();
    
    
    
    RenderWindow window(VideoMode(N*ts, M*ts), "Xonix Game!");
    window.setFramerateLimit(60);


    Text timeText;   
    Font font;

    font.loadFromFile("fonts/arial.ttf"); 
    
timeText.setFont(font);                                               
timeText.setCharacterSize(16);
timeText.setFillColor(Color::Red);
timeText.setPosition(20, 35);  


    Texture t1,t2,t3;
    t1.loadFromFile("images/tiles.png");
    t2.loadFromFile("images/gameover.png");
    t3.loadFromFile("images/enemy.png");

    

    Texture backgroundTexture;
backgroundTexture.loadFromFile("images/background.jpg"); 
Sprite backgroundSprite(backgroundTexture);
backgroundSprite.setScale(                                         
    float(window.getSize().x) / backgroundTexture.getSize().x,
    float(window.getSize().y) / backgroundTexture.getSize().y
);


Texture backgroundGame;
backgroundGame.loadFromFile("images/backgroundGame.jpg"); 
Sprite backgroundGameSprite(backgroundGame);
backgroundGameSprite.setScale(                                          
    float(window.getSize().x) / backgroundGame.getSize().x,
    float(window.getSize().y) / backgroundGame.getSize().y
);



    Sprite sTile(t1), sGameover(t2), sEnemy(t3);
    sGameover.setPosition(100,100);
    sEnemy.setOrigin(20,20);

    int enemyCount = 2;       
    Enemy a[25];

    bool Game=true;
    int x=0, y=0, dx=0, dy=0;
    float timer=0, delay=0.07;             //player move after every 0.07 seconed
    Clock clock;
 
    //clock for the end menu
    bool showingGameOver = false; 
Clock gameOverClock;          
float endMenuDelay = 1.75f;              //delay between the end menu and the game over

 
    for (int i=0;i<M;i++)                                                                             //to make the boders of the game a walls
     for (int j=0;j<N;j++)
      if (i==0 || j==0 || i==M-1 || j==N-1)  grid[i][j]=1;
 


int screenState = 0; // 0 = Menu, 1 = Levels, 2 = Game, 3 = Scoreboard , 4 = end menu

Text menuItems[5];  // Increased to 5
string labels[] = {"Start Game", "Levels", "Multiplayer Mode", "Scoreboard", "Exit Game"};
Text levelItems[4]; 
string levelLabels[] = {"Easy", "Medium", "Difficult", "Continuous Mode"};

Text endItems[3];                                              //for the end menu
string endLabels[] = {"Restart", "Main Menu", "Exit Game"};

for (int i = 0; i < 3; i++) {
    endItems[i].setFont(font);
    endItems[i].setCharacterSize(24);
    endItems[i].setFillColor(Color::White);
    endItems[i].setPosition(120, 200 + i * 50); 
    endItems[i].setString(endLabels[i]);
}


Text backButton;
backButton.setFont(font);
backButton.setCharacterSize(24);
backButton.setFillColor(Color::Yellow);
backButton.setPosition(120, 300+50); 
backButton.setString("Back");


for (int i = 0; i < 5; i++) {
    menuItems[i].setFont(font);
    menuItems[i].setCharacterSize(24);
    menuItems[i].setFillColor(Color::White);
    menuItems[i].setPosition(100, 100 + i * 50);
    menuItems[i].setString(labels[i]);
}

for (int i = 0; i<4; i++) {
    levelItems[i].setFont(font);
    levelItems[i].setCharacterSize(24);
    levelItems[i].setFillColor(Color::Yellow);
    levelItems[i].setPosition(120, 100 + i * 50);
    levelItems[i].setString(levelLabels[i]);
}


Text moveText;
moveText.setFont(font);                                                                
moveText.setCharacterSize(16);
moveText.setFillColor(Color::Red);
moveText.setPosition(20, 16);

Text scoreText;
scoreText.setFont(font);
scoreText.setCharacterSize(16);
scoreText.setFillColor(Color::Green); 
scoreText.setPosition(N*ts - 120, 16); 


Text powerUpDisplay;
powerUpDisplay.setFont(font);
powerUpDisplay.setCharacterSize(16);
powerUpDisplay.setPosition(N*ts - 120, 40);

Music music;
music.openFromFile("Music/Forest.mp3");   //for the playing of the music




    while (window.isOpen())
    {


       
    music.play();

    


        if (showingGameOver && gameOverClock.getElapsedTime().asSeconds() >= endMenuDelay) {
            showingGameOver = false; // Delay over, stop showing game over
        }


        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer+=time;

        //for the game timer
        float elapsedTime = gameClock.getElapsedTime().asSeconds();
int seconds = static_cast<int>(elapsedTime); 

 
if (elapsedTime  > 30 && !switched)                          //for changing the movement after 30 sec
{
    for (int i = 0; i < enemyCount / 2; i++)
    {
        a[i].geometricMode = true;
        a[i].direction = (i % 2 == 0) ? 1 : 2;
    }
    switched = true;
}







// Increase enemy speed every 20 seconds
speedIncreaseTimer += time;
if (speedIncreaseTimer >= speedIncreaseInterval)
{
    speedIncreaseTimer = 0.0f; // Reset timer
    
    // Increase speed for all enemies
    for (int i = 0; i < enemyCount; i++)
    {
        a[i].increaseSpeed(speedIncreaseAmount);
    }
    
}

char timeBuffer[50];
sprintf(timeBuffer, "Time: %d", seconds);
timeText.setString(timeBuffer);



        //for the continous mode
if (isContinuousMode) {
    addEnemies(enemyCount, enemyTimer, time);
}



// Check if player earned new power-ups
checkPowerUps();

// Activate power-up when Space is pressed
if (Keyboard::isKeyPressed(Keyboard::Space) && powerUps > 0 && !freezeEnemies) {
    powerUps--;
    freezeEnemies = true;
    freezeTime = 3.0f;
    
    // Stop all enemies
    for (int i = 0; i < enemyCount; i++) {
        a[i].dx = 0;
        a[i].dy = 0;
    }
}

// Update power-up timer                        //for the power up of the enemies
if (freezeEnemies){
    freezeTime -= time;
    if (freezeTime <= 0) {
        freezeEnemies = false;
        // Restore enemy movement
        for (int i = 0; i < enemyCount; i++) {
            float speed = a[i].baseSpeed * a[i].speedMultiplier;
            a[i].dx = (rand()%2 == 0) ? speed : -speed;
            a[i].dy = (rand()%2 == 0) ? speed : -speed;
        }
    }
}

        Event e;                                             
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed)
                window.close();

            // Handling Mouse Clicks for the Main Menu and Level Menu
if (e.type == Event::MouseButtonPressed && e.mouseButton.button == Mouse::Left) {
    Vector2i mouse = Mouse::getPosition(window);

    if (screenState == 0) { // Main Menu
        for (int i = 0; i < 5; i++) {
            if (menuItems[i].getGlobalBounds().contains(mouse.x, mouse.y)) {
                if (i == 0) { 
                    // Start Game
                    resetGame(Game, x, y, dx, dy, moveCount, grid, enemyTimer, gameClock, a , switched);
                    setLevel(0, enemyCount, delay);                                                   //initially start with the easy level;
                    screenState = 2;  // Transition to the game state
                }
                else if (i == 1) {
                    screenState = 1; // Levels
                }
                else if (i == 2) {
                    screenState = 5; // Multiplayer Mode 
                    multiplayerMode(window , font);   
                    
                }
                else if (i == 3) {
                    screenState = 3; // Scoreboard
                }
                else if (i == 4) {
                    window.close();  // Exit
                }
            }
        }
    }
    else if (screenState == 1) { // Level Menu
        for (int i = 0; i < 4; i++) {
            if (levelItems[i].getGlobalBounds().contains(mouse.x, mouse.y)) {

                
                isContinuousMode = (i == 3); // store the 1 for true

                setLevel(i, enemyCount, delay);

                screenState = 2; // Start Game
                resetGame(Game, x, y, dx, dy, moveCount, grid, enemyTimer, gameClock, a , switched);
                enemyTimer = 0.0f; // Reset timer when starting new game
            }
        }
        
        if (backButton.getGlobalBounds().contains(mouse.x, mouse.y)) {
            screenState = 0; // Go back to Main Menu
        }
    }
}

        }

                                        // clicking for the end menu

        if (e.type == Event::MouseButtonPressed && e.mouseButton.button == Mouse::Left)
{
    Vector2i mouse = Mouse::getPosition(window);

    if (showingGameOver) continue; // Ignore clicks during delay

   
     if (screenState == 4) { 
        // handle clicks for End Menu
        for (int i = 0; i < 3; i++) {
            if (endItems[i].getGlobalBounds().contains(mouse.x, mouse.y)) {
                if (i == 0) { 
                    // Restart Game
                    screenState = 2;
                    resetGame(Game, x, y, dx, dy, moveCount, grid, enemyTimer, gameClock, a , switched);
                    

                    if (isContinuousMode) {
                        enemyCount = 2;  
                        enemyTimer = 0.0f;  
                    }

                     

                }
                else if (i == 1) {
                    screenState = 0; // Back to Main Menu
                }
                else if (i == 2) {
                    window.close(); // Exit Game
                }
            }
        }
    }
}


        
        if(screenState == 0 || screenState == 1 || screenState ==3) {
            handleMenu(window, screenState, menuItems, levelItems, backButton, font, backgroundSprite);
            continue;
        }
       
          

        else if(screenState == 4) {
            if(!showingGameOver) {  
                endMenu(window, screenState, endItems, font, backgroundSprite, score);
            }
            continue;
        }


        else if (screenState == 5) {
            
            window.clear();
            
             window.draw(backgroundSprite);

            window.display();
            continue;
        }
        
    

        keyPressed = false;

        if (Keyboard::isKeyPressed(Keyboard::Left)) {dx=-1; dy=0; keyPressed = true;}
else if (Keyboard::isKeyPressed(Keyboard::Right)) {dx=1; dy=0; keyPressed = true;}
else if (Keyboard::isKeyPressed(Keyboard::Up)) {dx=0; dy=-1; keyPressed = true;}
else if (Keyboard::isKeyPressed(Keyboard::Down)) {dx=0; dy=1; keyPressed = true;}

        
        if (!Game) continue;

        if (timer>delay  || delay == 0.0)
        {
         x+=dx;
         y+=dy;

         if (x<0) x=0; if (x>N-1) x=N-1;
         if (y<0) y=0; if (y>M-1) y=M-1;


         if (grid[y][x] == 2){
            Game = false;
            showingGameOver = true;    // Set the flag
            gameOverClock.restart();   // Start the timer
            screenState = 4;           // Still go to end menu state
        }
        
        if (grid[y][x] == 0){                 //tile capture
            grid[y][x] = 2;
            tilesCapturedInMove++;
            if (keyPressed) {
                moveCount++;
            }
        }

         timer=0;
        }

        for (int i=0;i<enemyCount;i++) a[i].move();

        if (grid[y][x]==1)               
          {
           dx=dy=0;


           // Store tiles captured before they get cleared
    lastMoveTiles = tilesCapturedInMove;
    updateScore();
    tilesCapturedInMove = 0;

           for (int i=0;i<enemyCount;i++)
                drop(a[i].y/ts, a[i].x/ts);

           for (int i=0;i<M;i++)
             for (int j=0;j<N;j++)
              if (grid[i][j]==-1) grid[i][j]=0;
              else grid[i][j]=1;
          }

          for (int i=0;i<enemyCount;i++)
          if (grid[a[i].y/ts][a[i].x/ts]==2) {
              float elapsedTime = gameClock.getElapsedTime().asSeconds();
              updateScoreboard(score, elapsedTime); // Save score if it qualifies
              Game = false;
              showingGameOver = true;
              gameOverClock.restart();
              screenState = 4;
          }
  

      /////////draw//////////

      
      window.clear();
      
window.draw(backgroundGameSprite); // <-- Draw the background before drawing tiles and player


      for (int i=0;i<M;i++)
        for (int j=0;j<N;j++)
         {
            if (grid[i][j]==0) continue;
            if (grid[i][j]==1) sTile.setTextureRect(IntRect( 0,0,ts,ts));
            if (grid[i][j]==2) sTile.setTextureRect(IntRect(54,0,ts,ts));
            sTile.setPosition(j*ts,i*ts);
            window.draw(sTile);
         }

      sTile.setTextureRect(IntRect(36,0,ts,ts));
      sTile.setPosition(x*ts,y*ts);
      window.draw(sTile);

      sEnemy.rotate(10);
      for (int i=0;i<enemyCount;i++)
       {
        sEnemy.setPosition(a[i].x,a[i].y);
        window.draw(sEnemy);
       }

 
powerUpDisplay.setString("Power-Ups: " + to_string(powerUps));   
powerUpDisplay.setFillColor(Color::Magenta);
window.draw(powerUpDisplay);



       if (showingGameOver) window.draw(sGameover); 


       window.draw(timeText);  // show the elapsed game time on screen

                                                                 
      moveText.setString("Moves: " + to_string(moveCount));
        window.draw(moveText);

        // Update score display
scoreText.setString("Score: " + to_string(score));
window.draw(scoreText);


       window.display();
    }

    return 0;
}