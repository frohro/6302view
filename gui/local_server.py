
# standard library
import asyncio
import sys
import time
import argparse
import configparser

# ANSI color codes seem reasonably cross-platform?
RED = "\033[91m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
RESET = "\033[0m"

# Python version 3.8+ strongly suggested
if sys.version_info < (3, 8):
    import warnings
    version_warning_msg = (
        "Please consider running me with Python v3.8 or above! "
        f"(Detected: v{sys.version_info.major}.{sys.version_info.minor})"
    )
    warnings.warn(version_warning_msg, RuntimeWarning)
    print()

# third party
try:
    import websockets
    import serial
    import serial.tools.list_ports
except ModuleNotFoundError:
    print("I can't seem to load the websockets and serial modules")
    print("Can you check your installation before trying again?")
    print("These commands may come in handy:")
    print(f"   {YELLOW}pip freeze | grep -E 'websockets|pyserial'{RESET}")
    print(f"   {YELLOW}pip install -U websockets pyserial{RESET}")
    print("")
    raise

# type checking and organization
import typing
from dataclasses import dataclass

@dataclass(frozen=True)
class MCU:
    device_id: int
    name: str
    nickname: str
    nickname_determiner: str

# Supported device IDs
VALID_MCUs: typing.List[MCU] = [
    MCU(5824, "Teensy 3.2", "Teensy", "a Teensy"),
    MCU(10755, "Arduino Uno (FTDI chipset)", "Arduino Uno", "an Uno"),
    MCU(6790, "ESP8266 D1 Mini Pro (with CH340 Adapter)", "ESP8266", "an ESP8266"),
    MCU(4292, "ESP32 Dev Module Rev C(Si Labs Device)", "ESP32", "an ESP32"),
]

DEVICE_LIST: str = \
    "\n".join([ f"({i}): {mcu.name}" for i, mcu in enumerate(VALID_MCUs)])

class Preferences:
    """Loads, asks for, and generally handles user preferences
    """
    PROMPT_SYM = "- "
    DEFAULTS = { "DEV": "1", "PORT": "6302", "VERBOSE": "0" }

    def __init__(self):
        self.config = configparser.ConfigParser()
        self.config["PREFS"] = {}
        self.config.read(".preferences")

        # defaults
        for k, v in self.DEFAULTS.items():
            if not k in self.config["PREFS"]:
                self.config["PREFS"][k] = v
        self.save() # create new .preferences file if not exists

        # True if device came from .preferences file 
        self.device_from_config_file = True

    @property
    def verbose(self) -> bool:
        return self.config["PREFS"]["VERBOSE"] == "1"

    @property
    def port(self) -> int:
        return int(self.config["PREFS"]["PORT"])

    @property
    def device_index(self) -> int:
        return int(self.config["PREFS"]["DEV"])

    @property
    def mcu(self) -> MCU:
        return VALID_MCUs[self.device_index]

    def run_wizard(self) -> None:
        """Run configuration wizard"""
        print("Welcome to 6302View Configuration.")
        print("Your settings will be saved to `.preferences`.")
        
        self.choose_device()
        self.choose_port()
        self.choose_verbosity()

        self.save()
        self.device_from_config_file = False

    def save(self) -> None:
        with open(".preferences", "w") as configfile:
            self.config.write(configfile)

    def choose(self, prompt: str, test: typing.Callable[[str], bool], error_msg: str, default_input: str="") -> str:
        while True:
            print(prompt)
            print(self.PROMPT_SYM, end="")
            print(YELLOW, end="")
            try:
                choice = input().strip()
            except KeyboardInterrupt:
                print(f"{RESET}\nCtrl-C")
                sys.exit(0)
            print(RESET, end="")
            if not choice: choice = default_input
            try:
                if test(choice): return choice
                print(error_msg + "\n\n")
            except Exception as e:
                print(e)
                print("Invalid input...\n\n")
            finally:
                time.sleep(0.5)
        print("\n\n")

    def choose_device(self) -> None:
        self.config["PREFS"]["DEV"] = self.choose(
            "Choose and Enter the number of the Microcontroller you're using:\n" + DEVICE_LIST + \
                "\nDefault is 1 (Arduino Uno)",
            lambda choice: int(choice) in range(len(VALID_MCUs)),
            "Invalid Device Chosen. Try again...",
            "1"
        )

    def choose_port(self) -> None:
        self.config["PREFS"]["PORT"] = self.choose(
            "Enter the Websocket Port you'd like to use for Python to Browser Communication. " + \
                "Valid choices are 6300-6400. Default is 6302.",
            lambda choice: int(choice) in range(6300, 6401),
            "Invalid Port Chosen. Try again...",
            "6302"
        )

    def choose_verbosity(self) -> None:
        self.config["PREFS"]["VERBOSE"] = self.choose(
            "Verbose mode? (Warning may slow down system when operating at high data rates). " + \
                "Enter 1 for True, 0 for False. Default is 0.",
            lambda choice: int(choice) in [0, 1],
            "Invalid choice. Try again...",
            "0"
        )

