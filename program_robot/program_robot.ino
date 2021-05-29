/*!
 * @file program_robot.ino
 * @author Michał Dołharz
*/

#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

// Defines
#define M_LEFT_BACKWARD 4  //!< A left motor "backward" pin.
#define M_LEFT_FORWARD 5   //!< A left motor "forward" pin.
#define PWM 6              //!< Motors PWM pin.
#define M_RIGHT_BACKWARD 7 //!< A right motor "backward" pin.
#define M_RIGHT_FORWARD 8  //!< A right motor "forward" pin.
#define CE 9               //!< A nRF24L01 CK (chip enable, transfer/receive) pin.
#define CSN 10             //!< A nRF24L01 CSN (chip select, send/read SPI commands) pin.
#define MOTORS_ON_OFF 14   //!< Motors on/off switch pin

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
const uint64_t pipe = 0x8392b5ac58LL;               //!< A pipe address, same as in a remote control program.
RF24 radio(CE, CSN);                                //!< A RF24 object, communicates with an other nRF24L01 device.
int data[2];                                        //!< A data to be received from an other device.
bool flag = true;                                   //!< A flag to set motors on/off.
enum movementState direction = movementState::stop; //!< A state of current movement direction/type (might be turn).
enum joystickState x = joystickState::center;       //!< A state of joystick x axis.
enum joystickState y = joystickState::center;       //!< A state of joystick y axis.

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

    // Check if there is available data to read.
    if (motorsON() == true)
    {
        if (radio.available())
        {
            // Read data.
            radio.read(data, sizeof(data));

            // Print data.
            printJoystick(data[0], data[1]);

            // Check y axis.
            if (data[1] > 900)
                y = joystickState::forward;
            else if (data[1] < 100)
                y = joystickState::backward;
            else
                y = joystickState::center;

            // Check x axis.
            if (data[0] > 900)
                x = joystickState::left;
            else if (data[0] < 100)
                x = joystickState::right;
            else
                x = joystickState::center;

            // forward
            if (y == joystickState::forward && x == joystickState::center && direction != movementState::forward)
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
            else if (y == joystickState::forward && x == joystickState::left && direction != movementState::turnLeftOWForward)
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
            else if (y == joystickState::forward && x == joystickState::right && direction != movementState::turnRightOWForward)
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
