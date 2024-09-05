import asyncio
import websockets
import json
import random
import time
from zeroconf import Zeroconf, ServiceInfo
import socket

# Initialize counters for ids
fuel_management_id_counter = 1  # Starting id for FuelManagement
sensor_data_id_counter = 1  # Starting id for SensorData

# Function to update FuelManagement and SensorData based on specified requirements
def update_engine_data(engine):
    if engine['enabled'] == '1':
        # Update FuelManagement data
        engine['FuelManagement']['distance_to_empty'] = str(float(engine['FuelManagement']['distance_to_empty']) - 1)
        engine['FuelManagement']['id'] = str(int(engine['FuelManagement']['id']) + 1)
        engine['FuelManagement']['remaining_fuel'] = str(float(engine['FuelManagement']['remaining_fuel']) - 1)
        engine['FuelManagement']['total_fuel_burned'] = str(float(engine['FuelManagement']['total_fuel_burned']) + 1)
        
        # Update SensorData
        engine['SensorData']['fuel'] = str(float(engine['SensorData']['fuel']) - 1)  # Decrease fuel
        engine['SensorData']['id'] = str(int(engine['SensorData']['id']) + 1)  # Increase ID by 1
        engine['SensorData']['speed'] = str(round(random.uniform(50.0, 70.0), 6))  # Random speed between 50 and 70
        engine['SensorData']['temperature_return'] = str(round(random.uniform(20.0, 33.0), 6))  # Random temp between 20 and 33
        engine['SensorData']['temperature_supply'] = str(round(random.uniform(20.0, 33.0), 6))  # Random temp between 20 and 33
        engine['SensorData']['timestamp_unix'] = str(int(time.time()))  # Update timestamp to current time

# JSON data packet template
data_packet_template = {
    "data": {
        "Engines": [
            {
                "enabled": "1",
                "engine_type": "Type 1",
                "fuel_type": "Type 1",
                "id": "1",
                "name": "Engine M1",
                "position": "Port",
                "volume_per_cycle": "0.5",
                "FuelManagement": {
                    "distance_to_empty": "500",
                    "engine_id": "1",
                    "id": "1",
                    "remaining_fuel": "75",
                    "total_fuel_burned": "89"
                },
                "SensorData": {
                    "engine_id": "1",
                    "fuel": "75",
                    "id": "54",
                    "speed": "62.64727",
                    "temperature_return": "22.234875",
                    "temperature_supply": "21.718935",
                    "timestamp_unix": "132014061"
                }
            },
            {
                "enabled": "1",
                "engine_type": "Type 2",
                "fuel_type": "Type 2",
                "id": "2",
                "name": "Engine M2",
                "position": "Starboard",
                "volume_per_cycle": "1.0",
                "FuelManagement": {
                    "distance_to_empty": "400",
                    "engine_id": "2",
                    "id": "1",
                    "remaining_fuel": "68",
                    "total_fuel_burned": "97.300003"
                },
                "SensorData": {
                    "engine_id": "2",
                    "fuel": "68",
                    "id": "54",
                    "speed": "62.64727",
                    "temperature_return": "22.234875",
                    "temperature_supply": "21.718935",
                    "timestamp_unix": "132014061"
                }
            }
        ]
    }
}

# Set of connected clients
connected_clients = {}
connected_clients1 = {}

# Function to create a deep copy of the data packet template for each client
def create_new_packet():
    return json.loads(json.dumps(data_packet_template))

async def register(websocket):
    # Create a unique data packet for the client
    client_data_packet = create_new_packet()
    connected_clients[websocket] = client_data_packet
    print(f"timesp1.local Client connected: {websocket.remote_address}")

async def unregister(websocket):
    connected_clients.pop(websocket, None)
    print(f"timesp1.local Client disconnected: {websocket.remote_address}")

async def register1(websocket):
    # Create a unique data packet for the client
    client_data_packet = create_new_packet()
    connected_clients1[websocket] = client_data_packet
    print(f"timesp2.local Client connected: {websocket.remote_address}")

async def unregister1(websocket):
    connected_clients1.pop(websocket, None)
    print(f"timesp2.local Client disconnected: {websocket.remote_address}")

