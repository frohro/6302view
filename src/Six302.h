#ifndef _Six302_H_
#define _Six302_H_

/* Only have one of these uncommented! */

#define S302_SERIAL
//#define S302_WEBSOCKETS

#if defined (S302_WEBSOCKETS) && !defined (ESP32) && !defined (ESP8266)
#error "Communication over WebSockets is only available for the ESP32 or ESP8266."
#endif

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#if defined S302_WEBSOCKETS
#include <WiFi.h>
#include <WebSocketsServer.h>
using namespace std::placeholders;
#endif

#if defined ESP32
#define TAKE xSemaphoreTake(_baton, portMAX_DELAY);
#define GIVE xSemaphoreGive(_baton);
#else
#define TAKE
#define GIVE
#endif

#if defined S302_SERIAL
#define BROADCAST(msg, len) _serial->write((uint8_t*)msg, len)
#elif defined S302_WEBSOCKETS
#define BROADCAST(msg, len) _wss.broadcastBIN((uint8_t*)msg, len)
#endif

/* ARBITRARY LIMITS
   Eventually these might dynamically change according to the MC */

// How many elements you can add
// Joystick counts as two controls!
#define MAX_CONTROLS  20
#define MAX_REPORTERS 10
#define MAX_TALLY     10

// Max length of titles of controls & reporters
#define MAX_TITLE_LEN  10
#define MAX_BUFFER_LEN (2+4*MAX_REPORTERS+1+1)

// Worst case is when we have 20 slider controls with long titles
#define MAX_BUILD_STRING_LEN (2+MAX_CONTROLS*(8+MAX_TITLE_LEN+3*24+5)+1)

// Length of debug messages **per report period**
// Increase if your messages are getting cut off and you have space left
#define MAX_DEBUG_LEN 1000

/* Class definition! */

class CommManager {
   public:

      // Initialization
      CommManager(uint32_t sp=1000, uint32_t rp=20000);
      
#if defined S302_SERIAL
#if defined TEENSYDUINO
      void connect(usb_serial_class* s, uint32_t baud);
#else
      void connect(HardwareSerial* s, uint32_t baud);
#endif
#elif defined S302_WEBSOCKETS
      void connect(char* ssid, char* pw);
#endif

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
         std::initializer_list<float> range,
         float resolution,
         bool toggle=false);
      bool addJoystick(
         float* linker_x,
         float* linker_y,
         const char* title,
         std::initializer_list<float> xrange,
         std::initializer_list<float> yrange,
         float resolution,
         bool sticky=true);

      // Add reporters:
      bool addPlot(
         float* linker,
         const char* title,
         std::initializer_list<float> yrange,
         uint8_t steps_displayed=10,
         uint8_t tally=1,
         uint8_t num_plots=1);
      bool addNumber(int32_t* linker, const char* title, uint8_t tally=1);
      bool addNumber(float* linker, const char* title, uint8_t tally=1);

      // Tick
      void step();

      // Other
      
      uint32_t headroom();

      void debug(char*);
      //void debug(String);
      //void debug(bool);
      //void debug(int);
      //void debug(float);
      //void debug(double);

   protected:
      char _build_string[MAX_BUILD_STRING_LEN];
      char _buf[MAX_BUFFER_LEN];
      uint8_t _recordings[MAX_REPORTERS][MAX_TALLY][4];
      
#if defined S302_SERIAL
#if defined TEENSYDUINO
      usb_serial_class* _serial;
#else
      HardwareSerial* _serial;
#endif
      uint32_t _baud;
#elif defined S302_WEBSOCKETS
      WebSocketsServer _wss = WebSocketsServer(80);
      void _on_websocket_event(
         uint8_t num,
         WStype_t type,
         uint8_t* payload,
         size_t length);
#endif
      
      float* _controls[MAX_CONTROLS];   uint8_t _total_controls;
      float* _reporters[MAX_REPORTERS]; uint8_t _total_reporters;

      // Timing variables
      uint32_t _step_period;
      uint32_t _report_period;
      int32_t _headroom;
      uint8_t _tallies[MAX_REPORTERS];
#if defined TEENSYDUINO
      elapsedMicros _main_timer;
      elapsedMicros _report_timer;
#elif defined (ESP32) || (ESP8266)
      uint32_t _main_timer;
      uint32_t _report_timer;
#endif

      // Semaphore handle for the ESP32
#if defined ESP32
      SemaphoreHandle_t _baton;
#endif
      
      // Routines
      void _control();
      void _record(uint8_t reporter);
      void _report();
      bool _time_to_talk(uint32_t time_to_wait);
      void _wait();

      // Debug messages
      char _debug_string[MAX_DEBUG_LEN];
      
      void _NOT_IMPLEMENTED_YET();

      // ESP32 dual core
#if defined ESP32
      TaskHandle_t _six302_task;
      static void _step_forever(void* params);
#endif

};

#endif