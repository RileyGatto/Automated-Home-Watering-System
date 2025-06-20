
# Library imports for MQTT and Tkinter GUI
import paho.mqtt.client as mqtt
from tkinter import *
from tkinter import font

# MQTT SERVER IP which is also running on the raspberry pi and the moisture threshold
SERVER_IP = "127.0.0.1"  
MOISTURE_THRESHOLD = 400

# This function is responsible for when the client connects to the MQTT Server and subscribes to the topic "plant/moisture".
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker")
        client.subscribe("plant/moisture")
    else:
        print("Failed to connect")

# This function is responsible for uopdating the gui with the new moisture level and also publishing the mode of the plant to the topic "plant/mode"/
def on_message(client, userdata, message):
    moisture = int(message.payload.decode())
    moisture_label.config(text=f"Moisture Level: {moisture}")
    if auto_mode.get():
        client.publish("plant/mode", "AUTO")
    else:
        client.publish("plant/mode", "MANUAL")

# This function is responsible for manually watering the plant when the moisture level is above the threshold.
def manual_water():
    moisture = int(moisture_label.cget("text").split(": ")[1]) # gets moisture level from the label
    if moisture <= MOISTURE_THRESHOLD:
        status_label.config(text="Soil Too Wet")
    else:
        client.publish("plant/water", "ON")
        status_label.config(text="Watering Plant")

# This function is responsible for setting up the mqtt client and connecting to the mqtt server.
def setup_mqtt():
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(SERVER_IP, 1883, 60)
    client.loop_start()

# This function is responsible for hiding the manual button when the automatic mode is enabled.
def update_button_visibility():
    if auto_mode.get():
        manual_button.pack_forget()
    else:
        manual_button.pack(pady=20)

# Initialize Tkinter GUI
win = Tk()
win.title("Plant Watering System")
win.geometry("500x500")
win.config(bg="white")

# Set font styles
title_font = font.Font(family="Courier", size=18, weight="bold")
label_font = font.Font(family="Courier", size=14)
button_font = font.Font(family="Courier", size=14)

# Create GUI elements
auto_mode = BooleanVar(value=True)  # Set to True by default so auto mode is enabled automatically
moisture_label = Label(win, text="Moisture Level: ---", font=label_font, bg="white")
status_label = Label(win, text="Status: Idle", font=label_font, bg="white")
manual_button = Button(win, text="Water Plant", font=button_font, command=manual_water, bg="green", fg="white")
auto_checkbox = Checkbutton(win, text="Automatic Mode", variable=auto_mode, font=label_font, bg="white", command=update_button_visibility)

# Arrange GUI elements with padding
moisture_label.pack(pady=10)
status_label.pack(pady=10)
auto_checkbox.pack(pady=10)
update_button_visibility()

# Setup MQTT client
client = mqtt.Client()
setup_mqtt()

# Start Tkinter main loop
win.mainloop()