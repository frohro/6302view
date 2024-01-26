/*
   I use this script for testing to make sure that all available modules are
   working correctly when added.  I created this because we've had some issues
   with `sprintf` working on the Espressif & Teensy boards but not working on
   AVR boards (which need to use `dtostrf`, `itoa`, `strncpy`, and `strcat`
   instead).

   -Maddie
*/

#include <Six302.h>

// microseconds
#define STEP_TIME 10000
#define REPORT_TIME 50000

CommManager cm(STEP_TIME, REPORT_TIME);

bool toggle = false;
bool button;
float slider;

float plot = 6.283185;
int32_t number = 123;

void setup() {
   /* Add modules */
   cm.addToggle(&toggle, "(Toggle) really really really long title");
   //cm.addButton(&button, "(Button) really really really long title");
   //cm.addSlider(&slider, "(Slider) really really really long title", -3, 3, 0.1);

   //cm.addPlot(&plot, "(Plot) really really really long title", -10, 10);
   //cm.addNumber(&plot, "(float Number) really really really long title");
   //cm.addNumber(&number, "(int Number) really really really long title");

   /* Ready to communicate over Serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   plot = slider * slider;
   cm.step();
}