def parse_args() -> argparse.Namespace:
    """Defines the CLI arg parser and parses the args
    """
    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawTextHelpFormatter
    parser.description = \
        "Broadcast Serial communication over WebSockets for 6302view."
    parser.add_argument(
        "-d", "--device",
        help=f"specify microcontroller:\n{DEVICE_LIST}")
    parser.add_argument(
        "-p", "--port",
        help="specify websocket port")
    parser.add_argument(
        "-v", "--verbose", action="store_true",
        help="print up- and downstream bytes to console")
    parser.add_argument(
        "-w", "--wizard",
        action="store_true",
        help="change default preferences")

    return parser.parse_args()

def handle_preferences() -> Preferences:
    """
    Load the preferences from `.preferences`, the command-line args, or the
    wizard setup, in increasing order of priority.
    """
    preferences = Preferences()
    args = parse_args()

    if not args.wizard:
        print(f"Run with {YELLOW}-w{RESET} flag to set preferences\n")

        # verbose flag
        if args.verbose: preferences.config["PREFS"]["VERBOSE"] = "1"

        # port flag
        if args.port:
            port = -1 if not args.port.isdigit() else int(args.port)
            if not port in range(6300, 6401):
                print("Please specify a valid port within 6300 and 6400 inclusive.")
                sys.exit(1)
            preferences.config["PREFS"]["PORT"] = str(port)

        # device index flag
        if args.device:
            device_index = -1 if not args.device.isdigit() else int(args.device)
            if not device_index in range(len(VALID_MCUs)):
                print("Please specify a valid device ID in accordance with below:")
                print(DEVICE_LIST)
                sys.exit(1)
            preferences.config["PREFS"]["DEV"] = str(device_index)
            preferences.device_from_config_file = False

    else:
        preferences.run_wizard()

    return preferences

