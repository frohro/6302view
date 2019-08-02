#include "Six302.h"

CommManager::CommManager(uint32_t sp, uint32_t rp) {
   _step_period = sp;
   _report_period = rp;
#if defined S302_WEBSOCKETS
   _wss = WebSocketsServer(80);
#endif
   strcpy(_build_string, "\fB");
   strcpy(_debug_string, "\fD");
}

#if defined S302_SERIAL
#if defined TEENSYDUINO
void CommManager::connect(usb_serial_class* s, uint32_t baud)
#else
void CommManager::connect(HardwareSerial* s, uint32_t baud)
#endif
{
   _serial = s;
   _baud = baud;
   _serial->begin(baud);
   while(!_serial); // does this even work?
   while( _serial->available() )
      _serial->read();
#elif defined S302_WEBSOCKETS
void CommManager::connect(char* ssid, char* pw) {
   // Serial should be ready to go
   Serial.printf("Connecting to %s WiFi ", ssid);
   WiFi.begin(ssid, pw);
   while( WiFi.status() != WL_CONNECTED ) {
      Serial.print('.');
      delay(1000);
   }
   Serial.println(" connected!");
   // Print my IP!
   IPAddress ip = WiFi.localIP();
   Serial.printf("--> %d.%d.%d.%d <--\n", ip[0], ip[1], ip[2], ip[3]);
   // Start the WebSocket server
   _wss.begin();
   _wss.onEvent(std::bind(&CommManager::_on_websocket_event, this, _1, _2, _3, _4));
#endif
#if defined ESP32
   _baton = xSemaphoreCreateMutex();
   disableCore0WDT();
   xTaskCreatePinnedToCore( this->_step_forever,
                            "6302view",
                            10000, /* Stack size, in words, can probably be decreased */
                            this,
                            0,
                            &_six302_task,
                            0 );
#endif
}

/* Controls */

bool CommManager::addToggle(bool* linker, const char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LEN )
      return false;
      
   _controls[_total_controls++] = (float*)linker;
   sprintf(_buf, "T\r%s\r", title);
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addButton(bool* linker, const char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LEN )
      return false;
      
   _controls[_total_controls++] = (float*)linker;
   sprintf(_buf, "B\r%s\r", title);
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addSlider(float* linker, const char* title,
                            std::initializer_list<float> range,
                            float resolution, bool toggle) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LEN )
      return false;
      
   _controls[_total_controls++] = linker;
   sprintf(_buf, "S\r%s\r%f\r%f\r%f\r%s\r",
      title, *range.begin(), *(range.begin()+1),
      resolution, toggle? "True":"False");
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addJoystick(float* linker_x, float* linker_y,
                              const char* title,
                              std::initializer_list<float> xrange,
                              std::initializer_list<float> yrange,
                              float resolution, bool sticky) {
   if( _total_controls + 2 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LEN )
      return false;
      
   _controls[_total_controls++] = linker_x;
   _controls[_total_controls++] = linker_y;
   sprintf(_buf, "J\r%s\r%f\r%f\r%f\r%f\r%f\r%s\r",
      title, *xrange.begin(), *(xrange.begin()+1),
             *yrange.begin(), *(yrange.begin()+1),
             resolution, sticky? "True":"False");
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addPlot(float* linker, const char* title,
                          std::initializer_list<float> yrange,
                          uint8_t steps_displayed,
                          uint8_t tally,
                          uint8_t num_plots) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LEN
   ||  tally == 0
   ||  tally > (float)_report_period / (float)_step_period )
      return false;

   _reporters[_total_reporters] = linker;
   _tallies[_total_reporters++] = tally;
   sprintf(_buf, "P\r%s\r%f\r%f\r%d\r%d\r%d\r",
      title, *yrange.begin(), *(yrange.begin()+1),
      steps_displayed, tally, num_plots);
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addNumber(float* linker, const char* title, uint8_t tally) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LEN
   ||  tally == 0
   ||  tally > (float)_report_period / (float)_step_period )
      return false;
      
   _reporters[_total_reporters] = linker;
   _tallies[_total_reporters++] = tally;
   sprintf(_buf, "N\r%s\r%d\rfloat\r", title, tally);
   strcat(_build_string, _buf);
   
   return true;
}

bool CommManager::addNumber(int32_t* linker, const char* title, uint8_t tally) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LEN
   ||  tally == 0
   ||  tally > (float)_report_period / (float)_step_period )
      return false;

   _reporters[_total_reporters] = (float*)linker;
   _tallies[_total_reporters++] = tally;
   sprintf(_buf, "N\r%s\r%d\rint\r", title, tally);
   strcat(_build_string, _buf);

   return true;
}

/* The mitochondria */

void CommManager::step() {

   if( _total_reporters ) {

      if( _time_to_talk(_report_period) )
         _report();
         
      for( uint8_t reporter = 0; reporter < _total_reporters; reporter++ )
         _record(reporter);

   }
      
   _control();
   
   _wait(); // loop control
   
}

#if defined ESP32
void CommManager::_step_forever(void* param) {
   CommManager* parent = static_cast<CommManager*>(param);
   for(;;)
      parent->step();
}
#endif

uint32_t CommManager::headroom() {
   return _headroom;
}

