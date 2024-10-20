import socket
import time
from gpiozero import LED, Button
from signal import pause


red_led = LED(17)
green_led = LED(22)
yellow_led = LED(27)
white_led = LED(23) 

button = Button(18)

# UDP setup
ESP8266_IP = "192.168.1.82" 
ESP8266_PORT = 8266      
UDP_PORT = 8266      # Port on Raspberry Pi to listen
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  
sock.setblocking(False)

# Threshold values
LOW_THRESHOLD = 300
MEDIUM_THRESHOLD = 700

listening = False  # check whether UDP listening has started
communication_on = False
last_message_time = 0  # track the last received UDP message time
error_state = False

def send_udp_message(message):
    sock.sendto(message.encode(), (ESP8266_IP, ESP8266_PORT))
    print(f"Sent UDP message: {message} to ESP8266")

def control_leds(sensor_value):
    if sensor_value < LOW_THRESHOLD:
        red_led.on()
        green_led.off()
        yellow_led.off()
        print("Low light detected. Turning on RED LED.")
    elif LOW_THRESHOLD <= sensor_value < MEDIUM_THRESHOLD:
        red_led.on()
        yellow_led.on()
        green_led.off()
        print("Medium light detected. Turning on RED and YELLOW LEDs.")
    else:
        red_led.on()
        green_led.on()
        yellow_led.on()
        print("High light detected. Turning on all RGB LEDs.")

def reset_system():
    """Reset the system to its initial state."""
    global error_state
    red_led.off()
    green_led.off()
    yellow_led.off()
    white_led.off()
    error_state = False
    print("System reset to initial state")

def flash_white_led():
    """Flash the white LED to indicate an error state."""
    while error_state:
        white_led.toggle()
        time.sleep(0.5)  # Flash every 0.5 seconds

def button_pressed():
    """Handle the button press event to start/stop communication."""
    global listening, communication_on, last_message_time, error_state
    if not communication_on:
        print("Button Pressed. Initiating communication...")
        white_led.on()  # Indicate communication start
        send_udp_message("Start Communication")
        listening = True  # Start listening for UDP messages
        communication_on = True  # Set communication as active
        last_message_time = time.time()  # Reset the last message timer
    else:
        print("Button Pressed. Stopping communication...")
        send_udp_message("Stop Communication")
        reset_system()
        listening = False
        communication_on = False

# Attach the button press handler
button.when_pressed = button_pressed

print("Press the button to start/stop UDP communication...")

try:
    while True:
        if listening:
            try:
                data, addr = sock.recvfrom(1024)
                sensor_value = int(data.decode().split()[-1])  # Extract the sensor value
                print(f"Received light sensor value: {sensor_value} from {addr}")
                # Control the LEDs based on the sensor value
                control_leds(sensor_value)
                last_message_time = time.time()
            
            except BlockingIOError:
                if time.time() - last_message_time > 10:
                    if not error_state:
                        print("No message received from ESP8266 for 10 seconds. Entering error state.")
                        error_state = True
                        # Start flashing the white LED in error state
                        flash_white_led()

        time.sleep(0.1)

except KeyboardInterrupt:
    print("Program interrupted by user")

finally:
    reset_system()
    sock.close()
    print("GPIO cleanup done.")