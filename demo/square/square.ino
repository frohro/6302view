
#include "Six302.h"

// Step rate is once per 1 s
// Report rate is once per 5 s
#define STEP_TIME 1000000
#define REPORT_TIME 5000000

CommManager cm(STEP_TIME, REPORT_TIME);

float input;
float output;

void setup() {
   cm.addSlider(&input,  "Input", {-5, 5},  0.1);
   cm.addPlot(&output, "Output", {-1, 30});
   
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   //cm.debug("Test");
   cm.step();
}
