#include <LinkedList.h>
const int JOYSTICK_X_Pin = A4; // Reversed pin values from actual, values because my joystick is aligned vertically
const int JOYSTICK_Y_Pin = A5;

const byte ANODE_PINS[8] = {13, 12, 11, 10, 9, 8, 7, 6};
const byte CATHODE_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};

const unsigned long PAUSE_DURATION = 275;
const unsigned long RESET_GAME = 2000;

const int N_ROWS = 8;
const int N_COLS = 8;

// 1: up, 2: down, 3: left, 4: right
const int D_UP = 1;
const int D_DOWN = 2;   
const int D_LEFT = 3;
const int D_RIGHT = 4;

struct Segment 
{
  int x;
  int y;
};

unsigned long lastUpdateTime = 0; // To implement intervals for each PAUSE_DURATION
int currDirection = D_UP; // Snake starts by going up 
byte isOn[8][8]; // Led board (on = 1, off = 0)

// Snake body
LinkedList<Segment*> snakeBody = LinkedList<Segment*>();
// Location of apple
Segment *appleLocation;

/* Initializes a snake with 2 segments in middle
 * of display.
 */
void initializeSnake() {
  snakeBody.add(new Segment{4, 3});
  snakeBody.add(new Segment{4,4});
}

/* Returns the position of the snake's possible next move
 * based off the current direction of the snake.
 */
Segment *getSnakeMove() {
  Segment *head = snakeBody.get(0);
  switch(currDirection) {
    case D_DOWN:
      return new Segment{head->x + 1, head->y}; // My joystick is sideways so I have to account for that when adding or removing 1
    case D_UP:
      return new Segment{head->x - 1, head->y};
    case D_RIGHT:
      return new Segment{head->x, head->y - 1};
    case D_LEFT:
      return new Segment{head->x, head->y + 1};
  } 
}

/* Returns the possible next direction of the snake depending
 * on input value from the joysticks.
 */
int getJoystickInput() {
  int direction = currDirection;

  // Read the analog values from the joystick
  int joystickXValue = analogRead(JOYSTICK_X_Pin);
  int joystickYValue = analogRead(JOYSTICK_Y_Pin);

  if (joystickXValue < 400) {
    direction = D_LEFT; // Move left
  } else if (joystickXValue > 700) {
    direction = D_RIGHT; // Move right
  } else if (joystickYValue < 300) {
    direction = D_UP; // Move up
  } else if (joystickYValue > 700) {
    direction = D_DOWN; // Move down
  }

  // To stop snake from going in the direction it just came from
  // Ex: If snake wants to turn down as it's moving up, this isn't a legal move
  if ((direction == D_DOWN && currDirection == D_UP) || (direction == D_UP && currDirection == D_DOWN) ||
    (direction == D_RIGHT && currDirection == D_LEFT) || (direction == D_LEFT && currDirection == D_RIGHT)) {
    direction = currDirection;
  }
  return direction;
}

/* Receives a segment and checks if that segment is a part of the
 * snake's body. Returns true if it is and false otherwise.
 */
bool isSnakeSegment(int nextX, int nextY) {
  for (int i = 0; i < snakeBody.size(); i++) {
    Segment *seg = snakeBody.get(i);
    if (seg->x == nextX && seg->y == nextY) {
      return true;
    }
  }
  return false;
}

/* Receives the possible next segment of the snake (next move) 
 * and determines if that move is valid. If the snake goes
 * out of bounds or collides with itself false is returned. Otherwise
 * true is returned.
 */
bool checkMove(Segment *seg) {
  int nextX = seg->x;
  int nextY = seg->y;
  
  // Check bounds
  if (nextX < 0 || nextX >= N_ROWS || nextY < 0 || nextY >= N_COLS) {
    return false;
  }

  // Check if snake will collide with itself
  if (isSnakeSegment(nextX, nextY)) {
    return false;
  }
  return true;
}

/* Turns off all Leds in isOn.
 */
void allLedsOff() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      isOn[row][col] = 0;
    }
  }
}

/* Moves the snake in the current direction.
 */