async def send_data():
    update_interval = 5  # Update values every 5 seconds
    send_interval = 1  # Send JSON packet every 1 second
    counter = 0

    while True:
        # Update engine data every 'update_interval' seconds
        if counter % update_interval == 0:
            for client, data_packet in connected_clients.items():
                for engine in data_packet['data']['Engines']:
                    update_engine_data(engine)
            print(f"Updated data packet for all clients on timesp1.local domain at {time.strftime('%X')}")

        # Send the updated data to all connected clients
        for client, data_packet in connected_clients.copy().items():
            try:
                # Convert the data packet to a JSON string
                json_data = json.dumps(data_packet)
                await client.send(json_data)
                print(f"Sent data to timesp1.local client {client.remote_address} at {time.strftime('%X')}.")
                # print(f"Sent data to {client.remote_address} at {time.strftime('%X')}: {json_data}")
            except websockets.exceptions.ConnectionClosed:
                await unregister(client)

        # Wait for 1 second before sending the next packet
        await asyncio.sleep(send_interval)
        counter += send_interval

async def send_data1():
    update_interval = 5  # Update values every 5 seconds
    send_interval = 1  # Send JSON packet every 1 second
    counter = 0

    while True:
        # Update engine data every 'update_interval' seconds
        if counter % update_interval == 0:
            for client, data_packet in connected_clients1.items():
                for engine in data_packet['data']['Engines']:
                    update_engine_data(engine)
            print(f"Updated data packet for all clients on timesp2.local domain at {time.strftime('%X')}")

        # Send the updated data to all connected clients
        for client, data_packet in connected_clients1.copy().items():
            try:
                # Convert the data packet to a JSON string
                json_data = json.dumps(data_packet)
                await client.send(json_data)
                print(f"Sent data to timesp2.local client {client.remote_address} at {time.strftime('%X')}.")
                # print(f"Sent data to {client.remote_address} at {time.strftime('%X')}: {json_data}")
            except websockets.exceptions.ConnectionClosed:
                await unregister1(client)

        # Wait for 1 second before sending the next packet
        await asyncio.sleep(send_interval)
        counter += send_interval

async def handle_client(websocket, path):
    await register(websocket)
    try:
        # Keep the connection open to send periodic updates
        await websocket.wait_closed()
    finally:
        await unregister(websocket)

async def handle_client1(websocket, path):
    await register1(websocket)
    try:
        # Keep the connection open to send periodic updates
        await websocket.wait_closed()
    finally:
        await unregister1(websocket)

# Function to advertise service using mDNS
def start_mdns_service():
    zeroconf = Zeroconf()
    zeroconf1 = Zeroconf()
    local_ip = socket.gethostbyname(socket.gethostname())  # Get the IP address of the current machine
    service_info0 = ServiceInfo(
        "_http._tcp.local.",
        "My WebSocket Server._http._tcp.local.",
        addresses=[socket.inet_aton(local_ip)],
        port=80,  # Ensure that mDNS advertises the same port as the WebSocket server
        properties={},
        server="timesp1.local.",
    )
    service_info1 = ServiceInfo(
        "_http._tcp.local.",
        "My WebSocket Server1._http._tcp.local.",
        addresses=[socket.inet_aton(local_ip)],
        port=81,  # Ensure that mDNS advertises the same port as the WebSocket server
        properties={},
        server="timesp2.local.",
    )
    zeroconf.register_service(service_info0)
    print(f"mDNS service started at timesp1.local on {local_ip}")
    zeroconf1.register_service(service_info1)
    print(f"mDNS service started at timesp2.local on {local_ip}")

async def main():
    # Start the WebSocket server for timesp1.local on port 80
    server1 = websockets.serve(handle_client, "0.0.0.0", 80)

    # Start the WebSocket server for timesp2.local on port 81
    server2 = websockets.serve(handle_client1, "0.0.0.0", 81)

    # Start both servers concurrently
    await asyncio.gather(
        server1,
        server2,
        send_data(),
        send_data1()
    )

if __name__ == "__main__":
    # Set the event loop policy for Windows compatibility
    if asyncio.get_event_loop_policy().__class__.__name__ != 'WindowsSelectorEventLoopPolicy':
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    
    # Start mDNS service
    start_mdns_service()

    asyncio.run(main())
