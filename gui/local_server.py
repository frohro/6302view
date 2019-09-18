import http.server
import socketserver
import asyncio
import datetime
import websockets
import serial
import sys
import time
import argparse
import configparser
from time import sleep
import serial.tools.list_ports

MCU_options = """(0): Teensy 3.2 
(1): Arduino Uno (FTDI chipset)
(2): ESP8266 D1 Mini Pro (with CH340 Adapter)
(3): ESP32 Dev Module Rev C(Si Labs Device)"""

# Command-line arg parser
parser = argparse.ArgumentParser()
parser.description = \
    "Run local server from Serial port for 6302view."
parser.add_argument("-w", "--wizard",
    help="change default preferences")
parser.add_argument("-d", "--device",
        default=1,
        dest ='d', type=int,
        help="""choose microcontroller:"""+MCU_options)
parser.add_argument("-p", "--port",
        default=6302,
    action="store_true",
       dest='p',type=int, 
    help="specify websocket port")
parser.add_argument("-v", "--verbose",
    action="store_true",
    help="verbose/debug mode")
args = parser.parse_args()

print(vars(args))
print(args.verbose)
print(args.port)
print(args.device)
config = configparser.ConfigParser()
config.read('.preferences')
config['DEFAULTS'] = {'PORT':6302,
                       'DEV':1,
                       'DEBUG': 0
                       }

#print(config.sections())
DEBUG = config['DEFAULTS']['DEBUG']
PORT = config['DEFAULTS']['PORT'] 
DEVID = 10755
VALID_DEVS = [0,1,2,3]

# VIDs for various MCU dev boards
#Arduino Uno: 10755
#ESP8266: 6790
#ESP32: 4292
#Teensy: 5824

def choose_dev():
    while True:
        print(MCU_OPTIONS)
        choice = input("")
        try:
            if int(choice) not in VALID_DEVS:
                print("Invalid Device Chosen. Try again...\n\n")
                time.sleep(0.5)
            else:
                config['PREFS']['DEV'] = choice
                break
        except Exception as e: 
            print(e)
            print("Invalid Input...\n\n")
            time.sleep(0.5)
    print("\n\n")

def choose_port():
    while True:
        print("Enter the Websocket Port you'd like to use for Python to Browser Communication. Valid choices are 6300-6400.")
        choice = input("")
        try:
            if int(choice) not in list(range(6300,6401)):
                print("Invalid Port Chosen. Try again...\n\n")
                time.sleep(0.5)
            else:
                config['PREFS']['PORT'] = choice
                break
        except:
            print("Invalid Input...\n\n")
            time.sleep(0.5)
    print("\n\n")

def choose_verbosity(): 
    while True:
        print("Python Debugging on? (Warning may slow down system when operating at high data rates). Enter 1 for True, 0 for False")
        choice = input("")
        try:
            choice = int(choice)
            if choice in [0, 1]:
                config['PREFS']['DEBUG'] = str(choice)
                break
            else:
                print("Invalid choice. Try again...\n\n")
                time.sleep(0.5)
        except Exception as e:
            print(e)
            print("Invalid Input...\n\n")
            time.sleep(0.5)

print("Starting...")
time.sleep(0.25) #pause a bit so people feel like it is really starting up
config['PREFS'] = {}

# Populate ['PREFS']
if args.wizard:
    print("Welcome to 6302View Configuration.")
    choose_dev()
    choose_port()
    choose_verbosity()
else:
    print("Run with -w flag to set preferences\n")
    config['PREFS']['DEV'] =  vars(args)['d'] if args.device else 1
    config['PREFS']['DEBUG'] = 1 if args.verbose else 0 # verbose
    config['PREFS']['PORT'] =  vars(args)['p'] if args.port else 6302
with open('.preferences', 'w') as configfile:
    config.write(configfile)

try:
    try:
        val = config['PREFS']['DEV']
        val = config.get('PREFS', config['DEFAULTS'])['DEV']
    except:
        val = config['DEFAULTS']['DEV']
    val = int(val)
    if val == 0:
        print("Ah running a Teensy. Good Taste.")
        DEVID = 5824
    elif val==1:
        print("Ah running a Uno. Good Taste.")
        DEVID = 10755
    elif val==2:
        print("Ah running a ESP8266 D1 Mini. Good Taste.")
        DEVID = 6790
    elif val == 3:
        print("Ah running an ESP32. Good Taste.")
        DEVID = 4292
    else:
        print("Invalid Preference File. Rerun with -p argument. Exiting")
        sys.exit()
