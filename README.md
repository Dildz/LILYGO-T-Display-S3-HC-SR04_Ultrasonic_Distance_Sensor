LILYGO T-Display-S3 HC-SR04 Ultrasonic Distance Sensor Project
This code reads distance data from a HC-SR04 ultrasonic distance sensor and displays it on the built-in screen of the LilyGO T-Display-S3 using the TFT_eSPI library. The distance is displayed numerically in millimeters (mm) and as a visual meter in centimeters (0-100cm) with a colour gradient (red at 0cm to green at 100cm).

Pin Connections:
 - HC-SR04 Trig  -> GPIO1 (output)
 - HC-SR04 Echo  -> GPIO2 (input)
 - HC-SR04 GND   -> GND
 - HC-SR04 VCC   -> 5V
 - LCD Backlight -> GPIO15

KY-023 Specifications:
 - Measurement Range: ~2cm to ~400cm (20mm to 4000mm)
 - Resolution: 0.3cm (3mm)
 - Accuracy: ±3mm
 - Operating Voltage: 5V DC
 - Trigger Pulse Duration: 10µs
 - Interface: Trigger/Echo
