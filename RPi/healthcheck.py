import can
import struct
import RPi.GPIO as GPIO # RPi.GPIO is built for RPi only
import logging
import time

# Set up logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

# Initialization of CAN BUS Interface
CAN_INTERFACE = 'can0' # Check if can0 is the correct configuration - check with ifconfig or ip a
BUS = can.interface.Bus(CAN_INTERFACE, bustype = 'socketcan')

# Board Types for reference
# # Enum for board types
# BMS = 0
# ST = 1
# MC = 2
 
# # Define the board types for easier reference
# BOARD_TYPES = {
#     BMS: "BMS",
#     ST: "S&T",
#     MC: "MC"
# }

# Setup code for GPIO
GPIO.setwarnings(False)
RED = 17
GREEN = 27
BLUE = 22
GPIO.setmode(GPIO.BCM)
GPIO.setup(RED, GPIO.OUT)
GPIO.setup(GREEN, GPIO.OUT)
GPIO.setup(BLUE, GPIO.OUT)

# Define STM32 board arbitration IDs
STM32_BMS_ID = 0x67a                # STM32 Board 1 - BMS
STM32_ST_ID = 0x200                 # STM32 Board 2 - S&T
STM32_MC_ID = 0x300                 # STM32 Board 3 - Motor Controller
STM32_IDS=[0x100 , 0x200 , 0x300]   # List for all the stm32 ids
 
# Function to send CAN message
def send_can_message(node_id, data):
    try:
        msg = can.Message(arbitration_id=node_id, data=data, is_extended_id=False)
        BUS.send(msg)
        logger.info(f"Sent CAN message to ID={node_id}, Data={data}")
    except can.CanError as e:
        logger.error(f"Failed to send CAN message: {e}")
        GPIO.output(RED, GPIO.HIGH)
 
def send_telemetry():
    while True:
        send_can_message(STM32_BMS_ID, [0x01, 0x02, 0x03, 0x04])
        send_can_message(STM32_ST_ID,  [0x01, 0x02, 0x03, 0x04])
        send_can_message(STM32_MC_ID,  [0x01, 0x02, 0x03, 0x04])
        time.sleep(1)
 
def health_checkup():
    for node_id in STM32_IDS:
        try:
            send_can_message(node_id, [0x01, 0x02, 0x03, 0x04])  # Send the health check request
            msg = BUS.recv(timeout=1)  # Wait for a message within the timeout period
            if msg is None:
                logger.error(f"No response from STM32 with ID=0x{node_id:X}")
                error_indication_led()
                return False  # Avoid further processing for this node
            if msg.arbitration_id != node_id:
 
                logger.error(f"Unexpected message received from node ID=0x{msg.arbitration_id:X}.")
                error_indication_led()
                return False
            print(f"Received message from STM32: ID=0x{node_id:X}, Data={msg.data}")
            expected_packet=[0x01, 0x02, 0x03, 0x04]
            if msg.data == expected_packet:
                logger.info(f"STM32 ID=0x{node_id:X} Initialized successfully")
                success_indication_led()
            else:
                print(f"STM32 (ID=0x{node_id:X}) sent an unexpected response: {list(msg.data)}")
                error_indication_led()
        except can.CanError as e:
            logger.error(f"Error while receiving CAN message: {e}")
            error_indication_led()
            return False  # Added return for error case
 
def error_indication_led():
    # Turn on the error indication LED
    GPIO.output(RED, GPIO.HIGH)
 
def success_indication_led():
    # Turn off the error indication LED if it was on
    GPIO.output(RED, GPIO.LOW)
    # Turn on the success indication LED
    GPIO.output(GREEN, GPIO.HIGH)
    GPIO.cleanup()  # Clean up GPIO pinss
 
if __name__ == "__main__":
    send_telemetry()
    health_checkup()