void moveSnake(Segment *nextSeg) {
  delete(snakeBody.pop()); // Remove a seg from tail
  snakeBody.add(0, nextSeg); // Add seg to head
}

/* Turns on the Leds in isOn that match the 
 * positions of the snake's body and the apple.
 */
void createLed() {
  for (int i = 0; i < snakeBody.size(); i++) {
    Segment *seg = snakeBody.get(i);
    int currX = seg->x;
    int currY = seg->y;

    isOn[currX][currY] = 1;
  }
  isOn[appleLocation->x][appleLocation->y] = 1;
}

/* Loops through the Leds isOn and turns on
 * the specified Leds that are signified to be on
 * to display the game.
 */
void display() {
  for (byte row = 0; row < N_ROWS; row++) {
    for (byte col = 0; col < N_ROWS; col++) {
      if (isOn[row][col] == 1) {
          digitalWrite(CATHODE_PINS[col], LOW);
      } else {
          digitalWrite(CATHODE_PINS[col], HIGH);
      }
    }
    digitalWrite(ANODE_PINS[row], LOW);
    delay(2);
    digitalWrite(ANODE_PINS[row], HIGH);
  }
}

/* Receives the next move of the snake (next segment it will occupy)
 * and updates the games display, snake's body, and apple if eaten to reflect that move.
 */
void playMove(Segment *nextSeg) {
  if ((appleLocation->x == nextSeg->x) && (appleLocation->y == nextSeg->y)) {
    addToSnake(nextSeg);
  } else {
    moveSnake(nextSeg);
  }

  lastUpdateTime = millis();

  allLedsOff();
  createLed();
}

/* Resets the game to be played again.
 */
void restart() {
  while (snakeBody.size() > 0) { // reset snake
    delete(snakeBody.pop());
  }
  initializeSnake();

  delete(appleLocation); // reset apple
  appleLocation = initializeApple();
  
  unsigned long resetTime = millis();

  while (millis() - resetTime < RESET_GAME) { // Wait until RESET_GAME time has passed (2000 ms)
  }

  allLedsOff();
  currDirection = D_UP;
}

/* Initializes an apple at a random position
 * not occupied by the snake.
 */
Segment *initializeApple() { 
  int x;
  int y;

  do {
    x = random(N_ROWS);
    y = random(N_COLS); 
  } while(isSnakeSegment(x, y));

  return new Segment{x, y};
}

/* Receives the next move of the snake where they have eaten an
 * apple and grows the snake. Initializes a new apple because
 * old apple was eaten.
 */
void addToSnake(Segment *nextSeg) {
  snakeBody.add(0, nextSeg); // Grow snake
  delete(appleLocation); // Delete old apple
  appleLocation = initializeApple(); // Create new apple
}

/* Checks to see if the player has beaten the game
 * by filling the board entirely with a snake.
 * Returns true if the player has won and false otherwise.
 */
bool checkWin() {
  if (snakeBody.size() == 64) { // You won the game
    return true;
  }
  return false; // Still need to play
}

/* Sets up the snake game.
 */
void setup() {
  randomSeed(analogRead(0)); 
  pinMode(JOYSTICK_X_Pin, INPUT);
  pinMode(JOYSTICK_Y_Pin, INPUT);

  for (byte i = 0; i < 8; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    pinMode(CATHODE_PINS[i], OUTPUT);

    digitalWrite(ANODE_PINS[i], HIGH);
    digitalWrite(CATHODE_PINS[i], HIGH);
  }

  Serial.begin(115200);
  initializeSnake();
  appleLocation = initializeApple();
}

/* Continously loops to play the snake game.
 */
void loop() {
  display(); // Dislay constantly
  if (millis() > PAUSE_DURATION + lastUpdateTime) { // Take in input every PAUSE_DURATION
    currDirection = getJoystickInput(); // Store current direction to compare it to the next direction
    Segment *nextSeg = getSnakeMove();

    if (!checkWin() && checkMove(nextSeg)) { // the game hasn't been won and the next move is valid
      playMove(nextSeg);
    } else {
      restart();
    }
  }
}