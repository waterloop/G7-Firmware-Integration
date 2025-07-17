import can
import grpc
import threading
import host_pb2
import host_pb2_grpc
import logging
import struct

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

######## COMMENT OUT FOR TESTING WITHOUT CAN BUS ########
# CAN_INTERFACE = 'can0' # Check if can0 is the correct configuration - check with ifconfig or ip a
# BUS = can.interface.Bus(CAN_INTERFACE, interface= 'socketcan')
#########################################################


class MotorControlClient:
    def __init__(self, client_id="motor_control"):
        self.client_id = client_id
        self.channel = grpc.insecure_channel('localhost:50051')
        self.stub = host_pb2_grpc.HostControlStub(self.channel)
        self._running = False
        self._stop_event = threading.Event()
    
    def empty_stream(self):
        """Generate an initial message to establish the stream"""
        # Send only an initial message to establish the connection
        yield host_pb2.HostMessage(
            sender=self.client_id,
            recipient="server",
            command="Motor Control connected"
        )
        
        # Keep the client connection alive
        while not self._stop_event.is_set():
            # We don't send additional messages in this implementation
            # But we need to keep the generator alive
            if self._stop_event.wait(10):  # Check stop flag every 10 seconds
                break
    
    def process_commands(self, response_iterator):
        """Process command messages from the server (from dashboard)"""
        print("[Motor] Waiting for commands...")
        for response in response_iterator:
            if response.sender == "dashboard" and response.command.startswith("motor:"):
                try:
                    # Parse the CAN message details from the command
                    # Expected format: motor:can_id:byte1,byte2,byte3,...
                    command_parts = response.command.split(":")
                    if len(command_parts) >= 3:
                        can_id = int(command_parts[1])
                        data_bytes = [int(b) for b in command_parts[2].split(",")]
                        
                        # Validate CAN ID range
                        if not (0 <= can_id <= 2047):
                            print(f"[Motor] Invalid CAN ID: {can_id} (must be 0-2047)")
                            continue
                        
                        # Validate data length (CAN frames can have 0-8 bytes)
                        if len(data_bytes) > 8:
                            print(f"[Motor] Data too long: {len(data_bytes)} bytes (max 8)")
                            continue
                        
                        # Validate byte values (0-255)
                        if any(b < 0 or b > 255 for b in data_bytes):
                            print(f"[Motor] Invalid byte values: {data_bytes} (must be 0-255)")
                            continue
                        
                        # Parse motor command type for logging
                        command_type = "UNKNOWN"
                        if len(data_bytes) > 0:
                            cmd_byte = data_bytes[0]
                            command_map = {
                                17: "MC_STOP",      # 0x11
                                34: "MC_START",     # 0x22  
                                51: "MC_THROTTLE",  # 0x33
                                68: "MC_DIRECTION"  # 0x44
                            }
                            command_type = command_map.get(cmd_byte, f"UNKNOWN_CMD_{cmd_byte}")
                        
                        # Convert to bytes format for CAN message
                        data = bytes(data_bytes)
                        
                        print(f"[Motor] Received {command_type} command - CAN ID: 0x{can_id:03X}, Data: {data.hex().upper()}")
                        
                        # Log specific command details
                        if len(data_bytes) > 0:
                            if data_bytes[0] == 34 and len(data_bytes) >= 2:  # MC_START (0x22)
                                print(f"[Motor] Starting motor with {data_bytes[1]}% throttle")
                            elif data_bytes[0] == 17:  # MC_STOP (0x11)
                                print(f"[Motor] Stopping motor")
                            elif data_bytes[0] == 51 and len(data_bytes) >= 2:  # MC_THROTTLE (0x33)
                                print(f"[Motor] Setting throttle to {data_bytes[1]}%")
                            elif data_bytes[0] == 68 and len(data_bytes) >= 2:  # MC_DIRECTION (0x44)
                                direction = "FORWARD" if data_bytes[1] == 1 else "REVERSE"
                                print(f"[Motor] Setting motor direction to {direction}")
                        
                        ######## COMMENT OUT FOR TESTING WITHOUT THE CAN BUS ########
                        # Execute the motor command by sending through CAN bus
                        # self.execute_motor_command(can_id, data)  
                        #############################################################
                        
                        # For testing without CAN bus, simulate the action
                        print(f"[Motor] Would send CAN message: ID=0x{can_id:03X}, Data={data.hex().upper()}")
                    
                    else:
                        print(f"[Motor] Invalid command format: {response.command}")
                except ValueError as e:
                    print(f"[Motor] Error parsing command values: {e}")
                except Exception as e:
                    print(f"[Motor] Error processing command: {e}")
    
    def start(self):
        """Start the motor control client"""
        self._running = True
        self._stop_event.clear()
        print(f"[Motor] Client starting with ID: {self.client_id}")
        
        try:
            # Start bidirectional streaming but only care about responses
            responses = self.stub.MotorControlStream(self.empty_stream())
            self.process_commands(responses)
        except grpc.RpcError as e:
            print(f"[Motor] RPC error: {e}")
        finally:
            self._running = False
            self.channel.close()
    
    def stop(self):
        """Stop the motor control client"""
        self._stop_event.set()
        print("[Motor] Client stopping...")

    ######## COMMENT OUT FOR TESTING WITHOUT THE CAN BUS ########
    # def execute_motor_command(self, node_id, data):
    #     try:
    #         # Create CAN message with proper formatting
    #         msg = can.Message(
    #             arbitration_id=node_id,
    #             data=data,
    #             is_extended_id=False,
    #             dlc=len(data)  # Data Length Code
    #         )
            
    #         # Send the CAN message
    #         BUS.send(msg)
    #         logger.info(f"Sent CAN message: ID=0x{node_id:03X}, Data={data.hex().upper()}, DLC={len(data)}")
            
    #         # Log command type for debugging (UPDATED with correct command codes, IDLE removed)
    #         if len(data) > 0:
    #             cmd_type = {17: "STOP", 34: "START", 51: "THROTTLE", 68: "DIRECTION"}.get(data[0], "UNKNOWN")
    #             logger.info(f"Motor command executed: {cmd_type}")
                
    #     except can.CanError as e:
    #         logger.error(f"Failed to send CAN message: {e}")
    #         # GPIO.output(RED, GPIO.HIGH)  # Error indication
    #     except Exception as e:
    #         logger.error(f"Unexpected error sending CAN message: {e}")
    #############################################################

if __name__ == "__main__":
    client = MotorControlClient()
    try:
        client.start()
    except KeyboardInterrupt:
        client.stop()