void CommManager::_control() {

   // READ incoming message, if any
   _buf[0] = '\0';
#if defined S302_SERIAL
   if( !_serial->available() )
      return;
   uint8_t i = 0;
   while( _serial->available() ) {
      _buf[i] = (char)(_serial->read());
      if( _buf[i++] == '\n' ) // EOM
         break;
   }
   _buf[i] = '\0';
#elif defined S302_WEBSOCKETS
   _wss.loop();
#endif

   // PARSE the message
   switch(_buf[0]) {
      
      case '\0': {
         // (No message)
         return;
      } break;
      
      case '\n': {
         // (GUI is asking for the build string)
         // send buildstring
         BROADCAST(_build_string, strlen(_build_string));
         BROADCAST("\n", 2);
         return;
      } break;

      default: {
         // (update the value)
      
         int id = atoi(strtok(_buf, ":"));
         char val[24];
         strcpy(val, strtok(NULL, ":"));
         if( !strcmp(val, "true") ) { // booly boi
            *(bool*)_controls[id] = true;
         } else if ( !strcmp(val, "false") ) { // also bool
            *(bool*)_controls[id] = false;
         } else { // float
            *_controls[id] = atof(val);
         }

      } break;
   }
   
}

void CommManager::_record(uint8_t reporter) {
   float tally;
#if defined TEENSYDUINO
   tally = (float)_report_timer
#elif defined (ESP32) || (ESP8266)
   tally = (float)(micros() - _report_timer)
#endif
       * (float)_tallies[reporter] / (float)_report_period;
   uint8_t index = (int)tally; // round down to nearest index
   memcpy(&_recordings[reporter][index], (char*)_reporters[reporter], 4);
}

// Send debug messages if any
// Then send data report
void CommManager::_report() {

   TAKE
   
   // Debug messages
   uint16_t n = strlen(_debug_string); // at most MAX_DEBUG_LEN-2
   if( n > 2 ) {
      BROADCAST(_debug_string, n);
      BROADCAST("\n", 2);
      strcpy(_debug_string, "\fD");
   }

   GIVE

   // Data report
   strcpy(_buf, "\fR");
   BROADCAST(_buf, 2);
   
   for( uint8_t reporter = 0; reporter < _total_reporters; reporter++ )
      for( uint8_t tally = 0; tally < _tallies[reporter]; tally++ ) 
         BROADCAST(_recordings[reporter][tally], 4);
         
   BROADCAST("\n", 2);

}

// Whether or not enough time has passed according to the given
// time period. Determines when to report data.
bool CommManager::_time_to_talk(uint32_t time_to_wait) {
   // depends on the microcontroller
#if defined TEENSYDUINO
   if( time_to_wait <= _report_timer ) {
      _report_timer = _report_timer - time_to_wait;
#elif defined (ESP32) || (ESP8266)
   if( time_to_wait <= (micros() - _report_timer) ) {
      _report_timer = _report_timer + time_to_wait;
#else
   _NOT_IMPLEMENTED_YET();
#endif
      return true;
   }
   return false;
}

void CommManager::_wait() {
   // How we wait depends on the microcontroller
#if defined TEENSYDUINO
   uint32_t before = _main_timer;
   while(_step_period > _main_timer);
   _headroom = _main_timer - before;
   _main_timer = 0;
#elif defined (ESP32) || (ESP8266)
   _headroom = _step_period - (micros() - _main_timer);
   if( _headroom > 0 )
      delayMicroseconds(_headroom);
   _main_timer = micros();
#else
   _NOT_IMPLEMENTED_YET();
#endif
}

#if defined S302_WEBSOCKETS
// I eventually may implement a VERBOSE option to control whether anything
// is printed to Serial
void CommManager::_on_websocket_event(
   uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
   
   switch(type) {
      case WStype_DISCONNECTED: {
         Serial.printf("[%u] Disconnected\n", num);
      } break;
      case WStype_CONNECTED: {
         Serial.printf("[%u] Connected from %s\n",
            num, _wss.remoteIP(num).toString().c_str());
      } break;
      case WStype_TEXT: {
         if( payload[0] == '\0' ) {
            Serial.println("Received empty message!");
         } else {
            Serial.printf("[%u] Received:\n", num);
            Serial.println("%%%%");
            Serial.println((char*)payload);
            Serial.println("%%%%");
            Serial.printf("(Length: %d)\n", length);
            memcpy(_buf, payload, length);
            _buf[length] = '\0';
         }
      } break;

      default: break;
   }
}
#endif

/* Else */

void CommManager::debug(char* line) {

   // Add to the debug string buffer
   // (cuts off if too long though)

   TAKE

   uint16_t m = strlen(line);
   uint16_t n = strlen(_debug_string);
   uint16_t i = 0;
   while( i < m && n <= MAX_DEBUG_LEN-3 ) {
      _debug_string[n] = (line[i] != '\n'? line[i]:'\r');
      i++; n++;
   }

   _debug_string[n]   = '\r';
   _debug_string[n+1] = '\0';

   GIVE
   
   // _debug_string will be sent at each report period
}

void CommManager::_NOT_IMPLEMENTED_YET() {

   // I'm not ready yet
   // Let me clean the living room
   // Then you can come back

}