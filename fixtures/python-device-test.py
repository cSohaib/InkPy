# Copy to SD, edit Wi-Fi credentials, then Execute. No command-line arguments.
import os, json, inkpy, time

value = input("Value: ")
assert isinstance(value, str)
print("Received:", value)
with open("inkpy-input-test.txt", "w") as f:
    f.write(value + "\n")
with open("inkpy-input-test.txt", "a") as f:
    f.write("append works\n")
with open("inkpy-input-test.txt") as f:
    print(f.read())

assert json.loads('{"value":42}')['value'] == 42
with open("inkpy-json-test.json", "w") as f:
    json.dump({"value": value}, f)
with open("inkpy-json-test.json") as f:
    assert json.load(f)["value"] == value
print("Input/files/JSON passed")

# Set current date/time in Power before HTTPS. Leave these empty to skip Wi-Fi.
SSID = ""
PASSWORD = ""
if SSID:
    inkpy.wifi(SSID, PASSWORD)
    try:
        for attempt in range(150):
            connected, ip = inkpy.wifi_status()
            if connected:
                break
            time.sleep(0.1)
        if not connected:
            raise OSError("Wi-Fi connection failed")
        destination = "inkpy-http-test.json"
        if destination in os.listdir():
            os.remove(destination)
        status = inkpy.http("https://jsonplaceholder.typicode.com/todos/1", destination)
        print("HTTP:", status)
        with open(destination) as f:
            print(json.load(f))
    finally:
        inkpy.wifi_off()
