// Change this to 0,1, or 2.  Each produces more output than the one before.
// Output to the Serial Monitor
#define VERBOSE      (2)

// The value at which each servo does not move.
#define LEFT_NO_MOVE  (90)
#define RIGHT_NO_MOVE (90)

#define FORWARD_SPEED (7)
#define TURN_SPEED    (5)

#define NUM_EYES      (4)
#define EYE_FAR_LEFT  (0)
#define EYE_LEFT      (1)
// #define EYE_MIDDLE    (2)
#define EYE_RIGHT     (2)
#define EYE_FAR_RIGHT (3)

#define LEFT_MOTOR_PIN  (13)
#define RIGHT_MOTOR_PIN (12)
#define LED_PIN         (7)

#define WAIT_TIME   (5000)

//------------------------------------------------------------------------------
// INCLUDES 
//------------------------------------------------------------------------------

#include <Servo.h>

//------------------------------------------------------------------------------
// GLOBALS - variables used all over the program
//------------------------------------------------------------------------------

Servo left, right;

int inputs[NUM_EYES];
long avg_white[NUM_EYES];
long avg_black[NUM_EYES];
long cutoff[NUM_EYES];

//------------------------------------------------------------------------------
// METHODS
//------------------------------------------------------------------------------


void setup() {
  int i;
  
  // open comms
  Serial.begin(9600);
  Serial.println(F("START"));
  pinMode(LED_PIN,OUTPUT);

  // the lighting conditions are different in every room.
  // when the robot turns on, give the user 5 seconds to 
  // put the robot's eyes over the white surface and study
  // what white looks like in this light for one second.
  // The LED will turn on while it is studying.
  seeWhite();
  
  // After that, give the user 5 seconds to 
  // put the robot's eyes over the black surface and study
  // what black looks like in this light for one second.
  // The LED will turn on while it is studying.
  seeBlack();

  // Now that the robot knows what white and black looks like,
  // we can explain the difference.
  learnDiff();

  setupServos();
  
  // Give the user another 5 seconds to put the robot at
  // the start of the line.
#if VERBOSE > 0
  Serial.println(F("Move me to the start of the line..."));
#endif
  wait(10000);  // give user a moment to place robot
}


//------------------------------------------------------------------------------
void setupServos() {
  // attach servos
  left.attach(LEFT_MOTOR_PIN);
  right.attach(RIGHT_MOTOR_PIN);
  
  // make sure we aren't moving.
  steer(0,0);
}

//------------------------------------------------------------------------------
// average the color white that each eye sees during the next few seconds.
void seeWhite() {
  int i;

#if VERBOSE > 0
  Serial.println(F("Move me so I can see only white..."));
#endif
  wait(WAIT_TIME);  // give user a moment to place robot

  digitalWrite(LED_PIN,HIGH);  // turn on light - ready to sample

#if VERBOSE > 0
  Serial.println(F("Sampling white..."));
#endif
  memset(avg_white,0,sizeof(int)*NUM_EYES);
  int samples=0;
  long start=millis();
  while(millis()-start < 2000) {
    look();
    samples++;
    for(i=0;i<NUM_EYES;++i) {
      avg_white[i]+=inputs[i];
    }
    delay(5);
  }
  
  // average = sum / number of samples
#if VERBOSE > 1
  Serial.print(samples);
  Serial.println(F(" white samples: "));
#endif
  for(i=0;i<NUM_EYES;++i) {
    avg_white[i] = (float)avg_white[i] / (float)samples;
#if VERBOSE > 1
  Serial.print(avg_white[i]);
  Serial.print('\t');
#endif
  }
#if VERBOSE > 1
  Serial.print('\n');
#endif

  digitalWrite(LED_PIN,LOW);  // turn off light - done sampling
}

