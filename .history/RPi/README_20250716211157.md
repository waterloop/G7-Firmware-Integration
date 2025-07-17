# Waterloop-Host-Application
In development. Code for the distributed flight computer host application of Waterloop's Hyperloop Pod. This uses multiple clients and bidirectional streaming gRPCs to manage both the Telemetry and Motor Command Control State Machines located on the central Raspberry Pi as well as the web-hosted Dashboard. State Machines send and receive messages on STM32s through a CAN bus. Health Checks are conducted on through GPIO when the system starts.

Note: This version has only the implementation of the general host app architecture. Specific CAN message formats have not yet fully been implemented.

## Cloning Instructions
Before cloning the repo, ensure you:
- Install gRPC for Python
```
pip install grpcio
```
- Install gRPC tools for Python
```
pip install grpcio-tools
```
- Install CAN
```
pip install python-can
```
## Running the Host Application
If you are testing the application without CAN, first ensure the indicated lines in `MotorControl_client.py` are commented out.

Open a terminal and run:
```
python HostServer.py
```
This should open a local host. 

Run `Dashboard_client.py`, `Telemetry_client.py` and `MotorControl_client.py` in separate terminals.

When the clients are run, they are automatically registered to the server. The server should show the following:
```
[Server] Client registered: dashboard (type: dashboard)
[Server] Routing message: dashboard -> motor_control: Dashboard connected   
[Server] Client registered: motor_control (type: motor_control)
[Server] Routing message: motor_control -> server: Motor Control connected  
[Server] Client registered: telemetry (type: telemetry)
[Server] Routing message: telemetry -> dashboard: Telemetry connected
```
Note the startup messages will only be routed to open clients.
For the Telemetry client the initialization messages should be:
```
[Telemetry] Client starting with ID: telemetry
[Telemetry] Commands:
1. Type 'random' to send a random CAN message
2. Type 'can:ID:DATA' where ID is CAN ID (0-2047) and DATA is comma-separated bytes
   Example: can:123:10,20,30,40,50,60,70,80
3. Type 'exit' to quit
```
For the Dashboard client:
```
[Dashboard] Client starting with ID: dashboard
[Dashboard] Commands:
1. Motor Control Commands:
   - 'start:<throttle>' to start motor and set throttle (1-100)
   - 'stop' to stop motor
   - 'throttle:<value>' to set throttle (1-100)
   - 'forward', 'fwd' or 'f' to set direction to forward
   - 'reverse', 'rev' or 'r' to set direction to reverse
2. Type 'exit' to quit
```
For the Motor Control client:
```
[Motor] Client starting with ID: motor_control
[Motor] Waiting for commands...
```
## Sending Messages ##
In this version of the implementation, CAN messages have to be manually entered to test the server routing. 

### MC-Dashboard
To send a motor control command from the dashboard, enter a command in the CAN format in the dashboard client.

1. Enter `start:<throttle>` to start the MC with a throttle value (integer between 1-100)
2. Enter `stop` to stop the MC
3. Enter `throttle:<value>` to set the throttle value (integer between 1-100)
4. Enter `forward`, `fwd`, or `f` to set the direction to forward
5. Enter `reverse`, `rev`, or `r` to set the direction to reverse

For example, if you entered in the Dashboard:
```
> start:90 
[Dashboard] Starting motor with 90% throttle
[Dashboard] Sending CAN message: motor:291:34,90
```
The server will update accordingly:
```
[Server] Received command from Dashboard: motor:291:34,90
[Server] Routing message: dashboard -> motor_control: motor:291:34,90
[Server] Sending to motor control: motor:291:34,90
```
When testing without the CAN bus, the motor control will receive:
```
[Motor] Received MC_START command - CAN ID: 0x123, Data: 225A
[Motor] Starting motor with 90% throttle
[Motor] Would send CAN message: ID=0x123, Data=225A
```
### Telemetry-Dashboard
To manually send telemetry updates for testing, enter a message in CAN format in the telemetry client. NOTE: Currently, there are no set IDs for the Telemetry, so the only messages you can send are random:

1. Enter `random` to send a random Telemetry CAN message")
2. Enter `can:ID:DATA` where ID = CAN ID (0-2047) and DATA is comma-separated bytes"). 

   Example: `can:123:10,20,30,40,50,60,70,80`

Example:
```
> can:000:00,00,00,00,00,00,00,00
INFO:__main__:Received CAN message with ID=000, Data=00,00,00,00,00,00,00,00
INFO:__main__:Telemetry Data type: IMU
[Telemetry] Received CAN message from bus: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
[Telemetry] Sending CAN message: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
```
The server will update accordingly:
```
[Server] Received telemetry: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
[Server] Routing message: telemetry -> dashboard: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
[Server] Sending to dashboard: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
```
The dashboard will receive:
```
[Dashboard] Received from telemetry: Telemetry: IMU: X_ACCEL=0, Y_ACCEL=0, X_GYRO=0, Y_GYRO=0, Z_GYRO=0, ERROR_ID=0
```
