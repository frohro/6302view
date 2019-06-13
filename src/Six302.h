#ifndef _Six302_H_
#define _Six302_H_

#include "Arduino.h"

#include <stdlib.h>
#include <string.h>

/* Arbitrary limits */

// How many elements you can add
// Joystick counts as two controls
#define MAX_CONTROLS      20
#define MAX_REPORTERS     10

// Max length of titles of controls & reporters
#define MAX_TITLE_LENGTH  32

// Incoming and outgoing message length limits
#define MAX_IN_LEN 28 /* (id)(:)(float)(\n) = 2+1+24+1 = 28 */
#define MAX_OUT_LEN (1+4*MAX_REPORTERS+1)

#define MAX_BUILD_STRING_LEN (MAX_CONTROLS*(8+MAX_TITLE_LENGTH+3*24+5))
// 2340, last time I checked
// Worst case is when we have 20 slider controls with long titles

// Communication system setups
#define S302_SERIAL       0
#define S302_WEBSOCKETS   1

// Use macros to determine what board we're working with

#define THIS_BOARD S302_ARDUINO_UNO // default assumption
#ifdef ESP8266
   #undef THIS_BOARD
   #define THIS_BOARD S302_ESP8266
#endif
#ifdef ESP32
   #undef THIS_BOARD
   #define THIS_BOARD S302_ESP32
#endif
#ifdef TEENSYDUINO
   #undef THIS_BOARD
   #define THIS_BOARD S302_TEENSY
#endif

// States
#define IDLE 0

class CommManager {
   public:
      // Main things
      CommManager(uint32_t sp=1000, uint32_t rp=20000);
      bool connect(usb_serial_class* s, uint32_t baud); // Serial
      bool connect(char* ssid, char* pw); // Pure wifi
      bool step();

      // Add controls:
      bool addToggle(
         bool* linker,
         char* title);
      bool addButton(
         bool* linker,
         char* title);
      bool addSlider(
         float* linker,
         char* title,
         std::initializer_list<float> range,
         float resolution,
         bool toggle=false);
      bool addJoystick(
         float* linker_x,
         float* linker_y,
         char* title,
         std::initializer_list<float> xrange,
         std::initializer_list<float> yrange,
         float resolution,
         bool sticky=true);

      // Add reporters:
      bool addPlot(
         float* linker,
         char* title,
         std::initializer_list<float> yrange,
         int steps_displayed=10,
         int num_plots=1);
      bool addNumber(int32_t* linker, char* title);
      bool addNumber(float* linker, char* title);

      // Send debugging info
      void debug(const char*);
      //void debug(char*);
      //void debug(String);
      //void debug(bool);
      //void debug(int);
      //void debug(float);
      //void debug(double);

   protected:
      uint32_t _wait();
      void _control();
      void _report();
      bool _build();

      uint8_t _architecture;

      uint32_t _step_period;
      uint32_t _report_period;

      float* _controls[MAX_CONTROLS];
      float* _reporters[MAX_REPORTERS];
      uint8_t _total_controls;
      uint8_t _total_reporters;

      char _build_string[MAX_BUILD_STRING_LEN];

      char _incoming[MAX_IN_LEN];
      char _outgoing[MAX_OUT_LEN];

      bool _connected;

      // Serial
      usb_serial_class* _serial;
      uint32_t _baud;
      
      // Websockets
      void _NOT_IMPLEMENTED_YET();

      // Variables for timing
      #if defined TEENSYDUINO
         elapsedMicros _timer;
      #elif defined ESP32
         uint32_t _timer;
      #elif defined ESP8266
      #else // I'm assuming it's an Arduino Uno
      #endif

      uint32_t _headroom;
};

#endif
