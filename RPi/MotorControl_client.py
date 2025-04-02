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
                        
                        # Convert to bytes format for CAN message
                        data = bytes(data_bytes)
                        
                        print(f"[Motor] Received CAN message - ID: {can_id}, Data: {data}")
                        
                        ######## COMMENT OUT FOR TESTING WITHOUT THE CAN BUS ########
                        # Execute the motor command by sending through CAN bus
                        # self.execute_motor_command(can_id, data)
                        #############################################################
                    else:
                        print(f"[Motor] Invalid command format: {response.command}")
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
    #         msg = can.Message(arbitration_id=node_id, data=data, is_extended_id=False)
    #         BUS.send(msg)
    #         logger.info(f"Sent CAN message to ID={node_id}, Data={data.hex()}")
    #     except can.CanError as e:
    #         logger.error(f"Failed to send CAN message: {e}")
    #     # GPIO.output(RED, GPIO.HIGH)
    #############################################################

if __name__ == "__main__":
    client = MotorControlClient()
    try:
        client.start()
    except KeyboardInterrupt:
        client.stop()