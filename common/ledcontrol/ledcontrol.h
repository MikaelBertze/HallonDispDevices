#include <Arduino.h>

class LedControl {
    
    public:

        LedControl(byte redPin, byte greenPin, byte bluePin, bool inverted)
        : GreenPin(greenPin), RedPin(redPin), BluePin(bluePin), _inverted(inverted)
        {

        }
        
        void SetGreen(bool state) {
            digitalWrite(GreenPin, state^_inverted ? HIGH : LOW);
        }
        void SetRed(bool state) {
            digitalWrite(RedPin, state^_inverted ? HIGH : LOW);
        }
        void SetBlue(bool state) {
            digitalWrite(BluePin, state^_inverted ? HIGH : LOW);
        }
        void ToggleGreen() {
            digitalWrite(GreenPin, !digitalRead(GreenPin));
        }
        void ToggleRed() {
            digitalWrite(RedPin, !digitalRead(RedPin));
        }
        void ToggleBlue() {
            digitalWrite(BluePin, !digitalRead(BluePin));
        }
        
        void InitLeds() {
            pinMode(RedPin, OUTPUT);
            pinMode(BluePin, OUTPUT);
            pinMode(GreenPin, OUTPUT);
            SetGreen(false);
            SetRed(false);
            SetBlue(false);
        }

        void ledTest() {
            SetGreen(false);
            SetRed(false);
            SetBlue(false);
            for (byte i = 0; i < 5; i++) {
                SetGreen(true);
                delay(100);
                SetGreen(false);
                delay(100);
            }
            for (byte i = 0; i < 5; i++) {
                SetRed(true);
                delay(100);
                SetRed(false);
                delay(100);
            }
            for (byte i = 0; i < 5; i++) {
                SetBlue(true);
                delay(100);
                SetBlue(false);
                delay(100);
            }

        }

        void RebootSignal() {
            
            SetGreen(false);
            SetRed(false);
            SetBlue(false);

            for(byte i = 0; i < 20; i++) {
                ToggleBlue();
                ToggleRed();
                ToggleGreen();
                delay(200);
            }

            SetGreen(false);
            SetRed(false);
            SetBlue(false);
        }
    private:
        byte GreenPin;
        byte RedPin;
        byte BluePin;
        bool _inverted;
        
};
