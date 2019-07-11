
/* For this demo, make sure the `#define S302_SERIAL` macro
   is chosen in the library. This demo should compile on the Teensy,
   ESP32, and ESP8266. */

#include "Six302.h"

// microseconds
#define STEP_TIME 100000
#define REPORT_TIME 500000

CommManager cm(STEP_TIME, REPORT_TIME);

float input;
float output;

void setup() {
   /* Add modules */
   cm.addSlider(&input, "Input", {-5, 5}, 0.1);
   cm.addPlot(&output, "Output", {-1, 30});

   /* Ready to communicate over serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step(); // This line isn't necessary if compiling on the ESP32!
}
