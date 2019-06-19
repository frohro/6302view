# 6302view

Diagrams to be added. This page to be prettified.

## Quick example

```cpp
#include <Six302.h>

CommManager cm(1000, 5000);

float input, output;

void setup() {
   // Add modules
   cm.addSlider(&input, "Input", {-5, 5}, 0.01);
   cm.addPlot(&output, "Output", {-1, 30});
   // Connect via serial
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step();
}
```

This creates one control (a slider) and one reporter (a plot). The input is squared into the output, so, sliding from -5 to 0 to +5 moves the plot from 25 to 0 to 25.

## Modules

### Controls

* Add a toggle module with `addToggle`
* Add a button module with `addButton`
* Add a slider module with `addSlider`
* Add a joystick module with `addJoystick`

### Reporters

* Add a plot module with `addPlot`
* Add a plain number module with `addNumber`

## How the information is communicated

### GUI → Microcontroller

* When the build string is being sent to the GUI, the microcontroller listens for the signal `\n`, meaning "all is well", and the system is considered connected.
* The `cm.step` routine listens for messages of the form `id:value\n`. For example, if the ID index of a `float` control were `0`, and the GUI wants to set it to `6.28`, then it would send `0:6.28\n`. If the control were for a `bool`, then the message would be `0:true` or `0:false`. A joystick controls two `float`s and is controlled with two `id:value\n` signals.
* A message of `-1\n` signals disconnection.

### Microcontroller → GUI

There are three types of signals sent from the microcontroller:

* What modules to build
* The data report
* Debugger messages

#### How build instructions are sent

The build instructions' syntax is the character `B`, followed by the list of modules, and terminated with '\n'.

Each module starts with a letter to signify the module, follows with the name, and then with the remaining arguments as they are defined in the method.
* `T` for Toggle
* `B` for Button
* `S` for Slider
* `J` for Joystick
* `P` for Plot
* `N` for Numerical reporter

Each module, as well as the arguments of each module, are separated by `\r`.

For example, the build string for the quick example above (which adds a slider and a plot) is:

```plaintext
S\rInput\r-5.000000\r5.000000\r0.100000\rFalse\rP\rOutput\r-1.000000\r30.000000\r10\r1\r\n
```

Sadly, the carriage return is not rendered as a new line in the serial monitor (Arduino IDE 1.8.9 on Windows 10).

#### How the data are reported

Report messages are the character `R` (ASCII 82) followed by packs of 4 bytes, where each pack represent a `float` or 32-bit `int` value. The bytes are sent in the order they were added, which is also the same order as they appear in the build string. So, each report is `1 + 4 * (total reporters)` bytes in length.

<!-- Issue with missed bits? -->

#### How debug messages are sent

When using a serial communication setup, the intended way to write debug messages is with `cm.debug`. The syntax starts with `D` and follows with the user's message, null-terminated.

