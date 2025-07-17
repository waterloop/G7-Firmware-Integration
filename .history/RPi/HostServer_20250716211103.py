import grpc
from concurrent import futures
import host_pb2
import host_pb2_grpc
import threading
import queue
import time

class MessageRouter:
    """Ensures that messages can be sent from one client to another"""

    def __init__(self):
        # Dictionary to store message queues for each client
        # Key: client_id (string), Value: queue of messages
        self.client_queues = {}
        self.lock = threading.Lock()
    

    def register_client(self, client_id, client_type):
        """Connects any client to the server using a unique client id"""
        with self.lock:
            if client_id not in self.client_queues:
                self.client_queues[client_id] = queue.Queue()
                print(f"[Server] Client registered: {client_id} (type: {client_type})")
                return True
            return False
    

    def unregister_client(self, client_id):
        """Removes any client from the server. Useful for the final host application."""
        with self.lock:
            if client_id in self.client_queues:
                del self.client_queues[client_id]
                print(f"[Server] Client unregistered: {client_id}")
    

    def route_message(self, message):
        """General message routing system."""
        sender = message.sender
        recipient = message.recipient
        
        print(f"[Server] Routing message: {sender} -> {recipient}: {message.command}")
        
        # essential to multithreaded applications,protects access to subscribers (telemetry, motor control, dashboard)
        with self.lock:     
            # Queue the message for the recipient
            if recipient in self.client_queues:
                self.client_queues[recipient].put(message)
                return True
            elif recipient == "broadcast":
                # Queue for all clients except sender
                for client_id, q in self.client_queues.items():
                    if client_id != sender:
                        q.put(message)
                return True
            return False
    

    def get_message(self, client_id, timeout=0.1):
        """Get next message for a client, or None if no message is available"""
        if client_id in self.client_queues:
            try:
                return self.client_queues[client_id].get(block=False)
            except queue.Empty:
                return None
        return None


class HostControlServicer(host_pb2_grpc.HostControlServicer):
    """gRPC Control Servicer"""
    def __init__(self):
        self.router = MessageRouter()
        self.active_streams = set()
        self.lock = threading.Lock()

    def TelemetryStream(self, request_iterator, context):
        """Stream for telemetry clients to send updates that get forwarded to dashboard"""
        # Wait for first message to identify the client
        try:
            first_message = next(request_iterator)
            client_id = first_message.sender
            self.router.register_client(client_id, "telemetry")
            
            # Process first message
            self.router.route_message(first_message)
            
            # Create a thread to continuously check for messages to send back to this client
            def send_messages():
                while context.is_active():
                    message = self.router.get_message(client_id)
                    if message:
                        yield message           # S.J: gRPC part
                    time.sleep(0.01)  # Small delay to avoid busy-waiting (s.j.: is this really needed?)
            
            # Create and start the message sending thread
            message_thread = threading.Thread(target=lambda: None)
            message_thread.daemon = True
            
            # Process remaining incoming messages
            for message in request_iterator:
                print(f"[Server] Received telemetry: {message.command}")
                # Route the message (typically to dashboard)
                self.router.route_message(message)
                
                # Check for any messages to send back
                response = self.router.get_message(client_id)
                if response:
                    yield response
            
        except Exception as e:
            print(f"[Server] Error in TelemetryStream: {e}")
        finally:
            self.router.unregister_client(client_id)

    def CommandStream(self, request_iterator, context):
        """Stream for dashboard to send commands to motor control"""
        try:
            first_message = next(request_iterator)
            client_id = first_message.sender
            self.router.register_client(client_id, "dashboard")
            
            # Process first message (route to motor control)
            self.router.route_message(first_message)
            
            # Create a queue of messages to send back to dashboard
            response_queue = queue.Queue()
            
            # Handle incoming messages from dashboard
            def process_incoming():
                try:
                    # First message already processed
                    for message in request_iterator:
                        print(f"[Server] Received command from Dashboard: {message.command}")
                        self.router.route_message(message)
                except Exception as e:
                    print(f"[Server] Error processing dashboard commands: {e}")
            
            # Start thread to handle incoming messages
            incoming_thread = threading.Thread(target=process_incoming)
            incoming_thread.daemon = True
            incoming_thread.start()
            
            # Send messages to dashboard client
            while context.is_active():
                message = self.router.get_message(client_id)
                if message:
                    print(f"[Server] Sending to dashboard: {message.command}")
                    yield message
                time.sleep(0.01)  # Small delay to avoid busy-waiting
                
        except Exception as e:
            print(f"[Server] Error in CommandStream: {e}")
        finally:
            self.router.unregister_client(client_id)

    def MotorControlStream(self, request_iterator, context):
        """Stream for motor control client to receive commands and send status updates"""
        try:
            first_message = next(request_iterator, None)
            if first_message:
                client_id = first_message.sender
            else:
                # If no first message, generate a unique ID
                client_id = f"motor_control_{id(context)}"
                
            self.router.register_client(client_id, "motor_control")
            
            # If there was a first message, process it
            if first_message:
                self.router.route_message(first_message)
            
            # Handle incoming messages from motor control (if any)
            def process_incoming():
                try:
                    for message in request_iterator:
                        print(f"[Server] Received from Motor Control: {message.command}")
                        self.router.route_message(message)
                except Exception as e:
                    print(f"[Server] Error processing motor control messages: {e}")
            
            # Start thread to handle incoming messages
            incoming_thread = threading.Thread(target=process_incoming)
            incoming_thread.daemon = True
            incoming_thread.start()
            
            # Send commands to motor control client
            while context.is_active():
                message = self.router.get_message(client_id)
                if message:
                    print(f"[Server] Sending to motor control: {message.command}")
                    yield message
                time.sleep(0.01)  # Small delay to avoid busy-waiting
                
        except Exception as e:
            print(f"[Server] Error in MotorControlStream: {e}")
        finally:
            self.router.unregister_client(client_id)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    host_pb2_grpc.add_HostControlServicer_to_server(HostControlServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    print("Server running on port 50051")
    server.wait_for_termination()

if __name__ == '__main__':
    serve()