/*!
 * @file program_robot.ino
 * @author Michał Dołharz
*/

#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <HCSR04.h>

// Defines
#define TRIG 2             //!< A TRIG pin of sensor.
#define ECHO 3             //!< An ECHO pin of sensor.
#define M_LEFT_BACKWARD 4  //!< A "backward" pin of left motor.
#define M_LEFT_FORWARD 5   //!< A "forward" pin of left motor.
#define PWM 6              //!< Motors PWM pin.
#define M_RIGHT_BACKWARD 7 //!< A "backward" pin of right motor.
#define M_RIGHT_FORWARD 8  //!< A "forward" pin of right motor.
#define CE 9               //!< A CK (chip enable, transfer/receive) pin of nRF24L01.
#define CSN 10             //!< A CSN (chip select, send/read SPI commands) pin of nRF24L01.
#define MOTORS_ON_OFF 14   //!< Motors on/off switch pin
#define STOP_RANGE 15      //!< A distance (in centimeters) from an obstacle for which the robot will stop.

/*!
 * @brief A motor mode type, based on a truth table of L293 integrated circuit.
 */
enum class motorMode
{
    forward,   //!< Forward movement.
    backward,  //!< Backward movement.
    stop,      //!< Motor off.
    strongStop //!< Electric braking.
};

/*!
 * @brief A motor movement state type, necessary to set motors modes only when a change is needed. 
 */
enum class movementState
{
    forward,            //!< Forward movement.
    backward,           //!< Backward movement.
    stop,               //!< Motor off.
    strongStop,         //!< Electric braking.
    turnLeftIP,         //!< Left turn in place.
    turnRightIP,        //!< Right run in place.
    turnLeftOWForward,  //!< Left one-wheel turn forward.
    turnLeftOWBackward, //!< Left one-wheel turn backward.
    turnRightOWForward, //!< Right one-wheel turn forward.
    turnRightOWBackward //!< Right one-wheel turn backward.
};

/*!
 * @brief A joystick state type. 
 */
enum class joystickState
{
    forward,  //!< y axis high (>900, where max=1023).
    backward, //!< y axis low (<100, where min=0).
    center,   //!< both axes at their center (~512)
    left,     //!< x axis high (>900, where max=1023).
    right     //!< x axis low (<100, where min=0).
};

// Variables

RF24 radio(CE, CSN);                                //!< A RF24 object, communicates with an other nRF24L01 device.
HCSR04 sensor(TRIG, ECHO);                          //!< A HC-SR04 sensor.
enum movementState direction = movementState::stop; //!< A state of current movement direction/type (might be turn).
bool sensorBlockade = 0;                            //!< A flag to be set true if sensor detects an obstacle.
bool safetyFlag = true; //!< programmably set to false for UART debugging. Motors will be deactivated, so the current won't damge computer.
bool flag = true;
bool need = false;
/*!
 * @brief Setup funtion.
 */
void setup()
{

    // Set pin modes
    pinMode(MOTORS_ON_OFF, INPUT_PULLUP);
    pinMode(PWM, OUTPUT);
    pinMode(M_LEFT_BACKWARD, OUTPUT);
    pinMode(M_LEFT_FORWARD, OUTPUT);
    pinMode(M_RIGHT_BACKWARD, OUTPUT);
    pinMode(M_RIGHT_FORWARD, OUTPUT);

    Serial.begin(9600);

    // Configure nRF24L01.
    const uint64_t pipe = 0x8392b5ac58LL; //!< A pipe address, same as in a remote control program.
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);   // devices are close, otherwise PA_MAX or PA_HIGH
    radio.setDataRate(RF24_250KBPS); // lowest transmission speed
    radio.openReadingPipe(1, pipe);  // channel one
    radio.startListening();

    //delay(4000); // some time to safely walk away from a robot

    analogWrite(PWM, 255);
}

/*!
 * @brief Never ending loop.
 */
