from zeroconf import Zeroconf, ServiceBrowser, ServiceStateChange, ServiceInfo

def on_service_state_change(zeroconf, service_type, name, state_change):
    """Callback function to handle service state changes (found, updated, or removed)."""
    if state_change is ServiceStateChange.Added:
        print(f"Service {name} found. Resolving...")
        info = zeroconf.get_service_info(service_type, name)
        if info:
            resolve_service_info(info)
        else:
            print(f"Failed to resolve {name}.")

def resolve_service_info(service_info: ServiceInfo):
    """Resolve and display detailed service information."""
    print(f"\nResolved service: {service_info.name}")
    
    # Extract service details
    addresses = [addr for addr in service_info.parsed_addresses()]
    port = service_info.port
    properties = service_info.properties

    # Display resolved details
    print(f"Addresses: {addresses}")
    print(f"Port: {port}")
    print(f"Properties: {properties}\n")

def main():
    zeroconf = Zeroconf()

    # Specify the service type you are interested in (e.g., "_http._tcp.local." for HTTP services)
    service_type = "_websocket._tcp.local."

    print(f"Discovering services of type {service_type}...\n")
    
    # Create a service browser to discover services of the specified type
    browser = ServiceBrowser(zeroconf, service_type, handlers=[on_service_state_change])

    try:
        input("Press any key to exit...\n")
    finally:
        zeroconf.close()

if __name__ == "__main__":
    main()
