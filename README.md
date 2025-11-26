Smart Inverter-Based Load & Heating Management
Central Heating + Domestic Hot Water (DHW) Automation

This project contains a distributed ESP32-based system that manages two separate heating circuits using real-time data from a photovoltaic inverter:

    Central Heating Circuit (water used for radiators)
  
    Domestic Hot Water (DHW) System (water used for showering, washing, etc.)
  
The goal is to maximize solar energy usage and prevent drawing power from the electrical grid.


Project Structure:
✔ 1. inverter-communication.ino
Main controller — manages the Central Heating Circuit

This ESP32 communicates directly with the solar inverter and obtains:

current PV production

available surplus energy

grid import/export power

overall inverter status

Based on this data, it:

controls the heating elements of the Central Heating Circuit (radiators)

determines how much surplus energy remains available for heating domestic hot water

sends the calculated maximum allowed heating power to the Boiler Heating Controller

This unit makes all real-time energy decisions and ensures the system never exceeds available solar power.

✔ 2. temp-measuring.ino
Temperature monitoring for the DHW boiler

This ESP32 measures the temperature of the domestic hot-water boiler using sensors such as DS18B20.

It periodically sends the measured temperature to the Boiler Heating Controller.

Responsibilities:

precise water-temperature measurement

stable sensor communication

sending temperature data at regular intervals

This device does not control any relays — it only provides data.

✔ 3. boiler-heating-controller.ino
Controls the Domestic Hot Water (DHW) heating system

This ESP32 receives two key inputs:

Maximum allowed heating power
(sent from the inverter-communication module)

Current boiler water temperature
(sent from temp-measuring)

Using both inputs, it:

decides whether the DHW boiler can be heated

determines how much heating power can be safely used

prevents overheating

manages relays for the boiler’s heating elements

This unit ensures the DHW system uses only the energy that is actually available.
