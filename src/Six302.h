
/* Reach out to almonds@mit.edu or jodalyst@mit.edu for help */

#ifndef _Six302_H_
#define _Six302_H_

/* CHOOSE ONE: */

#define S302_SERIAL
//#define S302_WEBSOCKETS

#if ( defined S302_SERIAL &&  defined S302_WEBSOCKETS) || \
    (!defined S302_SERIAL && !defined S302_WEBSOCKETS)
#error "Choose a communication setup"
#endif

#if defined (S302_WEBSOCKETS) && !defined (ESP32) && !defined (ESP8266)
#error "Communication over WebSockets is only available for the ESP32 or ESP8266."
#endif

/* #includes */

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#if defined S302_WEBSOCKETS
#include <WiFi.h>
#include <WebSocketsServer.h>
using namespace std::placeholders;
#endif

/* Sugar */

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

#if defined (ESP32) || (ESP8266) || (TEENSYDUINO)
#else
#define S302_UNO
#endif

/* Memory constraints */

#if defined S302_UNO

// ARDUINO UNO:

         #define MAX_CONTROLS  5 // Joystick counts as two controls btw
         #define MAX_REPORTERS 5
         #define MAX_TALLY     5

         #define MAX_TITLE_LEN 10
         #define MAX_DEBUG_LEN 500

         #define MAX_PREC      7

#else

// All else:

         #define MAX_CONTROLS  20
         #define MAX_REPORTERS 10
         #define MAX_TALLY     10

         #define MAX_TITLE_LEN 10
         #define MAX_DEBUG_LEN 1000

#endif

// (conservative calculations:)
#define MAX_BUFFER_LEN (2+4*MAX_REPORTERS+1+1)
#define MAX_BUILD_STRING_LEN (2+MAX_CONTROLS*(8+MAX_TITLE_LEN+3*24+5)+1)

/* Class definition! */

class CommManager {

   /* PUBLIC */

   public:

      /* Initialization */

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

      /* To add controls: */

      bool addToggle(
         bool* linker,
         const char* title);

      bool addButton(
         bool* linker,
         const char* title);

#if defined S302_UNO
      bool addSlider(
         float* linker,
         const char* title,
         float range_min, float range_max,
         float resolution,
         bool toggle=false);
#else
      bool addSlider(
         float* linker,
         const char* title,
         std::initializer_list<float> range,
         float resolution,
         bool toggle=false);
#endif

#if defined S302_UNO
      bool addJoystick(
         float* linker_x,
         float* linker_y,
         const char* title,
         float xrange_min, float xrange_max,
         float yrange_min, float yrange_max,
         float resolution,
         bool sticky=true);
#else
      bool addJoystick(
         float* linker_x,
         float* linker_y,
         const char* title,
         std::initializer_list<float> xrange,
         std::initializer_list<float> yrange,
         float resolution,
         bool sticky=true);
#endif

      /* To add reporters: */

#if defined S302_UNO
      bool addPlot(
         float* linker,
         const char* title,
         float yrange_min, float yrange_max,
         uint8_t steps_displayed=10,
         uint8_t tally=1,
         uint8_t num_plots=1);
#else
      bool addPlot(
         float* linker,
         const char* title,
         std::initializer_list<float> yrange,
         uint8_t steps_displayed=10,
         uint8_t tally=1,
         uint8_t num_plots=1);
#endif

      bool addNumber(
         int32_t* linker,
         const char* title,
         uint8_t tally=1);

      bool addNumber(
         float* linker,
         const char* title,
         uint8_t tally=1);

      /* Tick */

      void step();
#if defined ESP32
      void _step();
#endif
      uint32_t headroom();

      /* Other */
      
      void debug(char*);
      //void debug(String);
      //void debug(bool);
      //void debug(int);
      //void debug(float);
      //void debug(double);

   /* PRIVATE */

   protected:

      /* Most important buffers */

      char    _buf[MAX_BUFFER_LEN]; // long general buffer
      char    _build_string[MAX_BUILD_STRING_LEN];
      char    _debug_string[MAX_DEBUG_LEN];
#if defined S302_UNO
      char    _tmp[24]; // short general buffer
#endif

      /* Tally mechanic */

      uint8_t _recordings[MAX_REPORTERS][MAX_TALLY][4];
      uint8_t _tallies[MAX_REPORTERS];

      /* Links */

      float*  _controls[MAX_CONTROLS];   uint8_t _total_controls;
      float*  _reporters[MAX_REPORTERS]; uint8_t _total_reporters;

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
         uint8_t num, WStype_t type,
         uint8_t* payload, size_t length);
#endif
      
      /* Timing */

      uint32_t _step_period;
      uint32_t _report_period;
      int32_t  _headroom;    // headroom for the last step
      float    _headroom_rp; // lowest headroom over the last report period

#if defined TEENSYDUINO
      elapsedMicros _main_timer;
      elapsedMicros _report_timer;
#else
      uint32_t _main_timer;
      uint32_t _report_timer;
#endif
#if defined ESP32
      uint32_t _secondary_timer;
#endif

      /* Semaphore handle for the ESP32 */

#if defined ESP32
      SemaphoreHandle_t _baton;
#endif
      
      /* Routines */

      void _control();
      void _record(uint8_t reporter);
      void _report();
      bool _time_to_talk(uint32_t time_to_wait);
      void _wait();
      
      void _NOT_IMPLEMENTED_YET();

      // ESP32 dual core
#if defined ESP32
      TaskHandle_t _six302_task;
      static void _walk(void* param);
#endif

};

#endif