//------------------------------------------------------------------------------
void seeBlack() {
  int i;

#if VERBOSE > 0
  Serial.println(F("Move me so I can see only black..."));
#endif
  wait(WAIT_TIME);  // give user a moment to place robot

  digitalWrite(LED_PIN,HIGH);  // turn on light - ready to sample

#if VERBOSE > 0
  Serial.println(F("Sampling black..."));
#endif
  memset(avg_black,0,sizeof(int)*NUM_EYES);
  int samples=0;
  long start=millis();
  while(millis()-start < 2000) {
    look();
    samples++;
    for(i=0;i<NUM_EYES;++i) {
      avg_black[i]+=inputs[i];
    }
    delay(5);
  }

  // average = sum / number of samples
#if VERBOSE > 1
  Serial.print(samples);
  Serial.println(F(" black samples: "));
#endif
  for(i=0;i<NUM_EYES;++i) {
    avg_black[i] = (float)avg_black[i] / (float)samples;
#if VERBOSE > 1
  Serial.print(avg_black[i]);
  Serial.print('\t');
#endif
  }
#if VERBOSE > 1
  Serial.print('\n');
#endif

  digitalWrite(LED_PIN,LOW);  // turn off light - done smapling
}

//------------------------------------------------------------------------------
void learnDiff() {
  int i;
  
#if VERBOSE > 1
  Serial.println(F("cutoff: "));
#endif
  for(i=0;i<NUM_EYES;++i) {
    cutoff[i] = ( avg_white[i] + avg_black[i] ) / 2.0f;
#if VERBOSE > 1
  Serial.print(avg_black[i]);
  Serial.print('\t');
#endif
  }
#if VERBOSE > 1
  Serial.print('\n');
#endif
}


//------------------------------------------------------------------------------
void loop() {
  followLine();
}


//------------------------------------------------------------------------------
void wait(int ms) {
  // give us time to position robot
  long start=millis();
  while(millis()-start < ms) {
    delay(5);
  }
}


//------------------------------------------------------------------------------
char eye_sees_white(int i) {
  Serial.print("Eye_sees_white.");
  Serial.println();
  return inputs[i] > cutoff[i];
}

//------------------------------------------------------------------------------
char eye_sees_black(int i) {
  Serial.print("Eye_sees_black.");
  Serial.println();
  return inputs[i] < cutoff[i];
}

//------------------------------------------------------------------------------
char all_eyes_see_white() {
  int i;
  for(i=0;i<NUM_EYES;++i) {
    if(eye_sees_black(i)) return 0;  // eye doesn't see white, quit
  }
  
  return 1;
}

//------------------------------------------------------------------------------
char all_eyes_see_black() {
  int i;
  for(i=0;i<NUM_EYES;++i) {
    if(eye_sees_white(i)) return 0;  // eye doesn't see black, quit
  }
  
  return 1;
}

//------------------------------------------------------------------------------
void followLine() {
  float forward, turn;
  
  look();
  
{
    // follow line
    // go forward...
    forward = FORWARD_SPEED;
    Serial.print("Go forward");
    Serial.println();
    
    // ...and steer
    turn = 0;
    if(!eye_sees_white(EYE_LEFT) || !eye_sees_white(EYE_FAR_LEFT)) {
      // left eye sees black. turn left
      Serial.print("Left eye/s sees black. Turn left.");
      Serial.println();
      turn+=TURN_SPEED;
    }
    if(!eye_sees_white(EYE_RIGHT) || !eye_sees_white(EYE_FAR_RIGHT)) {
      turn-=TURN_SPEED;
      Serial.print("Right eye/s see white. Turn.");
      Serial.println();
    }
  }
  // if all eyes see black, no turn happens
  steer(forward,turn);
}

//------------------------------------------------------------------------------
void look() {
  // read the amount of light hitting each sensor
  int i;
  for(i=0;i<NUM_EYES;++i) {
    inputs[i] = analogRead(A1+i);
#if VERBOSE > 2
    Serial.print(inputs[i]);
    Serial.print('\t');
#endif
  }
#if VERBOSE > 2
  Serial.print('\n');
#endif
}


//------------------------------------------------------------------------------
void steer(float forward,float turn) {
  int a = LEFT_NO_MOVE - forward + turn;
  int b = RIGHT_NO_MOVE + forward + turn;
  
  if(a<10) a=10;  if(a>170) a=170;
  if(b<10) b=10;  if(b>170) b=170;
  
  left.write(a);
  right.write(b);
}
