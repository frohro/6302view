#ifndef _Six302_H_
#define _Six302_H_

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

/* Arbitrary limits */

// How many elements you can add
// Joystick counts as two controls
#define MAX_CONTROLS      10
#define MAX_REPORTERS     10

// Max length of titles of controls & reporters
#define MAX_TITLE_LENGTH  10

// Incoming and outgoing message length limits
#define MAX_IN_LEN 28 /* (id)(:)(float)(\n) = 2+1+24+1 = 28 */
#define MAX_OUT_LEN (1+4*MAX_REPORTERS+1)

#define MAX_BUILD_STRING_LEN (MAX_CONTROLS*(8+MAX_TITLE_LENGTH+3*24+5))
// 2340, last time I checked
// Worst case is when we have 20 slider controls with long titles

// Communication system setups
#define S302_SERIAL       0
#define S302_WEBSOCKETS   1

// States
#define IDLE 0
#define S302_DISCONNECTED 0
#define S302_LISTEN       1
#define S302_TALK         2

class CommManager {
   public:
      // Main things
      CommManager(uint32_t sp=1000, uint32_t rp=20000);

      /* Serial */
      #if defined TEENSYDUINO
         void connect(usb_serial_class* s, uint32_t baud);
      #else
         void connect(HardwareSerial* s, uint32_t baud);
      #endif
      /* Pure wifi */
      void connect(char* ssid, char* pw);
      void step();
      uint32_t headroom();

      // Add controls:
      bool addToggle(
         bool* linker,
         const char* title);
      bool addButton(
         bool* linker,
         const char* title);
      bool addSlider(
         float* linker,
         const char* title,
         float range_low, float range_high,
         float resolution,
         bool toggle=false);
      bool addJoystick(
         float* linker_x,
         float* linker_y,
         const char* title,
         float xrange_low, float xrange_high,
         float yrange_low, float yrange_high,
         float resolution,
         bool sticky=true);

      // Add reporters:
      bool addPlot(
         float* linker,
         const char* title,
         float yrange_low, float yrange_high,
         int steps_displayed=10,
         int num_plots=1);
      bool addNumber(int32_t* linker, const char* title);
      bool addNumber(float* linker, const char* title);

      // Send debugging info
      void debug(char*);
      //void debug(String);
      //void debug(bool);
      //void debug(int);
      //void debug(float);
      //void debug(double);

   protected:
      void _control();
      void _report();
      void _wait_for_connection();
      bool _time_to_talk();
      void _wait();

      int32_t _headroom;

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

      uint8_t _state;

      // Serial
      #if defined TEENSYDUINO
         usb_serial_class* _serial;
      #else
         HardwareSerial* _serial;
      #endif
      uint32_t _baud;
      
      // Websockets
      void _NOT_IMPLEMENTED_YET();

      // Variables for timing
      #if defined TEENSYDUINO
         elapsedMicros _main_timer;
         elapsedMicros _report_timer;
      #elif defined (ESP32) || (ESP8266)
         uint64_t _main_timer;
         uint64_t _report_timer;
      #else // I'm assuming it's an Arduino Uno
      #endif
      
};

#endif
