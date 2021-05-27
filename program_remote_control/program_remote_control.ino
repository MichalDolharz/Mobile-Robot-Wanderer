/*!
 * @file program_remote_control.ino
 * @author Michał Dołharz
*/

#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

// Defines
#define LED_GREEN 6 //!< A green led vcc pin.
#define LED_YELLOW 7 //!< A yellow led vcc pin.
#define LED_RED 8 //!< A red led vcc pin.
#define CE 9 //!< A nRF24L01 CK (chip enable, transfer/receive) pin.
#define CSN 10 //!< A nRF24L01 CSN (chip select, send/read SPI commands) pin.
#define X_AXIS 14 //!< A joystick X axis pin.
#define Y_AXIS 15 //!< A joystick Y axis pin.
#define SWITCH 16 //!< A joystick switch pin.


// Variables
const uint64_t pipe = 0x8392b5ac58LL; //!< A pipe address, same as in the robot program.
RF24 radio(CE, CSN); //!< A RF24 object, communicates with an other nRF24L01 device.
int data[2]; //!< A data to be transfered to an other device.
int x_axis; //!< A value that holds a joystick x axis value.
int y_axis; //!< A value that holds a joystick y axis value.

/*!
 * @brief Setup funtion.
 */
void setup() {

  // Set pin modes
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  Serial.begin(9600);

  // Configure nRF24L01.
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // devices are close, otherwise PA_MAX or PA_HIGH
  radio.setDataRate(RF24_250KBPS); // lowest transmission speed
  radio.openWritingPipe(pipe); // channel one
}

/*!
 * @brief Never ending loop.
 */
void loop() {
  x_axis = analogRead(X_AXIS);
  y_axis = analogRead(Y_AXIS);

  data[0] = x_axis;
  data[1] = y_axis;

  radio.write(data, sizeof(data));
  delay(100);

  /*digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, HIGH);
  delay(200);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, HIGH);
  delay(200);
  digitalWrite(LED_RED, LOW);
  radio.write(&i, sizeof(i));
  Serial.println(i);
  i++;*/

}
