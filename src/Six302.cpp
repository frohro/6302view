#include "Six302.h"

CommManager::CommManager(uint32_t sp=1000, uint32_t rp=20000) {
   _step_period = sp;
   _report_period = rp;
   _total_controls = 0;
   _total_reporters = 0;
}

bool CommManager::connect(usb_serial_class* s, uint32_t baud) {
   _architecture = S302_SERIAL;
   _serial = s;
   _baud = baud;
   _serial->begin(baud);
   while(!_serial);
   strcpy(_build_string, "test"); // DEBUG
   _build();
   return true;
}

bool CommManager::connect(char* ssid, char* pw) {
   _architecture = S302_WEBSOCKETS;
   _NOT_IMPLEMENTED_YET();
   return false;
}

/* Controls */

bool CommManager::addToggle(bool* linker, char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = (float*)linker;
   char temp[4+MAX_TITLE_LENGTH];
   sprintf(temp, "T\r%s\r", title);
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addButton(bool* linker, char* title) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = (float*)linker;
   char temp[4+MAX_TITLE_LENGTH];
   sprintf(temp, "B\r%s\r", title);
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addSlider(float* linker, char* title,
                            std::initializer_list<float> range,
                            float resolution, bool toggle=false) {
   if( _total_controls + 1 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = linker;
   char temp[8+MAX_TITLE_LENGTH+3*24+5];
   sprintf(temp, "S\r%s\r%d\r%d\r%d\r%s\r",
      title, range[0], range[1], resolution, toggle? "True":"False");
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addJoystick(float* linker_x, float* linker_y,
                              char* title,
                              std::initializer_list<float> xrange,
                              std::initializer_list<float> yrange,
                              float resolution, bool sticky=true) {
   if( _total_controls + 2 > MAX_CONTROLS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _controls[_total_controls++] = linker_x;
   _controls[_total_controls++] = linker_y;
   char temp[10+MAX_TITLE_LENGTH+5*24+5];
   sprintf(temp, "J\r%s\r%d\r%d\r%d\r%d\r%d\r%s\r",
      title, xrange[0], xrange[1],
             yrange[0], yrange[1], resolution, sticky? "True":"False");
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addPlot(float* linker, char* title,
                          std::initializer_list<float> yrange,
                          int steps_displayed=10, int num_plots=1) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = linker;
   char temp[8+MAX_TITLE_LENGTH+4*24];
   sprintf(temp, "P\r%s\r%d\r%d\r%d\r%d\r",
      title, yrange[0], yrange[1], steps_displayed, num_plots);
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addNumber(float* linker, char* title) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = linker;
   char temp[10+MAX_TITLE_LENGTH];
   sprintf(temp, "N\r%s\rfloat\r", title);
   strcat(_build_string, temp);
   return true;
}

bool CommManager::addNumber(int32_t* linker, char* title) {
   if( _total_reporters >= MAX_REPORTERS
   ||  strlen(title) > MAX_TITLE_LENGTH )
      return false;
   _reporters[_total_reporters++] = (float*)linker;
   char temp[10+MAX_TITLE_LENGTH];
   sprintf(temp, "N\r%s\rint\r", title);
   strcat(_build_string, temp);
   return true;
}

/* Build */

bool CommManager::_build() {
   switch( _architecture ) {
      case S302_SERIAL: {
         while(_serial->available())
            _serial->read();
         _serial->print('B');
         _serial->print(_build_string);
         _serial->write(0);
         while(!_serial->available());
         if( (char)_serial->read() == '\n' ) {
            _connected = true; // yay
            return true;
         } else { return false; }
      } break;
      case S302_WEBSOCKETS: {
         _NOT_IMPLEMENTED_YET();
         return false;
      } break;
   }
}

/* The mitochondria */

bool CommManager::step() {
   _control(); // get info from GUI
   _report();  // send info to GUI
   _wait();    // loop control
   return true; // may return headroom in the future
}

void CommManager::_control() {
   // read incoming message
   switch( _architecture ) {
      case S302_SERIAL: {
         if( !_serial->available() )
            return;
         uint8_t temp = 0;
         while( _serial->available() ) {
            _incoming[temp] = (char)(_serial->read());
            if( _incoming[temp++] == '\n' ) // EOM
               break;
         }
      } break;
      case S302_WEBSOCKETS: {
         _NOT_IMPLEMENTED_YET();
      } break;
   }
   // parse the message
   int id = atoi(strtok(_incoming, ":"));
   if( id < 0 ) { // disconnect!
      _connected = false;
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
   if( !_total_reporters ) return;
   // Pack up the data
   for( uint8_t i = 0; i < _total_reporters; i++ ) {
      // I'm making the assumption that all reports are floats
      // otherwise, use something like `sizeof(*_reporters[i])`
      memcpy(&_outgoing[4 * i], _reporters[i], 4);
   }

   switch( _architecture ) {
      case S302_SERIAL: {

         _serial->print('R');
         _serial->write(_outgoing, 4*_total_reporters);

      } break;
      case S302_WEBSOCKETS: {
         _NOT_IMPLEMENTED_YET();
      } break;
   }
}

uint32_t CommManager::_wait() {
   // How we wait depends on the microcontroller
   #if defined TEENSYDUINO
      while(_step_period > _timer);
      _timer = 0;
   #elif defined ESP32
      _NOT_IMPLEMENTED_YET();
   #elif defined ESP8266
      _NOT_IMPLEMENTED_YET();
   #else // I'm assuming it's an Arduino Uno
      _NOT_IMPLEMENTED_YET();
   #endif
}

/* Else */

void CommManager::debug(const char* line) {
   _serial->print('D');
   _serial->print(line);
   _serial->write(0);
}

void CommManager::_NOT_IMPLEMENTED_YET() {

   // I'm not ready yet
   // Let me clean the living room
   // Then you can come back

   delay(666000);
}
