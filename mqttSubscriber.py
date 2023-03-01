# Import necessary libraries
import paho.mqtt.client as mqttClient  # MQTT client library
from Crypto.Util import strxor  # Byte manipulation library
import time  # Time library
import json  # JSON library
import base64  # Base64 decoding library
import codecs  # Hex decoding library
from Crypto.Cipher import AES  # AES encryption library

# Callback function for when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected  # Use global variable to store the state of the connection
        Connected = True  # Signal successful connection
    else:
        print("Connection failed")

# Callback function for when a message is received on the subscribed topic
def on_message(client, userdata, msg):
    # Decode the message payload as UTF-8
    payload = msg.payload.decode("utf-8")
    # Convert the JSON string to a dictionary
    json_dict_payload = json.loads(payload)
    # Get the temperature value from the "payload" key of the dictionary
    temperature = json_dict_payload.get("payload")
    # Decode the temperature value from base64 to bytes
    decoded_temp = base64.b64decode(temperature)

    # Print the encrypted temperature value in hexadecimal
    print("Encrypted Temp : ", int(decoded_temp.hex(), 16))
    # Decrypt the temperature value using XOR and the AES key, and print the decrypted value in hexadecimal
    temp_dcd = strxor.strxor_c(decoded_temp, AesKey[0])
    print("Decrypted Temp : ", int(temp_dcd.hex(), 16))

# Initialize the state of the connection to the broker as False
Connected = False

# Set the address, port, and topic of the MQTT broker
broker_address = "198.27.70.149"  # Broker address
port = 443  # Broker port
topic = "isima/yahya"  # Connection username

# Decode the AES key and initialization vector from hexadecimal to bytes
key = codecs.decode('f6c7d9e50f8a1b3b3e6b2d6f799077c2', 'hex_codec')
iv = codecs.decode('abcdef123456789abcdeF01122334455', 'hex_codec')

# Create an AES cipher object with the key and initialization vector, and encrypt the initialization vector to generate the AES key
cipher = AES.new(key, AES.MODE_ECB)
AesKey = cipher.encrypt(iv)

# Create a new MQTT client instance
client = mqttClient.Client("Python")
client.topic = topic  # Set the client's topic
client.on_connect = on_connect  # Set the callback function for when the client connects to the broker
client.on_message = on_message  # Set the callback function for when a message is received on the subscribed topic

# Connect to the MQTT broker
client.connect(broker_address, port=port)

# Start the MQTT client loop
client.loop_start()        

#Wait for connection
while Connected != True:   
    time.sleep(0.1)
  
client.subscribe("isima/yahya")
  
try:
    while True:
        time.sleep(1)
  
except KeyboardInterrupt:
    print( "exiting")
    client.disconnect()
    client.loop_stop()