class Handler:
    """Handles communication between the microcontroller and the WebSockets server
    """
    def __init__(self, preferences: Preferences):
        self.preferences = preferences
        self.serial = self.connect_serial()

    @property
    def connected(self) -> bool:
        #return bool(self.serial)
        return self.serial.is_open

    def get_usb_port(self) -> typing.Optional[str]:
        """Detects the device of the USB-Serial controller, if found
        """
        # automatically select first USB-Serial controller if found
        controllers = list(serial.tools.list_ports.grep("USB-Serial Controller"))
        if len(controllers) == 1:
            port = controllers[0] # (port, desc, hwid)
            print(f"Automatically found USB-Serial Controller: {BLUE}{port[1]}{RESET}")
            #print("port[0]    : ", port[0])
            #print("port.device: ", port.device) # type: ignore
            return port[0]
        
        # alternative
        ports = list(serial.tools.list_ports.comports())
        device_id = self.preferences.mcu.device_id
        print(f"Found {len(ports)} port(s), looking for {BLUE}{device_id}{RESET}...")
        ports_found = []
        for i, port in enumerate(ports):
            ports_found.append(f"   {i}: {port} (Vendor ID: {BLUE}{port.vid}{RESET})")
            if port.vid == device_id: # one type of Teensy might have unexpected vendor id?
                break
        else:
            print("\nI couldn't find your chosen device! D:")
            if ports_found:
                print("Here is the list of ports I did find:")
                print("\n".join(ports_found))
                print()
            supported_ports = [
                p for p in ports
                if any(mcu.device_id == p.vid for mcu in VALID_MCUs)
            ]
            if supported_ports:
                for mcu in VALID_MCUs:
                    if mcu.device_id == supported_ports[0].vid:
                        if self.preferences.device_from_config_file:
                            print((
                                f"Your preferences told me to look for {self.preferences.mcu.nickname_determiner}, "
                                f"but maybe you meant to go with your {BLUE}{mcu.nickname}{RESET}? "
                                f"That one seems plugged in just fine. "
                                f"You can change your preferences by running me with the {YELLOW}-w{RESET} flag.\n"
                            ))
                        else:
                            print((
                                f"I know you told me to search for {self.preferences.mcu.nickname_determiner}, "
                                f"but maybe you meant to go with your {BLUE}{mcu.nickname}{RESET}? "
                                f"That one seems plugged in just fine.\n"
                            ))
                        break
            return None

        print("Found it!")
        print(f"USB-Serial Controller: Device {i}")
        return port.device

    def connect_serial(self) -> serial.Serial:
        """Connects to the device found by `get_usb_port()`
        """
        device = self.get_usb_port()
        if device is None:
            #print("I couldn't find your USB-Serial controller D:")
            print((
                f"Please check your setup before trying again. You may find these commands helpful:\n"
                f"   {YELLOW}python -m serial.tools.list_ports{RESET}\n"
                f"   {YELLOW}ls -d /dev/ttyUSB*{RESET}\n"
            ))
            sys.exit(1)

        print(f"USB Port: {BLUE}{device}{RESET}")
        self.serial = serial.Serial(
            port=device,
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=0.01,
        )

        print(self.serial)
        print("Serial connected!\n")

        return self.serial

    async def downlink(self, websocket):
        """Passes websocket messages to the microcontroller
        """
        while True:
            # get message from websocket
            message = await websocket.recv()

            # send message down the serial connection
            if not self.connected: self.connect_serial()
            try:
                msg = message.encode("ascii")
                self.serial.write(msg)
                if self.preferences.verbose: print("▼", msg)
            except KeyboardInterrupt:
                raise
            except Exception as e:
                print(f"failing on write: {RED}{e}{RESET}")
                self.serial.close()

    async def uplink(self, websocket):
        """Passes microcontroller messages to the websocket
        """
        while True:
            if not self.connected: self.connect_serial()

            # get message from serial
            data = b""
            try:
                data = self.serial.read(100) # Larger packets improve efficiency.
                await asyncio.sleep(0) # Needed so page->cpu messages go.
            except KeyboardInterrupt:
                raise
            except Exception as e:
                print(f"failing on read: {RED}{e}{RESET}")
                self.serial.close()
                await asyncio.sleep(1)

            # send message to websocket
            try:
                if self.preferences.verbose and data != b"": print("▲", data)
                await websocket.send(data)
            except KeyboardInterrupt:
                raise
            except Exception as e:
                print(e)
                break

    async def handler(self, websocket, path):
        """Handles communication between the websocket and the microcontroller
        """
        if not self.connected: self.connect_serial()

        page_to_mcu = asyncio.ensure_future(self.downlink(websocket))
        mcu_to_page = asyncio.ensure_future(self.uplink(websocket))

        done, pending = await asyncio.wait(
            [page_to_mcu, mcu_to_page],
            return_when=asyncio.FIRST_COMPLETED
        )
        for task in pending: task.cancel()

    def run(self):
        """Creates the WebSockets server

        `self.handler` handles incoming websocket connections
        """
        async def main():
            serve = websockets.serve # type: ignore
            server = serve(self.handler, "127.0.0.1", self.preferences.port)
            async with server: await asyncio.Future()

        asyncio.run(main())

if __name__ == "__main__":
    preferences = handle_preferences()

    print(f"Ah, running {preferences.mcu.nickname_determiner}. Good taste.\n")
    print("Starting...")
    time.sleep(0.25) # pause a bit so we feel like it is really starting up

    handler = Handler(preferences)

    try:
        handler.run()
    except KeyboardInterrupt:
        print("\nCtrl-C")
