# skimap


1. Plug ESP into a powersource and let it boot
2. If it can't find a saved access point ssid and password, goes into AP mode.
3. Connect to wifi Skimap setup
4. Access the webserver via the IP 192.168.4.1 (you can change this in the code)
5. Enter wifi credentials and hit save
6. ESP reboots and connects to wifi, if unsuccessful goes back into AP mode
7. Will then run the fastLED lib to cycle through the LEDs after connecting to wifi, pin 13, 20 LEDs (variables in code) 


Reset procedure:
1. Click the "EN" (reboot button) and it will clear the saved wifi config and restart into AP mode
