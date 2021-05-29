/*!
 * @file program_remote_control.ino
 * @author Michał Dołharz
*/

#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

// Defines
#define LED_GREEN 6  //!< A green led vcc pin.
#define LED_YELLOW 7 //!< A yellow led vcc pin.
#define LED_RED 8    //!< A red led vcc pin.
#define CE 9         //!< A nRF24L01 CK (chip enable, transfer/receive) pin.
#define CSN 10       //!< A nRF24L01 CSN (chip select, send/read SPI commands) pin.
#define X_AXIS 14    //!< A joystick X axis pin.
#define Y_AXIS 15    //!< A joystick Y axis pin.
#define SWITCH 16    //!< A joystick switch pin.

// Variables
const uint64_t pipe = 0x8392b5ac58LL; //!< A pipe address, same as in the robot program.
RF24 radio(CE, CSN);                  //!< A RF24 object, communicates with an other nRF24L01 device.
int data[2];                          //!< A data to be transfered to an other device.
bool sensorStatus = true;             //!< A status of sensor. Active or inactive. Red LED. Currenty not used.
bool remoteStatus = true;             //!< A status of remote control mode. Green LED. Active of inactive.
bool otherStatus = false;             //!< An other status. Yellow LED. Currently not used.
int menuFlag = 0;                     //!< A flag of current menu option. Currently not used.

/*!
 * @brief Setup funtion.
 */
void setup()
{

    // Set pin modes
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(SWITCH, INPUT_PULLUP);

    Serial.begin(9600);

    // Configure nRF24L01.
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);   // Devices are close, otherwise PA_MAX or PA_HIGH.
    radio.setDataRate(RF24_250KBPS); // Lowest transmission speed.
    radio.openWritingPipe(pipe);     // Channel one.

    //digitalWrite(LED_GREEN, HIGH);  // Power ON information.
    digitalWrite(LED_RED, HIGH); // Power ON information.
    //digitalWrite(LED_YELLOW, HIGH); // Power ON information.
    //delay(200);
    //digitalWrite(LED_GREEN, LOW);  // Power ON information.
    //digitalWrite(LED_RED, LOW);    // Power ON information.
    //digitalWrite(LED_YELLOW, LOW); // Power ON information.
}

/*!
 * @brief Never ending loop.
 */
void loop()
{
    //static bool led = false;
    static bool sw = false;
    static int x = 0;
    static int y = 0;
    static unsigned long time;
    static unsigned long timeSaved;

    data[0] = analogRead(X_AXIS);
    data[1] = analogRead(Y_AXIS);

    radio.write(data, sizeof(data));
    delay(100);

    // Check joystick switch
    sw = getSwitchState();

    // If the joystick switch is pressed
    if (sw == true)
    {
        // Simple vibration filtration
        delay(50);

        // Check again if switch is pressed.
        sw = getSwitchState();
        if (sw == true)
        {
            sensorStatus = !sensorStatus;
            data[0] = -1;           // Value that robot recognize as mode change.
            data[1] = sensorStatus; // Mode flag change.

            radio.write(data, sizeof(data));
            digitalWrite(LED_GREEN, sensorStatus);

            // Make sure the button is unpressed.
            while (sw == true)
                sw = getSwitchState();
            /*
            //timeSaved = millis();
            while (sw == false)
            {
                x = analogRead(X_AXIS);
                y = analogRead(Y_AXIS);
                if(x <= 100)
                {
                  menuFlag++;
                  if(menuFlag > 2)
                    menuFlag = 2;
                  while(x <= 100)
                    x = analogRead(X_AXIS);
                }
                else if(x >= 900)
                {
                  menuFlag--;
                  if(menuFlag < 0)
                    menuFlag = 0;
                  while(x >= 900)
                    x = analogRead(X_AXIS);
                }

              if(menuFlag == 0)
                digitalWrite(LED_RED, HIGH);
              if(menuFlag == 1)
                digitalWrite(LED_YELLOW, HIGH);
              if(menuFlag == 2)
                digitalWrite(LED_GREEN, HIGH);

              
                // Check if led diodes state change is necessary
                if (millis() - timeSaved >= 250UL)
                {
                    led = !led; // state change
                    if(menuFlag != 0)
                      digitalWrite(LED_RED, led);
                    if(menuFlag != 1)
                      digitalWrite(LED_YELLOW, led);
                    if(menuFlag != 2)
                      digitalWrite(LED_GREEN, led);

                    timeSaved = millis();
                }

                // Exit settings
                sw = getSwitchState();
                if (sw == true)
                {
                    delay(50);
                    sw = getSwitchState(); // if sw == true, then it is the end of while() loop
                }
            }

            // Make sure the button is unpressed.
            while (sw == true)
                sw = getSwitchState();

            // Setio diodes to low.
            digitalWrite(LED_RED, LOW);
            digitalWrite(LED_YELLOW, LOW);
            digitalWrite(LED_GREEN, LOW);
            led = false;
            */
        }
    }
}

bool getSwitchState()
{
    return !digitalRead(SWITCH);
}
