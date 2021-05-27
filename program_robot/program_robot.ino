/*!
 * @file program_robot.ino
 * @author Michał Dołharz
*/

#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

// Defines
#define M_LEFT_BACKWARD 4 //!< A left motor "backward" pin.
#define M_LEFT_FORWARD 5 //!< A left motor "forward" pin.
#define PWM 6 //!< Motors PWM pin. 
#define M_RIGHT_BACKWARD 7 //!< A right motor "backward" pin.
#define M_RIGHT_FORWARD 8 //!< A right motor "forward" pin.
#define CE 9 //!< A nRF24L01 CK (chip enable, transfer/receive) pin.
#define CSN 10 //!< A nRF24L01 CSN (chip select, send/read SPI commands) pin.
#define MOTORS_ON_OFF 14 //!< Motors on/off switch pin

// Variables
const uint64_t pipe = 0x8392b5ac58LL; //!< A pipe address, same as in a remote control program.
RF24 radio(CE, CSN); //!< A RF24 object, communicates with an other nRF24L01 device.
int data[2]; //!< A data to be received from an other device.
bool flag = true; //!< A flag to set motors on/off.

/*!
 * @brief A motor movement type, based, based on a truth table of L293 integrated circuit.
 */
enum motorMode {
  forward, //!< Forward movement.
  backward, //!< Backward movement.
  stop, //!< Motor off. 
  strongStop //!< Electric braking.
};

/*
enum turnSide {
  left,
  right
};

enum turnDirection {
  forward, 
  backward
};

enum turnMode {
  inPlace,
  oneWheel
};*/

/*!
 * @brief Setup funtion.
 */
void setup() {

  // Set pin modes
  pinMode(MOTORS_ON_OFF, INPUT_PULLUP);
  pinMode(PWM, OUTPUT);
  pinMode(M_LEFT_BACKWARD, OUTPUT);
  pinMode(M_LEFT_FORWARD, OUTPUT);
  pinMode(M_RIGHT_BACKWARD, OUTPUT);
  pinMode(M_RIGHT_FORWARD, OUTPUT);

  Serial.begin(9600);

  // Configure nRF24L01.
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // devices are close, otherwise PA_MAX or PA_HIGH
  radio.setDataRate(RF24_250KBPS); // lowest transmission speed
  radio.openReadingPipe(1, pipe); // channel one
  radio.startListening();

  delay(4000); // some time to safely walk away from a robot
}

/*!
 * @brief Never ending loop.
 */
void loop() {

  // Check if there is available data to read.
  if(radio.available())
  {
    // Read data.
    radio.read(data, sizeof(data));

    // Print data.
    Serial.print("X: ");
    Serial.print(data[0]);
    Serial.print(", Y: ");
    Serial.println(data[1]);
  }

  // If motors' switch is set to ON
  /*if(motorsON() == true && flag == true)
  {
    flag = false;

    //makePWMShow();

    makeShow(170);
    delay(150);
    makeShow(255);
  }
  else if(motorsON() == false && flag == false)
  {
    flag = true;
    move(stop);
  }*/
}
/*!
 * @brief A simple function to show a diffrence between a slowest mode and a fastest mode. Probably only fast mode will be used.
 */
void makePWMShow()
{
  analogWrite(PWM, 170);
  move(forward);
  delay(2000);
  analogWrite(PWM, 255);
  move(forward);
  delay(2000);
  move(stop);
}

/*!
 * @brief A simple function to show some of robot moves.
 * @param[in] pwm A speed to be set based on a PWM, where maximum is 255 and minimum is 170 (any lower value may not move a motor).
 */
void makeShow(int pwm)
{
  analogWrite(PWM, pwm);

  move(forward);
  delay_(4000);
  move(backward);
  delay_(4000);
  turnRightOW(forward);
  delay_(2000);
  turnRightOW(backward);
  delay_(2000);
  turnLeftIP(forward);
  delay_(3000);
  move(stop);
  delay(150);
  turnRightIP(forward);
  delay_(3000);
  move(stop);
}

/*!
 * @brief A simple function to make a delay before a direction change of a motor.
 * @param[in] time Time to wait before stoping the motor.
 */
void delay_(int time)
{
  delay(time);
  move(stop);
  delay(150);
}

/*!
 * @brief A function to set both motors the same movement.
 * @param[in] mode A movement type to be set.
 */
void move(enum motorMode mode)
{
  rightMotor(mode);
  leftMotor(mode);
}

/*!
 * @brief A function to make an one wheel left turn.
 * @param[in] direction A direction in which the turn will be performed. 
 */
void turnLeftOW(enum motorMode direction)
{
  rightMotor(direction);
}

/*!
 * @brief A function to make an one wheel right turn.
 * @param[in] direction A direction in which the turn will be performed. 
 */
void turnRightOW(enum motorMode direction)
{
  leftMotor(direction);
}

/*!
 * @brief A function to make an in place (both wheels) left turn.
 * @param[in] direction A direction in which the turn will be performed. 
 */
void turnLeftIP(enum motorMode direction)
{
  switch(direction)
  {
    case forward:
      leftMotor(backward);
      rightMotor(forward);
      break;
    case backward:
      leftMotor(forward);
      rightMotor(backward);
      break;
    case stop:
      return;
    case strongStop:
      return;
  }  
}

/*!
 * @brief A function to make an in place (both wheels) right turn.
 * @param[in] direction A direction in which the turn will be performed. 
 */
void turnRightIP(enum motorMode direction)
{
  switch(direction)
  {
    case forward:
      leftMotor(forward);
      rightMotor(backward);
      break;
    case backward:
      leftMotor(backward);
      rightMotor(forward);
      break;
    case stop:
      return;
    case strongStop:
      return;
  }  
}

/*!
 * @brief A function to set a movement to a left motor.
 * @param[in] mode The movement type to be set.
 */
void leftMotor(enum motorMode mode)
{
  switch(mode)
  {
    case forward:
      digitalWrite(M_LEFT_BACKWARD, LOW);
      digitalWrite(M_LEFT_FORWARD, HIGH);
      break;
    case backward:
      digitalWrite(M_LEFT_BACKWARD, HIGH);
      digitalWrite(M_LEFT_FORWARD, LOW);
      break;
    case stop:
      digitalWrite(M_LEFT_BACKWARD, LOW);
      digitalWrite(M_LEFT_FORWARD, LOW);
      break;
    case strongStop:
      digitalWrite(M_LEFT_BACKWARD, HIGH);
      digitalWrite(M_LEFT_FORWARD, HIGH);
      break;
  }
    
}

/*!
 * @brief A function to set a movement to a right motor.
 * @param[in] mode The movement type to be set.
 */
void rightMotor(enum motorMode mode)
{
  switch(mode)
  {
    case forward:
      digitalWrite(M_RIGHT_BACKWARD, LOW);
      digitalWrite(M_RIGHT_FORWARD, HIGH);
      break;
    case backward:
      digitalWrite(M_RIGHT_BACKWARD, HIGH);
      digitalWrite(M_RIGHT_FORWARD, LOW);
      break;
    case stop:
      digitalWrite(M_RIGHT_BACKWARD, LOW);
      digitalWrite(M_RIGHT_FORWARD, LOW);
      break;
    case strongStop:
      digitalWrite(M_RIGHT_BACKWARD, HIGH);
      digitalWrite(M_RIGHT_FORWARD, HIGH);
      break;
  }
}

/*! 
 * @brief A function that checks a switch if motors can move.
 */
bool motorsON()
{
  if(digitalRead(MOTORS_ON_OFF))
  {
    return false;
  }
  else{
    return true;
  }
}