void loop()
{
    static int data[2];  //!< A data to be received from an other device.
    static int distance; //!< A distance measured by a sensor.
    static enum joystickState x = joystickState::center; //!< A state of joystick x axis.
    static enum joystickState y = joystickState::center; //!< A state of joystick y axis.

    // Check if there is available data to read.
    if (motorsON() == true)
    {

        distance = sensor.dist();

        // Check if robot is in stop range. Stop motors if necessary.
        checkDistance(distance);

        // Check if nRF24L01 received any data
        if (radio.available() && safetyFlag == true)
        {
            // Read data.
            radio.read(data, sizeof(data));

            // Print data.
            //printJoystick(data[0], data[1]);

            if(data[0] == -1)
            {
              flag = data[1];
            }
            else if(flag == true)
            {
              // Check axes.
              x = checkXAxis(data[0]);
              y = checkYAxis(data[1]);
              need = true;

              // Change movement or keep the same without overwrite.
              movementChange(x, y);
            }
        }

        // Remote control mode.
        if(flag == true)
        {
          need = false;
          movementChange(x, y);
        }
        // Autonomous mode.
        else
        {
          while(!radio.available())
          {
            wander();
          }
          move(motorMode::stop);
        }
    }
}

/*!
 * @brief Wandering mode. Robot will be avoiding obstacles and wander around.
 */
void wander()
{
  unsigned long timeSaved = millis();
  unsigned long turningTime;;
  int turnType;
  int distance;

  move(motorMode::forward);

  distance = sensor.dist();

  if (distance <= STOP_RANGE)
  {
    move(motorMode::stop);

    turningTime = random(500, 2500); // random turn time
    turnType = random(0, 3); // random turn type

    switch(turnType)
    {
      case 0:
        turnRightIP();
        break;
      case 1:
        turnLeftIP();
        break;
      case 2:
        turnRightOW(motorMode::backward);
        break;
      case 3:
        turnLeftOW(motorMode::backward);
        break;
    }

    while(millis() - timeSaved <= turningTime)
    {    } // turn time

  }
}

/*!
 * @brief Checks distance measured by a sensor. If necessary, stops motors and sets a flag.
 * @param[in] distance Distance measured by a sensor.
 */
void checkDistance(int distance)
{
          if (distance <= STOP_RANGE)
        {
            sensorBlockade = true;

            // Stop robot if going forward
            if (direction == movementState::forward || direction == movementState::turnLeftOWForward || direction == movementState::turnRightOWForward)
            {
                move(motorMode::stop);
                direction = movementState::stop;
            }
        }
        else
        {
            sensorBlockade = false;
        }
}

/*!
 * @brief Checks data of y axis from joystick and returns joystick state type value.
 * @param[in] data A raw data of y axis from joystick.
 * @return Return joystickState value.
 */
enum joystickState checkYAxis(int data)
{
    if (data > 900)
        return joystickState::forward;
    else if (data < 100)
        return joystickState::backward;
    else
        return joystickState::center;
}

/*!
 * @brief Checks data of x axis from joystick and returns joystick state type value.
 * @param[in] data A raw data of x axis from joystick.
 * @return Return joystickState value.
 */
enum joystickState checkXAxis(int data)
{
    // Check x axis.
    if (data > 900)
        return joystickState::left;
    else if (data < 100)
        return joystickState::right;
    else
        return joystickState::center;
}

/*!
 * @brief A function that determines movement based on x and y joystickState values. There is nine different possibilities.
 * @param[in] mode A movement type to be set.
 */
