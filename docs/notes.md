# 6302view

Diagrams to be added. This page to be prettified.

## Set-up

Start off including the library and creating an instance of the `CommManager` class.

```cpp
#include <Six302.h>

CommManager cm(1000, 5000);
```

The numbers control the rate at which the system works. `1000` is the time in microseconds between loops (step period), and `5000` is the time in microseconds between each *report* (report period).

There are, so far, two modes of communication between the GUI and the microcontroller. Data can be communicated over **Serial**, or by **WebSockets**.

### Serial

Choose `#define S302_SERIAL` at the top of `Six302.h` for this mode. To connect, enter a Serial pointer and BAUD rate:

```cpp
cm.connect(&Serial, 115200);
```

### WebSockets

Choose `#define S302_WEBSOCKETS` at the top of `Six302.h` for this mode. To connect, enter your SSID and p/w:

```cpp
cm.connect("Mom use this one", "password");
```

The Serial monitor will display the local IP address of your microcontroller that is used in the GUI. Something like:

```plaintext
Connecting to Mom use this one WiFi .. connected!
--> 10.0.0.18 <--
```

### Adding modules

To add controls and reporters, use the following `CommMannger` routines.

#### Controls

* Add a toggle module with `addToggle`
* Add a button module with `addButton`
* Add a slider module with `addSlider`
* Add a joystick module with `addJoystick`

#### Reporters

* Add a plot module with `addPlot`
* Add a plain number module with `addNumber`

Check the header file for what arguments these take. In general, it's `pointer`, `title`, followed by other, potentially optional args. <!-- to be filled in -->

### `cm.step`

```cpp
void loop() {
   /* do stuff */
   cm.step();
}
```

`cm.step` updates the inputs, reports the outputs, and conveniently blocks according to your given loop rate (e.g. the 1000 microseconds above).

**Note**: On the ESP32, `cm.step` runs on the second core and so does not need to be ran in the main loop as it does for other microcontrollers.

## For example

On the **Teensy** over **Serial**, at the head of `Six302.h`, you would have

```cpp
#define S302_SERIAL
//#define S302_WEBSOCKETS
```

and in your `.ino`, you may have:

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

## How the information is communicated (the details)

### GUI → Microcontroller

* The `cm.step` routine listens for messages from the GUI of the form `id:value\n` where `\n` is a newline character. For example, if the ID index of a `float` control were `0`, and the GUI wants to set it to `6.28`, then it would send `0:6.28\n`. If the control were for a `bool`, then the message would be `0:true\n` or `0:false\n`. A joystick controls two `float`s and is controlled with two `id:value\n` messages.
* The GUI asks the microcontroller for the buildstring by just sending `\n`.

### Microcontroller → GUI

There are three types of signals sent from the microcontroller:

* What modules to build
* The data report
* Debugger messages

**Note:** All messages sent from the microcontroller to the GUI are enclosed in `\f` to start and `\n` to close.

#### How build instructions are sent

The build instructions' syntax is `\fB` followed by the list of modules and closing with `\n`. Each module starts with a letter to signify the type, follows with the name, and then with the remaining arguments as they are defined in the routine.
* `T` for Toggle
* `B` for Button
* `S` for Slider
* `J` for Joystick
* `P` for Plot
* `N` for Numerical reporter

Each module, as well as the arguments of each module, are separated by `\r`.

For example, the build string for the quick example above (the one that adds a slider and a plot) is:

```plaintext
\fBS\rInput\r-5.000000\r5.000000\r0.100000\rFalse\rP\rOutput\r-1.000000\r30.000000\r10\r1\r\n
```

<!--(Sadly, the carriage return is not rendered as a new line in the serial monitor (Arduino IDE 1.8.9 on Windows 10).)-->

#### How the data are reported

Report messages take the form of `\fR` followed by packs of 4 bytes, where each pack represent a `float` or 32-bit `int` value, closing with `\n`. The bytes are sent in the order they were added, which is precisely the order as they appear in the build string.

Therefore, from the GUI perspective, messages coming in starting with `\fR` will have `4 * _total_reporters` bytes follow, then the closing `\n`.

<!-- Issue with missed bits??? -->

#### How debug messages are sent

When using a serial communication setup, the intended way to write debug messages is with `cm.debug`. Each line of the user's debug messages starts with `\fD`, follows with the line, and terminates by `\n`.

