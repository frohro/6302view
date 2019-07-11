#include "Six302.h"

CommManager::CommManager(uint32_t sp, uint32_t rp) {
   _step_period = sp;
   _report_period = rp;
#if defined S302_WEBSOCKETS
   _wss = WebSocketsServer(80);
#endif
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
   while(!_serial);
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
   disableCore0WDT();
   xTaskCreatePinnedToCore(
      this->_step_forever,
      "6302view",
      10000, /* Stack size, in words */
      this,
      0,
      &_six302_task,
      0);
#endif
}

/* Controls */

bool CommManager::addToggle(bool* linker, const char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = (float*)linker;
   sprintf(_tmp, "T\r%s\r", title);
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addButton(bool* linker, const char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = (float*)linker;
   sprintf(_tmp, "B\r%s\r", title);
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addSlider(float* linker, const char* title,
                            std::initializer_list<float> range,
                            float resolution, bool toggle) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = linker;
   sprintf(_tmp, "S\r%s\r%f\r%f\r%f\r%s\r",
      title, *range.begin(), *(range.begin()+1),
      resolution, toggle? "True":"False");
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addJoystick(float* linker_x, float* linker_y,
                              const char* title,
                              std::initializer_list<float> xrange,
                              std::initializer_list<float> yrange,
                              float resolution, bool sticky) {
   if( _total_controls + 2 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = linker_x;
   _controls[_total_controls++] = linker_y;
   sprintf(_tmp, "J\r%s\r%f\r%f\r%f\r%f\r%f\r%s\r",
      title, *xrange.begin(), *(xrange.begin()+1),
             *yrange.begin(), *(yrange.begin()+1),
             resolution, sticky? "True":"False");
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addPlot(float* linker, const char* title,
                          std::initializer_list<float> yrange,
                          int steps_displayed, int num_plots) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = linker;
   sprintf(_tmp, "P\r%s\r%f\r%f\r%d\r%d\r",
      title, *yrange.begin(), *(yrange.begin()+1),
      steps_displayed, num_plots);
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addNumber(float* linker, const char* title) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = linker;
   sprintf(_tmp, "N\r%s\rfloat\r", title);
   strcat(_build_string, _tmp);
   return true;
}

bool CommManager::addNumber(int32_t* linker, const char* title) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = (float*)linker;
   sprintf(_tmp, "N\r%s\rint\r", title);
   strcat(_build_string, _tmp);
   return true;
}

/* The mitochondria */

void CommManager::step() {
   switch( _state ) {
      case S302_DISCONNECTED: {
         _wait_for_connection();
      } break;
      case S302_TALK: { // send info to GUI
         if( _total_reporters
         &&  _time_to_talk(_report_period) )
            _report();
         _control();
      } break;
   }
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

   // read incoming message, if any
   strcpy(_buf, "");
#if defined S302_SERIAL
   if( !_serial->available() )
      return;
   uint8_t i = 0;
   while( _serial->available() ) {
      _buf[i] = (char)(_serial->read());
      if( _buf[i++] == '\n' ) // EOM
         break;
   }
#elif defined S302_WEBSOCKETS
   _wss.loop();
#endif
   if( !strlen(_buf) ) return;
   
   // parse the message
   int id = atoi(strtok(_buf, ":"));
   if( id < 0 ) { // disconnect!
      _state = S302_DISCONNECTED;
      return;
   }
   // update the value
   char val[24];
   strcpy(val, strtok(NULL, ":"));
   if( !strcmp(val, "true") ) { // booly boi
      *(bool*)_controls[id] = true;
   } else if ( !strcmp(val, "false") ) { // also bool
      *(bool*)_controls[id] = false;
   } else { // float
      *_controls[id] = atof(val);
   }
}

// Pack the important data as bytes
// And send 'em off
void CommManager::_report() {

   // Pack up the data
   _buf[0] = 'R';
   for( uint8_t i = 0; i < _total_reporters; i++ ) {
      // I'm making the assumption that all reports are floats
      // otherwise, use something like `sizeof(*_reporters[i])`
      memcpy(&_buf[1 + 4 * i], _reporters[i], 4);
   }
   
   // Send it off
#if defined S302_SERIAL
   _serial->write(
#elif defined S302_WEBSOCKETS
   _wss.broadcastBIN(
#endif
      (uint8_t*)_buf, 4 * _total_reporters);
   
}

void CommManager::_wait_for_connection() {
#if defined S302_SERIAL
   if( (char)_serial->read() == '\n' )
      _state = S302_TALK; // yay
                          // we're connected
   else if( _time_to_talk(S302_WAIT_FOR_CONNECT) )
      _serial->println(_build_string);
      
#elif defined S302_WEBSOCKETS
   _wss.loop();
   if( _buf[0] == '\n' )
      _state = S302_TALK;
   else if( _time_to_talk(S302_WAIT_FOR_CONNECT) )
      _wss.broadcastBIN((uint8_t*)_build_string, strlen(_build_string));
         
#endif

}

// Whether or not enough time has passed to report again.
// Controls how often the build string is sent and how often data
// is reported.
bool CommManager::_time_to_talk(uint32_t time_to_wait) {
   // depends on the microcontroller
#if defined TEENSYDUINO
   if( time_to_wait <= _report_timer ) {
      _report_timer = 0;
      return true;
   }
#elif defined (ESP32) || (ESP8266)
   if( time_to_wait <= (micros() - _report_timer) ) {
      _report_timer = micros();
      return true;
   }
#endif
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
#else // I'm assuming it's an Arduino Uno
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
#if defined S302_SERIAL
   _serial->print('D');
   _serial->print(line);
   _serial->write(0);
#elif defined S302_WEBSOCKETS
   //Serial.println(line);
   _NOT_IMPLEMENTED_YET();
#endif
}

void CommManager::_NOT_IMPLEMENTED_YET() {

   // I'm not ready yet
   // Let me clean the living room
   // Then you can come back

}