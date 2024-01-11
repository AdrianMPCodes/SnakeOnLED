#include <LinkedList.h>
const int joystickXPin = A4; // Reversed pin values from actual, values because my joystick is aligned vertically
const int joystickYPin = A5;

const byte ANODE_PINS[8] = {13, 12, 11, 10, 9, 8, 7, 6};
const byte CATHODE_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};

const unsigned long PAUSE_DURATION = 350;
const unsigned long RESET_GAME = 2000;

unsigned long currTime = 0;
unsigned long lastUpdateTime = 0;

const int N_ROWS = 8;
const int N_COLS = 8;

//0: no input, 1: up, 2: down, 3: left, 4: right
const int D_UP = 1;
const int D_DOWN = 2;   
const int D_LEFT = 3;
const int D_RIGHT = 4;

int currDirection = D_UP;

struct Segment 
{
  int x;
  int y;
};

// Snake body
LinkedList<Segment*> snakeBody = LinkedList<Segment*>();
// Location of apple
Segment *appleLocation;

/* Initialize a snake with 2 segments in middle
 * of display
 */
void initializeSnake() {
  snakeBody.add(new Segment{4, 3});
  snakeBody.add(new Segment{4,4});
}

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

int getJoystickInput() {
  int direction = currDirection;

  // Read the analog values from the joystick
  int joystickXValue = analogRead(joystickXPin);
  int joystickYValue = analogRead(joystickYPin);

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
  switch (direction) {
    case D_DOWN:
      if (currDirection == D_UP) {
        direction = D_UP;
      }
      break;
    case D_UP:
      if (currDirection == D_DOWN) {
        direction = D_DOWN;
      }
      break;
    case D_RIGHT:
      if (currDirection == D_LEFT) {
        direction = D_LEFT;
      }
      break;
    case D_LEFT:
      if (currDirection == D_RIGHT) {
        direction = D_RIGHT;
      }
      break;
  }
  return direction;
}

bool isSnakeSegment(int nextX, int nextY) {
  for (int i = 0; i < snakeBody.size(); i++) {
    Segment *seg = snakeBody.get(i);
    if (seg->x == nextX && seg->y == nextY) {
      return true;
    }
  }
  return false;
}

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

void allLedsOff() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      digitalWrite(ANODE_PINS[row], HIGH);
      digitalWrite(CATHODE_PINS[col], HIGH);
    }
  }
}

void moveSnake(Segment *nextSeg) {
  snakeBody.add(0, nextSeg); // Add seg to head
  delete(snakeBody.pop()); // Remove a seg from tail
}

void displaySnake() {
  for (int i = 0; i < snakeBody.size(); i++) {
    Segment *seg = snakeBody.get(i);
    int currX = seg->x;
    int currY = seg->y;

    digitalWrite(ANODE_PINS[currX], LOW);
    digitalWrite(CATHODE_PINS[currY], LOW);
  }
}

void displayApple() { 
    int appleX = appleLocation->x;
    int appleY = appleLocation->y;

    digitalWrite(ANODE_PINS[appleX], LOW);
    digitalWrite(CATHODE_PINS[appleY], LOW);
}

void playMove(Segment *nextSeg) {
  allLedsOff(); // Clear board before doing anything

  if ((appleLocation->x == nextSeg->x) && (appleLocation->y == nextSeg->y)) {
    //addToSnake(nextSeg);
  } else {
    moveSnake(nextSeg);
  }

  displaySnake();
  displayApple();
}

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

  currDirection = D_UP;
  allLedsOff();
}

Segment *initializeApple() { 
  int x;
  int y;

  do {
    x = random(N_ROWS);
    y = random(N_COLS);  
  } while(isSnakeSegment(x, y));

  return new Segment{x, y};
}

void addToSnake(Segment *nextSeg) {
  snakeBody.add(0, nextSeg); // Grow snake
  delete(appleLocation); // Delete old apple
  appleLocation = initializeApple(); // Create new apple
}

void setup() {
  pinMode(joystickXPin, INPUT);
  pinMode(joystickYPin, INPUT);

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

void loop() {
  currTime = millis();
  if (currTime - lastUpdateTime >= PAUSE_DURATION) { // Take in input every PAUSE_DURATION
    lastUpdateTime = currTime;

    currDirection = getJoystickInput(); // Store current direction to compare it to the next direction
    Segment *nextSeg = getSnakeMove();
    if (checkMove(nextSeg)) { // Move is valid
      playMove(nextSeg);
    } else {
      restart();
    }
  }
}
