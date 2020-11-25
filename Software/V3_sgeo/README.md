# AM_WeatherBox
Astro Makers project to make a weather monitor box for and observatory

Designed by Alan Sliski and Aaron Sliski. Other contributers: James Synge, George Silvis

The goal is to create an inexpensive device for weather monitoring at a telescope site/observatory. It features:
  - Just two onboard sensors: rain detector and sky/cloud monitor. These are the most important sensors for making 
    the scope safety decision: Is it safe to open the scope enclosure? 
  - Other information can be collected from online API's like https://darksky.net/dev
  - One wire connected to the box: an ethernet cable with POE (Power Over Ethernet) providing the power.
  - Inexpensive. This unit should cost about $150 for parts.
  - Open source Arduino code

Documentation is this readme and the files in the Documentation folder.

- Some pictures of the device: picxxxx.png
- The weather box excel spread sheet has the BOM and two drawings.
- Processor: Arduino Mega + Ethernet shiedl + POE
    The RobotDyn part that combines all three is easier to work with and may be less expensive.
    It also comes with a heat sink: The POE does dump a fair amount of heat.
- The sky sensor is just 4 connections to the board
- The rain sensor needs 12V. For that we need a step up regulator from the nominal 9V of the POE.
- wiring notes ( these supercede any images and drawings you might find.
   - Rain sensor. 
       - dip switches set for "It's Raining":  5 on for rain gauge, 1,2 off for high sensitivity, 3 on for 15 min hysteresis
       - 12V comes off the step up regulator
       - relay output to arduino pin 47. No need for external pullup; internal is set and is adequate
   -
- monitor script
   - The goal is to access the weather box and decide if its safe to be open and make that information available to ACP
   - This is a work in progress. 
   - Output can be a one line report file in the Boltwood format
   - This can be run from a cron or windows scheduled task.
   
       

 


    
 