void movementChange(enum joystickState x, enum joystickState y)
{
    // forward
    if (y == joystickState::forward && x == joystickState::center && direction != movementState::forward && sensorBlockade == false)
    {
        move(motorMode::forward);
        direction = movementState::forward;
    }
    // backward
    else if (y == joystickState::backward && x == joystickState::center && direction != movementState::backward)
    {
        move(motorMode::backward);
        direction = movementState::backward;
    }
    // stop
    else if (y == joystickState::center && x == joystickState::center && direction != movementState::stop)
    {
        move(motorMode::stop);
        direction = movementState::stop;
    }
    // turn left in place
    else if (y == joystickState::center && x == joystickState::right && direction != movementState::turnRightIP)
    {
        turnRightIP();
        direction = movementState::turnRightIP;
    }
    // turn right in place
    else if (y == joystickState::center && x == joystickState::left && direction != movementState::turnLeftIP)
    {
        turnLeftIP();
        direction = movementState::turnLeftIP;
    }
    // turn left one wheel forward
    else if (y == joystickState::forward && x == joystickState::left && direction != movementState::turnLeftOWForward && sensorBlockade == false)
    {
        turnLeftOW(motorMode::forward);
        direction = movementState::turnLeftOWForward;
    }
    // turn left one wheel backward
    else if (y == joystickState::backward && x == joystickState::left && direction != movementState::turnLeftOWBackward)
    {
        turnLeftOW(motorMode::backward);
        direction = movementState::turnLeftOWBackward;
    }
    // turn right one wheel forward
    else if (y == joystickState::forward && x == joystickState::right && direction != movementState::turnRightOWForward && sensorBlockade == false)
    {
        turnRightOW(motorMode::forward);
        direction = movementState::turnRightOWForward;
    }
    // turn right one wheel backward
    else if (y == joystickState::backward && x == joystickState::right && direction != movementState::turnRightOWBackward)
    {
        turnRightOW(motorMode::backward);
        direction = movementState::turnRightOWBackward;
    }
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
    leftMotor(motorMode::stop);
}

/*!
 * @brief A function to make an one wheel right turn.
 * @param[in] direction A direction in which the turn will be performed. 
 */
void turnRightOW(enum motorMode direction)
{
    leftMotor(direction);
    rightMotor(motorMode::stop);
}

/*!
 * @brief A function to make an in place (both wheels) left turn.
 */
void turnLeftIP()
{
    leftMotor(motorMode::backward);
    rightMotor(motorMode::forward);
}

/*!
 * @brief A function to make an in place (both wheels) right turn.
 */
void turnRightIP()
{
    leftMotor(motorMode::forward);
    rightMotor(motorMode::backward);
}

/*!
 * @brief A function to set a movement to a left motor.
 * @param[in] mode The movement type to be set.
 */
void leftMotor(enum motorMode mode)
{
    switch (mode)
    {
    case motorMode::forward:
        digitalWrite(M_LEFT_BACKWARD, LOW);
        digitalWrite(M_LEFT_FORWARD, HIGH);
        break;
    case motorMode::backward:
        digitalWrite(M_LEFT_BACKWARD, HIGH);
        digitalWrite(M_LEFT_FORWARD, LOW);
        break;
    case motorMode::stop:
        digitalWrite(M_LEFT_BACKWARD, LOW);
        digitalWrite(M_LEFT_FORWARD, LOW);
        break;
    case motorMode::strongStop:
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
    switch (mode)
    {
    case motorMode::forward:
        digitalWrite(M_RIGHT_BACKWARD, LOW);
        digitalWrite(M_RIGHT_FORWARD, HIGH);
        break;
    case motorMode::backward:
        digitalWrite(M_RIGHT_BACKWARD, HIGH);
        digitalWrite(M_RIGHT_FORWARD, LOW);
        break;
    case motorMode::stop:
        digitalWrite(M_RIGHT_BACKWARD, LOW);
        digitalWrite(M_RIGHT_FORWARD, LOW);
        break;
    case motorMode::strongStop:
        digitalWrite(M_RIGHT_BACKWARD, HIGH);
        digitalWrite(M_RIGHT_FORWARD, HIGH);
        break;
    }
}

/*!
 * @brief Prints values of x and y axes of joystick.
 * @param[in] x x axis value.
 * @param[in] y y axis value.
 */
void printJoystick(int x, int y)
{
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.println(y);
}

/*! 
 * @brief A function that checks a switch if motors can move.
 */
bool motorsON()
{
    if (digitalRead(MOTORS_ON_OFF))
    {
        return false;
    }
    else
    {
        return true;
    }
}