except IOError:
    print("Invalid Preference File. Rerun with -p argument. Exiting")
    sys.exit()

try:
    PORT = int(config['PREFS']['PORT'])
except:
    PORT = int(config['DEFAULTS']['PORT'])


try:
    DEBUG = int(config['PREFS']['DEBUG'])==1
except:
    DEBUG  = int(config['DEFAULTS']['DEBUG'])==1

def get_usb_port():
    usb_port = list(serial.tools.list_ports.grep("USB-Serial Controller"))
    if len(usb_port) == 1:
        print("Automatically found USB-Serial Controller: {}".format(usb_port[0].description))
        return usb_port[0].device
    else:
        ports = list(serial.tools.list_ports.comports())
        port_dict = {i:[ports[i],ports[i].vid] for i in range(len(ports))}
        usb_id=None
        for p in port_dict:
            print("{}:   {} (Vendor ID: {})".format(p,port_dict[p][0],port_dict[p][1]))
            print(port_dict[p][1],DEVID)
            if port_dict[p][1]==DEVID: #for Teensy (or one type of Teensy anyways...seems to have changed)
                usb_id = p
        if usb_id== None:
            return False
        else:
            print("Found it")
            print("USB-Serial Controller: Device {}".format(p))
            return port_dict[usb_id][0].device

#use the one below later
def get_usb_ports():
    usb_ports = list(serial.tools.list_ports.grep(""))
    print(usb_ports)
    if len(usb_ports) == 1:
        print("Automatically found USB-Serial Controller: {}".format(usb_ports[0].description))
        return usb_ports[0].device
    else:
        ports = list(serial.tools.list_ports.comports())
        port_dict = {i:[ports[i],ports[i].vid] for i in range(len(ports))}
        usb_id=None
        for p in port_dict:
            print("{}:   {} (Vendor ID: {})".format(p,port_dict[p][0],port_dict[p][1]))
        return port_dict


serial_connected = False
comm_buffer = b""  #messages start with \f and end with \n.

async def connect_serial():
    global ser 
    global serial_connected     
    s = get_usb_port()
    print("USB Port: "+str(s))
    baud = 115200
    if s:
        ser = serial.Serial(port = s, 
            baudrate=115200, 
            parity=serial.PARITY_NONE, 
            stopbits=serial.STOPBITS_ONE, 
            bytesize=serial.EIGHTBITS,
            timeout=0.01) #auto-connects already I guess?
        print(ser)
        print("Serial Connected!")
        if ser.isOpen():
             #print(ser.name + ' is open...')
             serial_connected = True
             return True
    return False

ser = connect_serial()

async def send_down(message):
    global serial_connected
    if not serial_connected:
        await connect_serial()
    try:
        msg = message.encode('ascii')
        ser.write(msg)
        if DEBUG:
            print("▼", msg)
    except Exception as e:
        print("failing on write")
        ser.close()
        serial_connected = False

async def downlink(websocket):
    while True:
        message = await websocket.recv()
        await send_down(message)

async def uplink(websocket):
    global comm_buffer
    global serial_connected
    while True:
        if not serial_connected:
            print("c");
            await connect_serial()
        try:
            data = ser.read(100) #Larger packets improve efficiency.
            await asyncio.sleep(0.001) #Needed so page->cpu messages go.
        except Exception as e:
            print("failing on read")
            ser.close()
            serial_connected = False

        try:
            if DEBUG and data != b'':
                print("▲", data)
            await websocket.send(data)
        except Exception as e:
            #print(e)
            break
        
async def handler(websocket, path):
    global serial_connected
    if not serial_connected:
        await connect_serial()
    #asyncio.gather(uplink(websocket),downlink(websocket))
    page2mcu = asyncio.ensure_future(downlink(websocket))
    mcu2page = asyncio.ensure_future(uplink(websocket))
    done, pending = await asyncio.wait([page2mcu,mcu2page],return_when=asyncio.FIRST_COMPLETED)
    for task in pending:
        task.cancel()
try:
    server = websockets.serve(handler, "127.0.0.1", PORT)
    asyncio.get_event_loop().run_until_complete(server)
    asyncio.get_event_loop().run_forever()
except KeyboardInterrupt:
    print('\nCtrl-C')
finally:
    server.close()
    asyncio.get_event_loop.close()



