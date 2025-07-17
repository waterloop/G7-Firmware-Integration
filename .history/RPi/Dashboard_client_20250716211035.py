# VERSION 10: with CAN
import can
import grpc
import threading
import host_pb2
import host_pb2_grpc
import logging
import random

class DashboardClient:
    def __init__(self, client_id="dashboard"):
        self.client_id = client_id
        self.channel = grpc.insecure_channel('localhost:50051')
        self.stub = host_pb2_grpc.HostControlStub(self.channel)
        self._running = False
        self._stop_event = threading.Event()
        self.message_queue = []
        self.queue_lock = threading.Lock()
        
    def command_stream(self):
        """Generate motor commands based on user input"""
        # Initial message to establish the stream
        yield host_pb2.HostMessage(
            sender=self.client_id,
            recipient="motor_control",
            command="Dashboard connected" # this doesn't send to motor controller properly, but it isn't necessary
        )
        
        while not self._stop_event.is_set():
            # Check if there are messages to send
            with self.queue_lock:
                if self.message_queue:
                    command = self.message_queue.pop(0)
                    message = host_pb2.HostMessage(
                        sender=self.client_id,
                        recipient="motor_control",
                        command=command  # Command format is already prepared
                    )
                    print(f"[Dashboard] Sending CAN message: {command}") # command holds the motor control data
                    yield message
    
    def process_responses(self, response_iterator):
        """Process responses from the server (telemetry updates)"""
        for response in response_iterator:
            print(f"[Dashboard] Received from {response.sender}: {response.command}") # command holds the telemetry data
    
    def input_loop(self):
        """Handle user input from terminal"""
        print("[Dashboard] Commands:")
        print("1. Motor Control Commands:")
        print("   - 'start:<throttle>' to start motor and set throttle (1-100)")
        print("   - 'stop' to stop motor")
        print("   - 'throttle:<value>' to set throttle (1-100)")
        print("   - 'forward', 'fwd' or 'f' to set direction to forward")
        print("   - 'reverse', 'rev' or 'r' to set direction to reverse")
        print("2. Type 'exit' to quit")
        
        while not self._stop_event.is_set():
            try:
                user_input = input("> ")
                if user_input.lower() == 'exit':
                    self.stop()
                    break
    
                # Motor Control Commands (using correct CAN ID 0x123 = 291 from can.c)
                elif user_input.lower().startswith('start:'):
                    parts = user_input.split(':')
                    if len(parts) == 2:
                        try:
                            throttle = int(parts[1])
                            if 1 <= throttle <= 100:  # Changed to 1-100 range
                                command = f"motor:291:34,{throttle}"  # MC_START (0x22 = 34) + throttle value
                                print(f"[Dashboard] Starting motor with {throttle}% throttle")
                            else:
                                print("[Dashboard] Throttle must be 1-100")
                                continue
                        except ValueError:
                            print("[Dashboard] Invalid throttle value")
                            continue
                    else:
                        print("[Dashboard] Invalid format. Use start:THROTTLE")
                        continue
                    
                elif user_input.lower() == 'stop':
                    command = f"motor:291:17"  # MC_STOP (0x11 = 17)
                    print("[Dashboard] Stopping motor")

                elif user_input.lower().startswith('throttle:'):
                    parts = user_input.split(':')
                    if len(parts) == 2:
                        try:
                            throttle = int(parts[1])
                            if 1 <= throttle <= 100:  # Changed to 1-100 range
                                command = f"motor:291:51,{throttle}"  # MC_THROTTLE (0x33 = 51) + throttle value
                                print(f"[Dashboard] Setting throttle to {throttle}%")
                            else:
                                print("[Dashboard] Throttle must be 1-100")
                                continue
                        except ValueError:
                            print("[Dashboard] Invalid throttle value")
                            continue
                    else:
                        print("[Dashboard] Invalid format. Use throttle:VALUE")
                        continue

                elif user_input.lower() in ['forward', 'fwd', 'f']:
                    command = f"motor:291:68,1"  # MC_DIRECTION (0x44 = 68), 1 = forward
                    print("[Dashboard] Setting direction to FORWARD")

                elif user_input.lower() in ['reverse', 'rev', 'r']:
                    command = f"motor:291:68,0"  # MC_DIRECTION (0x44 = 68), 0 = reverse
                    print("[Dashboard] Setting direction to REVERSE")

                else:
                    print("[Dashboard] Invalid command")
                    continue
        
                # Add the command to the message queue
                with self.queue_lock:
                    self.message_queue.append(command)
            
            except EOFError:
                break
    
    def start(self):
        """Start the dashboard client"""
        self._running = True
        self._stop_event.clear()
        print(f"[Dashboard] Client starting with ID: {self.client_id}")
        
        # Start input thread
        input_thread = threading.Thread(target=self.input_loop)
        input_thread.daemon = True
        input_thread.start()
        
        try:
            # Start bidirectional streaming
            responses = self.stub.CommandStream(self.command_stream())
            self.process_responses(responses)
        except grpc.RpcError as e:
            print(f"[Dashboard] RPC error: {e}")
        finally:
            self._running = False
            self.channel.close()
    
    def stop(self):
        """Stop the dashboard client"""
        self._stop_event.set()
        print("[Dashboard] Client stopping...")

if __name__ == "__main__":
    client = DashboardClient()
    try:
        client.start()
    except KeyboardInterrupt:
        client.stop()