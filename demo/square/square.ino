
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
   cm.step();
}
