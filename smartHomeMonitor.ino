//import necessary libraries
#include <Adafruit_RGBLCDShield.h>
#include <Wire.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

/*-------------------------------------*/
/* Declaraction of LCD Backlight colors */
/*-------------------------------------*/
const uint8_t PURPLE = 0x5;
const uint8_t WHITE = 0x7;
const uint8_t YELLOW = 0x3;
const uint8_t GREEN = 0x2; 

// A struct to hold smart device data.
struct SmartDevice {
  String id; 
  char type; // S,O,L,T,C
  String location;
  String state; // all devies, on or off
  int power; // for speaker and light
  int temperature;  // for thermostats
};

/*-------------------------------------*/
/* Declaraction of global variables. */
/*-------------------------------------*/

// An array to hold the smart devices and a counter to keep track of the number of devices.
SmartDevice devices[20];

// Count the number of devices
int deviceCount = 0;

// Keep track of the current device displayed on LCD
int currentDisplayIndex = 0;

// This is a boolean variable that is set to true if you only want to display devices with the state "ON". 
bool showOnlyOnDevices = false;

// This is a boolean variable that is set to true if you only want to display devices with the state "OFF". 
bool showOnlyOffDevices = false;

// Time variable to help scrolling the device's location.
unsigned long currentMillis;
unsigned long previousMillis = 0;
unsigned long scrollInterval = 500; // Set scroll interval to 2 characters per second (500 ms for 1 character)
int scrollPosition = 0;


//This is a boolean variable that is set to true when the initialistion phase has been completed.
bool beginningPhaseComplete = false;



/*----------------------------------*/
/* Memory Management Functions */
/*----------------------------------*/

/*
 * External heap start and break value pointers.
 * These pointers represent the start of the heap and the end of the currently used
 * heap memory respectively. They are used to calculate the available free memory.
 */
extern unsigned int __heap_start;
extern void *__brkval;
/*
 * The free list structure as maintained by the
 * avr-libc memory allocation routines.
 */
struct __freelist
{
  size_t sz;
  struct __freelist *nx;
};


// Calculates the size of the free list, which represents the total amount of free memory available in the heap that has been previously allocated and freed.
extern struct __freelist *__flp;

// Calculates the size of the free list 
int freeListSize()
{
  struct __freelist* current;
  int total = 0;
  for (current = __flp; current; current = current->nx)
  {
    total += 2; /* Add two bytes for the memory block's header  */
    total += (int) current->sz;
  }

  return total;
}

// Calculates the available free memory in bytes by determining the difference
// between the stack pointer and the heap end (__brkval) or heap start (__heap_start)
// if __brkval is 0.

int freeMemory()
{
  int free_memory;
  if ((int)__brkval == 0)
  {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  }
  else
  {
    free_memory = ((int)&free_memory) - ((int)__brkval);
    free_memory += freeListSize();
  }
  return free_memory;
}


// Custom character data for up arrow
byte upArrow[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};
// Custom character data for down arrow
byte downArrow[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};

// Sort devices by their ID
void sortDevices() {
  for (int i = 0; i < deviceCount - 1; i++) {
    for (int j = 0; j < deviceCount - i - 1; j++) {
      if (devices[j].id.compareTo(devices[j + 1].id) > 0) {
        // Swap devices[j] and devices[j + 1]
        SmartDevice temp = devices[j];
        devices[j] = devices[j + 1];
        devices[j + 1] = temp;
      }
    }
  }

  // Print the sorted devices
  /*for (int i = 0; i < deviceCount; i++) {
    //Serial.print(F("DEBUG:Device ID: "));
    //Serial.print(devices[i].id);
    //Serial.print(F(" Location: "));
    //Serial.println(devices[i].location);
  }*/
}

/*
This function handles the behavior when the up button is pressed. 
It checks if there are any devices to display and if the currently displayed device isn't the first one in the list. 
Depending on the state of showOnlyOffDevices and showOnlyOnDevices, 
it will update the currentDisplayIndex variable to point to the previous device with the desired state (either "OFF" or "ON") and update the display accordingly. 
If neither showOnlyOffDevices nor showOnlyOnDevices is true, it will simply display the previous device in the list, regardless of its state.
*/
/*-------------------*/
/* Button handlers */
/*-------------------*/
void upButtonHandler() {
  if (deviceCount > 0 && currentDisplayIndex > 0) {
    if (showOnlyOffDevices) {
      // Find the previous "OFF" device
      int prevOffDeviceIndex = -1;
      for (int i = currentDisplayIndex - 1; i >= 0; i--) {
        if (devices[i].state == "OFF") {
          prevOffDeviceIndex = i;
          break;
        }
      }

      // If there's a previous "OFF" device, update the currentDisplayIndex and update the display
      if (prevOffDeviceIndex != -1) {
        currentDisplayIndex = prevOffDeviceIndex;
        updateDisplay();
      }
    } else if (showOnlyOnDevices) {
      // Find the previous "OFF" device
      int prevOnDeviceIndex = -1;
      for (int i = currentDisplayIndex - 1; i >= 0; i--) {
        if (devices[i].state == "ON") {
          prevOnDeviceIndex = i;
          break;
        }
      }

      // If there's a previous "OFF" device, update the currentDisplayIndex and update the display
      if (prevOnDeviceIndex != -1) {
        currentDisplayIndex = prevOnDeviceIndex;
        updateDisplay();
      }
    }else {
      currentDisplayIndex--;
      updateDisplay();
    }
  }
}

/*
This function handles the behavior when the down button is pressed. 
It checks if there are any devices to display and if the currently displayed device isn't the first one in the list. 
Depending on the state of showOnlyOffDevices and showOnlyOnDevices, 
it will update the currentDisplayIndex variable to point to the previous device with the desired state (either "OFF" or "ON") and update the display accordingly. 
If neither showOnlyOffDevices nor showOnlyOnDevices is true, it will simply display the previous device in the list, regardless of its state.
*/
void downButtonHandler() {
  if (deviceCount > 0 && currentDisplayIndex < deviceCount - 1) {
    if (showOnlyOnDevices) {
      // Find the next "ON" device
      int nextOnDeviceIndex = -1;
      for (int i = currentDisplayIndex + 1; i < deviceCount; i++) {
        if (devices[i].state == "ON") {
          nextOnDeviceIndex = i;
          break;
        }
      }

      // If there's a next "ON" device, update the currentDisplayIndex and update the display
      if (nextOnDeviceIndex != -1) {
        currentDisplayIndex = nextOnDeviceIndex;
        updateDisplay();
      }
    } else if (showOnlyOffDevices) {
      // Find the next "OFF" device
      int nextOffDeviceIndex = -1;
      for (int i = currentDisplayIndex + 1; i < deviceCount; i++) {
        if (devices[i].state == "OFF") {
          nextOffDeviceIndex = i;
          break;
        }
      }

      // If there's a next "OFF" device, update the currentDisplayIndex and update the display
      if (nextOffDeviceIndex != -1) {
        currentDisplayIndex = nextOffDeviceIndex;
        updateDisplay();
      }
    } else {
      currentDisplayIndex++;
      updateDisplay();
    }
  }
}
// Handler for the RIGHT button press
void rightButtonHandler() {
  // Toggle the showOnlyOnDevices filter
  showOnlyOnDevices = !showOnlyOnDevices;
  if (showOnlyOnDevices) {
    // Find the first "ON" device and set currentDisplayIndex accordingly
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].state == "ON") {
        currentDisplayIndex = i;
        break;
      }
    }
    // Display the list of "ON" devices
    displayOnDevices();
  } else {
    // Reset the currentDisplayIndex to the first device when switching back to the unfiltered view
    currentDisplayIndex = 0;
    updateDisplay();
  }
}

// Handler for the LEFT button press
void leftButtonHandler() {
  // Toggle the showOnlyOffDevices filter
  showOnlyOffDevices = !showOnlyOffDevices;
  if (showOnlyOffDevices) {
    // Find the first "OFF" device and set currentDisplayIndex accordingly
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].state == "OFF") {
        currentDisplayIndex = i;
        break;
      }
    }
    // Display the list of "OFF" devices
    displayOffDevices();
  } else {
    // Reset the currentDisplayIndex to the first device when switching back to the unfiltered view
    currentDisplayIndex = 0;
    updateDisplay();
  }
}
/*-----------------*/
/* Hnadling input */
/*------------------*/
/* This is a function for handling incoming data, it accepts the incoming data as a parameter and does the needful depending on 
the first character, A, S, P, or R. Any other character will be treated as an error.
It also validates the incoming data and displays errors to the serial monitor.
A denotes adding a device, S denotes setting the state of a device, P denotes setting the power output and R denotes removal of a device.
The above is handled by a bunch of switch cases
*/
void processSerialData(String data) {
  // Check if the synchronisation phase has been completed.
  if (!beginningPhaseComplete || data.length() == 0) {
    return;
  }
    // Validation check for missing field separator '-'
  if (data.indexOf('-') == -1) {
    Serial.print(F("ERROR:"));
    Serial.println(data);

    return;
  }
  // Exctraction of beginning character for switch case
  char operation = data.charAt(0);
  //Serial.print(F("DEBUG:Data received in processSerialData: ")); // Debug
  //Serial.println(data);

  //Serial.print(F("DEBUG:Command extracted: ")); // Debug
  //Serial.println(operation);

  switch (operation) {    
    
    // Adding a device
    case 'A':
      //Serial.println(F("DEBUG:A case executed")); // Debug
      addDevice(data);
      break;
    // Setting the state of a device  
    case 'S':
      //Serial.println(F("DEBUG:S case executed")); // Debug
      updateDeviceState(data);
      break;
    // Displaying power output of a device      
    case 'P':
      //Serial.println(F("DEBUG:P case executed")); // Debug
      updateDevicePower(data);      
      break;
    // Removal of a device
    case 'R':
      //Serial.println(F("DEBUG:R case executed")); // Debug
      removeDevice(data);
      break;
    // Error
    default:
      Serial.print(F("ERROR:"));
      Serial.println(data);
  }
  
}

// This function updates the display based on the values of showOnlyOnDevices, showOnlyOffDevices, and other device information.
void updateDisplay() {

  // If showOnlyOnDevices is true, call the displayOnDevices function and return.
  if (showOnlyOnDevices) {
  displayOnDevices();
  return;
  }
  // If showOnlyOffDevices is true, call the displayOffDevices function and return.
  if (showOnlyOffDevices) {
  displayOffDevices();
  return;
  }

  lcd.clear();


    if (deviceCount == 0) {
    // If there are no devices, set the backlight color to white.
    lcd.setBacklight(WHITE);
    return;
  }
  // Display the up arrow only after the first device display.
  if (currentDisplayIndex > 0) {
    // Set the cursor to the position where the up arrow should be displayed
    lcd.setCursor(0, 0);
    lcd.write(byte(0)); // Up arrow (custom character 0)
  }

  // Set the cursor to the position where device ID should be displayed
  lcd.setCursor(1, 0);

  // Print device ID
  lcd.print(devices[currentDisplayIndex].id);

  // Set the cursor to the position where device location should be displayed
  lcd.setCursor(2 + String(devices[currentDisplayIndex].id).length(), 0);
  
  // Calculate the maximum allowed length for the location string
  int maxLocationLength = min(11, 16 - (2 + String(devices[currentDisplayIndex].id).length()));

  // Check if the location string is longer than 11 characters
  String location = devices[currentDisplayIndex].location;
  if (location.length() > maxLocationLength) {
    // If the scroll position reached the end, reset it to the start
    if (scrollPosition >= location.length() - maxLocationLength + 1) {
      scrollPosition = 0;
    }
    
    // Extract the substring to display on the screen
    String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
    lcd.print(displayText);
  } else {
    lcd.print(location);
  }
  // Display the down arrow for all devices except the last device
  if (currentDisplayIndex < deviceCount - 1) {
    // Set the cursor to the position where the down arrow should be displayed
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); // Down arrow (custom character 1)
  }

  // Set the cursor to the position where device type should be displayed
  lcd.setCursor(1, 1);

  // Print device type
  lcd.print(devices[currentDisplayIndex].type);

  // Print device state with a leading space
  if (devices[currentDisplayIndex].state == "ON") {
    lcd.print("  " + devices[currentDisplayIndex].state);
  } else {
    lcd.print(" " + devices[currentDisplayIndex].state);
  }

  // Print device power percentage, temperature, or an empty space, depending on the device type
  if (devices[currentDisplayIndex].type == 'S' || devices[currentDisplayIndex].type == 'L') {
        if (devices[currentDisplayIndex].power >= 0 && devices[currentDisplayIndex].power <= 9) {
      lcd.print("   " + String(devices[currentDisplayIndex].power) + "%");
      } else if (devices[currentDisplayIndex].power >= 10 && devices[currentDisplayIndex].power <= 99) {
      lcd.print("  "+ String(devices[currentDisplayIndex].power) + "%"); }
    // lcd.print(" " + String(devices[currentDisplayIndex].power) + "%");
    else if (devices[currentDisplayIndex].power == 100) {
      lcd.print(" "+ String(devices[currentDisplayIndex].power) + "%"); }
    // lcd.print(" " + String(devices[currentDisplayIndex].power) + "%");
  } else if (devices[currentDisplayIndex].type == 'T') {
    if (devices[currentDisplayIndex].temperature == 9) {
      lcd.print("  " + String(devices[currentDisplayIndex].temperature) + (char)223 + "C");}
    else if (devices[currentDisplayIndex].temperature > 9 && devices[currentDisplayIndex].temperature <= 32) {
      lcd.print(" " + String(devices[currentDisplayIndex].temperature) + (char)223 + "C"); }
  } else {
    lcd.print(" ");
  }


    // Change the backlight color based on the device state
  if (deviceCount == 0) {
    lcd.setBacklight(WHITE);
  }
  else if (devices[currentDisplayIndex].state == "ON") {
    lcd.setBacklight(GREEN);
  } else {
    lcd.setBacklight(YELLOW);
  }
}

/* This function reads from the serial input until it encounters the specified terminator character, 
and returns the read string.*/
String readSerialUntil(char terminator) {
  String result = "";
  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == terminator) {
        break;
      } else if (c != '\r' && c != '\n') { // Ignore carriage return and newline characters
        result += c;
      }
    }
  }
  return result;
}
// This function checks if the given deviceId is a valid device ID (3 uppercase letters).
bool isValidDeviceId(String deviceId) {
  if (deviceId.length() != 3) return false;
  for (int i = 0; i < deviceId.length(); i++) {
    if (deviceId.charAt(i) < 'A' || deviceId.charAt(i) > 'Z') return false;
  }
  return true;
}

// This function checks if the given deviceLocation is a valid device location (alphanumeric characters).
bool isValidDeviceLocation(String deviceLocation) {
  if (deviceLocation.length() < 1) return false;
  for (int i = 0; i < deviceLocation.length(); i++) {
    char c = deviceLocation.charAt(i);
    bool isAllowedChar = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    
    if (!isAllowedChar) {
      return false;
    }
  }
  return true;
}

// Function to add a new device
void addDevice(String data) {
  // Find the positions of the dashes in the input string
  int firstDash = data.indexOf('-');
  int secondDash = data.indexOf('-', firstDash + 1);
  int thirdDash = data.indexOf('-', secondDash + 1);
  // If any of the dashes are missing or misplaced, print an error message and return
  if (firstDash == -1 || secondDash == -1 || thirdDash == -1 || thirdDash != secondDash + 2) {
  Serial.print(F("ERROR:"));
  Serial.println(data);
  return;
  }
  // Extract the device information from the input string
  String deviceId = data.substring(firstDash + 1, secondDash);
  char deviceType = data.charAt(secondDash + 1);
  String deviceLocation = data.substring(thirdDash + 1);

  // Check if the device ID is valid
  if (!isValidDeviceId(deviceId)) {
    //Serial.print(F("DEBUG:Invalid device ID - ")); // Debug
    //Serial.println(data);
    Serial.print(F("ERROR:"));
    Serial.println(data);
    return;
  }
  // Check if the device type is valid
  if (!(deviceType == 'S' || deviceType == 'O' || deviceType == 'L' || deviceType == 'T' || deviceType == 'C')) {
    //Serial.print(F("DEBUG:Invalid device type - ")); // Debug
    //Serial.println(data);
    Serial.print(F("ERROR:"));
    Serial.println(data);
    return;
  }
  // Check if the device location is valid
  if (!isValidDeviceLocation(deviceLocation)) {
    //Serial.print(F("DEBUG:Invalid device location: ")); // Debug
    //Serial.println(data);
    Serial.print(F("ERROR:")); // Debug
    Serial.println(data);
    return;
  }
  // Check if device ID already exists
  bool deviceExists = false;
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].id == deviceId) {
      deviceExists = true;
      // Check if the device location is valid
      devices[i].type = deviceType;
      devices[i].location = deviceLocation.substring(0, 15);
      break;
    }
  }
  // If the device doesn't exist and there's room for more devices
  if (!deviceExists && deviceCount < 20) {
    // Add the new device to the devices array
    devices[deviceCount].id = deviceId;
    devices[deviceCount].type = deviceType;
    devices[deviceCount].location = deviceLocation.substring(0, 15);
    devices[deviceCount].state = "OFF";
    // Set default power values based on device type
    if (deviceType == 'T') {
      devices[deviceCount].temperature = 9;
    } else if (deviceType == 'S' || deviceType == 'L') {
      devices[deviceCount].power = 0;
    } else {
      devices[deviceCount].power = -1;
      devices[deviceCount].temperature = -1;
    }
    // Increment the device count
    deviceCount++;
    // Sort the devices and update the display after adding a new device
    sortDevices();
    updateDisplay(); 
  }
}
// Function to remove a device
void removeDevice(String data) {
  
  // Find the position of the dash in the input string
  int dashIndex = data.indexOf('-');
  // If the dash is missing or misplaced, print an error message and return
  if (dashIndex == -1 || dashIndex >= data.length() - 1) {
    Serial.print(F("ERROR:"));
    Serial.println(data);
    return;
  }
  // Extract the device ID from the input string
  String id = data.substring(dashIndex + 1);
  id.trim();

  // If the ID is empty or has incorrect length, print an error message and return
  if (id.length() == 0 || id.length() > 3||id.length() < 3) {
    Serial.print(F("ERROR:"));
    Serial.println(data);
    return; 
  }
  // Find the index of the device with the specified ID
  int indexToRemove = -1;
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].id == id) {
      indexToRemove = i;
      break;
    }
  }
  // If the device is found, remove it from the devices array
  if (indexToRemove != -1) {
    //Serial.print(F("DEBUG:Removing device with ID: ")); // Debug
    //Serial.println(id);
    // Shift all devices to the left, effectively removing the device
    for (int i = indexToRemove; i < deviceCount - 1; i++) {
      devices[i] = devices[i + 1];
    }
    // Decrement the device count
    deviceCount--;

    // Check if there are no devices left
    if (deviceCount == 0) {
      // Clear the LCD display
      lcd.clear();
    } else {
      // Update the current display index if necessary
      if (currentDisplayIndex >= deviceCount && currentDisplayIndex > 0) {
        currentDisplayIndex--;
      }
      // Sort the devices and update the display after removing a device
      sortDevices();
      updateDisplay();
    }
  } else {
    //Serial.print(F("DEBUG:Device with ID not found: ")); // Debug
    //Serial.println(id);
    Serial.print(F("ERROR:"));
    Serial.println(data);
  }
}

// Function to update the power or temperature value of a specific device
void updateDevicePower(String data) {
  // Find the positions of the '-' characters in the input string
  int separator1 = data.indexOf('-');
  int separator2 = data.indexOf('-', separator1 + 1);

  // Extract the device ID and power value from the input string
  String id = data.substring(separator1 + 1, separator2); 
  String powerString = data.substring(separator2 + 1); 
  int power = powerString.toInt(); // Convert the power value to an integer

  // Initialize variables to track if the device ID is found and if the power value is valid
  bool idFound = false;
  bool validPower = false;

  // Check if the powerString is a valid number
  for (char c : powerString) {
    if (!isdigit(c)) {
      Serial.print(F("ERROR:"));
      Serial.println(data);
      return;
    }
  }
  // Iterate through the devices array and update the power or temperature value for the matching device
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].id == id) {
      idFound = true;
      // Check if the device type is 'S' or 'L' and if the power value is within the valid range
      if ((devices[i].type == 'S' || devices[i].type == 'L') && (power >= 0 && power <= 100)) {
        devices[i].power = power;
        validPower = true;
        updateDisplay(); // Update the display after changing the device power
        return;
      }
      // Check if the device type is 'T' and if the temperature value is within the valid range
      else if (devices[i].type == 'T' && (power >= 9 && power <= 32)) {
        devices[i].temperature = power;
        validPower = true;
        updateDisplay(); // Update the display after changing the device power
        return;
      }
    }
  }
  // If the device ID is not found or the power value is not valid, print an error message
  if (!idFound || !validPower) {
    Serial.print(F("ERROR:"));
    Serial.println(data);
  }
}

// Function to update the state (ON/OFF) of a specific device
void updateDeviceState(String data) {
  // Find the positions of the '-' characters in the input string
  int separator1 = data.indexOf('-');
  int separator2 = data.indexOf('-', separator1 + 1);
  // Extract the device ID and state from the input string
  String id = data.substring(separator1 + 1, separator2); // Extract the ID between the two '-' characters
  String state = data.substring(separator2 + 1);
  // Initialize a variable to track if the device ID is found
  bool idFound = false;
  // Check if the state is either "ON" or "OFF"
  if (state == "ON" || state == "OFF") {
    // Iterate through the devices array and update the state for the matching device
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].id == id) {
        idFound = true;
        devices[i].state = state;
        updateDisplay(); // Update the display after changing the device state
        return;
      }
    }
  }
  // If the device ID is not found or the state is not valid ("ON" or "OFF"), print an error message
  if (!idFound || !(state == "ON" || state == "OFF")) {
    Serial.print(F("ERROR:"));
    Serial.println(data);
  }
}

// Displays information about devices that are currently "ON" on an LCD screen when the right button is pressed.
void displayOnDevices() {
  // Counters for the number of devices that are currently "ON".
  int onDevicesCount = 0;
  // Iterate through the device array to find the number of on devices
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].state == "ON") {
      onDevicesCount++;
    }
  }
  // If no devices are "ON", display a message
  if (onDevicesCount == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NOTHING'S ON");
    lcd.setBacklight(WHITE);
    } else {
      // Indices of devices in the devices array, used to keep track of the currently displayed device and to find the previous and next devices in the list.
      int onDeviceIndex = 0;
      int prevOnDeviceIndex = -1;
      int nextOnDeviceIndex = -1;
      for (int i = 0; i < deviceCount; i++) {
        if (devices[i].state == "ON") {
          if (onDeviceIndex == currentDisplayIndex) {
            // Check if there's an ON device above the current one
            for (int j = i - 1; j >= 0; j--) {
              if (devices[j].state == "ON") {
                prevOnDeviceIndex = j;
                break;
              }
            }
            // Check if there's an ON device below the current one
            for (int j = i + 1; j < deviceCount; j++) {
              if (devices[j].state == "ON") {
                nextOnDeviceIndex = j;
                break;
              }
            }

            lcd.clear();
            // Display the information of the "ON" device
            lcd.setCursor(1, 0);
            // Print device ID
            lcd.print(devices[i].id);
            // Set the cursor to the position where device location should be displayed
            lcd.setCursor(2 + String(devices[i].id).length(), 0);
            // Calculate the maximum allowed length for the location string
            int maxLocationLength = min(11, 16 - (2 + String(devices[i].id).length()));
            // Check if the location string is longer than 11 characters
            String location = devices[i].location;
            if (location.length() > maxLocationLength) {
              // If the scroll position reached the end, reset it to the start
              if (scrollPosition >= location.length() - maxLocationLength + 1) {
                scrollPosition = 0;
              }
              
              // Extract the substring to display on the screen
              String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
              lcd.print(displayText);
            } else {
              lcd.print(location);
            }
            // Set the cursor to the position where device type should be displayed
            lcd.setCursor(1, 1);
            // Print device type
            lcd.print(devices[i].type);
            // Print device state with a leading space
            lcd.print("  " + devices[i].state);
            // Print device power percentage, temperature, or an empty space, depending on the device type
            if (devices[i].type == 'S' || devices[i].type == 'L') {
                  if (devices[i].power >= 0 && devices[i].power <= 9) {
                lcd.print("   " + String(devices[i].power) + "%");
                } else if (devices[i].power >= 10 && devices[i].power <= 99) {
                lcd.print("  "+ String(devices[i].power) + "%"); }
              else if (devices[i].power == 100) {
                lcd.print(" "+ String(devices[i].power) + "%"); }
              // lcd.print(" " + String(devices[currentDisplayIndex].power) + "%");
            } else if (devices[i].type == 'T') {
              if (devices[i].temperature == 9) {
                lcd.print("  " + String(devices[i].temperature) + (char)223 + "C");}
              else if (devices[i].temperature > 9 && devices[i].temperature <= 32) {
                lcd.print(" " + String(devices[i].temperature) + (char)223 + "C"); }
            } else {
              lcd.print(" ");
            }
            // Set the cursor to the position where the up arrow should be displayed
            if (prevOnDeviceIndex != -1) {
              lcd.setCursor(0, 0);
              lcd.write(byte(0)); // Up arrow (custom character 0)
            }
            // Set the cursor to the position where the down arrow should be displayed
            if (nextOnDeviceIndex != -1) {
              lcd.setCursor(0, 1);
              lcd.write(byte(1)); // Down arrow (custom character 1)
            }

            break;
          }
          onDeviceIndex++;
        }
      }
    }
    // Set the backlight color to green for ON devices
  lcd.setBacklight(GREEN);
}
// Displays information about devices that are currently "OFF" on an LCD screen when the left button is pressed. 
void displayOffDevices() {
  // Counters for the number of devices that are currently "OFF".
  int offDevicesCount = 0;
  // Iterate through the device array to find the number of off devices
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].state == "OFF") {
      offDevicesCount++;
    }
  }
  // If no devices are "OFF", display a message
  if (offDevicesCount == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NOTHING'S OFF");
    lcd.setBacklight(WHITE);
  } else {
    // Indices of devices in the devices array, used to keep track of the currently displayed device and to find the previous and next devices in the list.
    int offDeviceIndex = 0;
    int prevOffDeviceIndex = -1;
    int nextOffDeviceIndex = -1;
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].state == "OFF") {
        if (offDeviceIndex == currentDisplayIndex) {
          // Check if there's an ON device above the current one
          for (int j = i - 1; j >= 0; j--) {
            if (devices[j].state == "OFF") {
              prevOffDeviceIndex = j;
              break;
            }
          }
          // Check if there's an ON device below the current one
          for (int j = i + 1; j < deviceCount; j++) {
            if (devices[j].state == "OFF") {
              nextOffDeviceIndex = j;
              break;
            }
          }

          lcd.clear();
          // Display the information of the "ON" device
          lcd.setCursor(1, 0);
          // Print device ID
          lcd.print(devices[i].id);
          // Set the cursor to the position where device location should be displayed
          lcd.setCursor(2 + String(devices[i].id).length(), 0);
          // Calculate the maximum allowed length for the location string
          int maxLocationLength = min(11, 16 - (2 + String(devices[i].id).length()));
          // Check if the location string is longer than 11 characters
          String location = devices[i].location;
          if (location.length() > maxLocationLength) {
            // If the scroll position reached the end, reset it to the start
            if (scrollPosition >= location.length() - maxLocationLength + 1) {
              scrollPosition = 0;
            }
            
            // Extract the substring to display on the screen
            String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
            lcd.print(displayText);
          } else {
            lcd.print(location);
          }
          // Set the cursor to the position where device type should be displayed
          lcd.setCursor(1, 1);
          // Print device type
          lcd.print(devices[i].type);
          // Print device state with a leading space
          lcd.print(" " +devices[i].state);
          // Print device power percentage, temperature, or an empty space, depending on the device type
          if (devices[i].type == 'S' || devices[i].type == 'L') {
                if (devices[i].power >= 0 && devices[i].power <= 9) {
              lcd.print("   " + String(devices[i].power) + "%");
              } else if (devices[i].power >= 10 && devices[i].power <= 99) {
              lcd.print("  "+ String(devices[i].power) + "%"); }
            else if (devices[i].power == 100) {
              lcd.print(" "+ String(devices[i].power) + "%"); }
            // lcd.print(" " + String(devices[currentDisplayIndex].power) + "%");
          } else if (devices[i].type == 'T') {
            if (devices[i].temperature == 9) {
              lcd.print("  " + String(devices[i].temperature) + (char)223 + "C");}
            else if (devices[i].temperature > 9 && devices[i].temperature <= 32) {
              lcd.print(" " + String(devices[i].temperature) + (char)223 + "C"); }
          } else {
            lcd.print(" ");
          }
          // Set the cursor to the position where the up arrow should be displayed
          if (prevOffDeviceIndex != -1) {
            lcd.setCursor(0, 0);
            lcd.write(byte(0)); // Up arrow (custom character 0)
          }
          // Set the cursor to the position where the down arrow should be displayed
          if (nextOffDeviceIndex != -1) {
            lcd.setCursor(0, 1);
            lcd.write(byte(1)); // Down arrow (custom character 1)
          }
          break;
        }
        offDeviceIndex++;
      }
    }
  }
    // Set the backlight color to green for ON devices
  lcd.setBacklight(YELLOW);
}
/*----------------------------------*/
/* Scroll Extension Functions */
/*----------------------------------*/
// Function to scroll the device location text when it's too long to fit the screen
void scrollLocation() {
  // Check if there are devices to display
  if (deviceCount > 0) {
    // Calculate the maximum allowed length for the location string
    int maxLocationLength = min(11, 16 - (2 + String(devices[currentDisplayIndex].id).length()));
    
    // Check if the location string is longer than maxLocationLength characters
    String location = devices[currentDisplayIndex].location;
    if (location.length() > maxLocationLength) {
      // If the scroll position reached the end, reset it to the start
      if (scrollPosition >= location.length() - maxLocationLength + 1) {
        scrollPosition = 0;
      }
      
      // Extract the substring to display on the screen
      String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
      lcd.setCursor(2 + String(devices[currentDisplayIndex].id).length(), 0);
      lcd.print(displayText);
    }
  }
}

// Function to scroll the device location text for ON devices when it's too long to fit the screen
void scrollOnDevicesLocation() {
  if (deviceCount > 0) {

  // A variable to keep track of the number of on devices
    int onDeviceIndex = 0;
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].state == "ON") {
        if (onDeviceIndex == currentDisplayIndex) {
          // Set the cursor to the position where device location should be displayed
          lcd.setCursor(2 + String(devices[i].id).length(), 0);

          // Calculate the maximum allowed length for the location string
          int maxLocationLength = min(11, 16 - (2 + String(devices[i].id).length()));

          // Check if the location string is longer than maxLocationLength characters
          String location = devices[i].location;
          if (location.length() > maxLocationLength) {
            // If the scroll position reached the end, reset it to the start
            if (scrollPosition >= location.length() - maxLocationLength + 1) {
              scrollPosition = 0;
            }

            // Extract the substring to display on the screen
            String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
            lcd.print(displayText);
          }
          break;
        }
        onDeviceIndex++;
      }
    }
  }
}


// Function to scroll the device location text for OFF devices when it's too long to fit the screen
void scrollOffDevicesLocation() {
  if (deviceCount > 0) {
  // A variable to keep track of the number of off devices
    int offDeviceIndex = 0;
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].state == "OFF") {
        if (offDeviceIndex == currentDisplayIndex) {
          // Set the cursor to the position where device location should be displayed
          lcd.setCursor(2 + String(devices[i].id).length(), 0);

          // Calculate the maximum allowed length for the location string
          int maxLocationLength = min(11, 16 - (2 + String(devices[i].id).length()));

          // Check if the location string is longer than maxLocationLength characters
          String location = devices[i].location;
          if (location.length() > maxLocationLength) {
            // If the scroll position reached the end, reset it to the start
            if (scrollPosition >= location.length() - maxLocationLength + 1) {
              scrollPosition = 0;
            }

            // Extract the substring to display on the screen
            String displayText = location.substring(scrollPosition, scrollPosition + maxLocationLength);
            lcd.print(displayText);
          }
          break;
        }
        offDeviceIndex++;
      }
    }
  }
}



// Function to set up the environment, run once at the beginning of the program
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  // Create custom characters for up and down arrows
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);

  // Set initial backlight of synchronisation phase to purple
  lcd.setBacklight(PURPLE);

  // Synchronisation phase, where when Q is received, main phase begins
  // Every other character will be treated as an error
  while (true) {
    if (Serial.available() > 0) {
      char incomingChar = Serial.read();
      if (incomingChar == 'X') {
        beginningPhaseComplete = true;
        break;
      } else if (incomingChar == '\n' || incomingChar == '\r') {
        Serial.println(F("ERROR:Newline or carriage return received during initialization."));
      }
    }
    Serial.print('Q');
    // Print Q once per second
    delay(1000);
  }

  // When main phase has started, set backlight to white and update display
  if (beginningPhaseComplete) {
    Serial.println(F("UDCHARS,FREERAM,HCI,SCROLL"));
    lcd.setBacklight(WHITE);
    sortDevices();
    updateDisplay();
  }
}
// Function that contains the main loop of the program, called repeatedly
void loop() {

  //  A global variable that holds the current time in milliseconds since the program started.
  currentMillis = millis();

    if (!beginningPhaseComplete) {
    return;
  }
  // A static unsigned long variable that stores the time of the last button press. 
  static unsigned long lastButtonPressTime = 0;
  // A constant unsigned long variable that represents the debounce delay (200 milliseconds) to prevent button bouncing.*/
  const unsigned long debounceDelay = 200;
  // A uint8_t variable that holds the current state of the buttons. 
  uint8_t buttons = lcd.readButtons();
  // An unsigned long variable that holds the current time in milliseconds since the program started.
  unsigned long currentTime = millis();

  //Check if enough time has passed since the last button press to avoid debounce issues
  if (currentTime - lastButtonPressTime > debounceDelay) {
    // Handle UP button press
    if (buttons & BUTTON_UP) {
      currentDisplayIndex--;
      if (currentDisplayIndex < 0) {// Ensure that the currentDisplayIndex does not go below 0
        currentDisplayIndex = 0;
      }
      updateDisplay();
      lastButtonPressTime = currentTime;
    }
    // Handle DOWN button press
    if (buttons & BUTTON_DOWN) {
      currentDisplayIndex++;
      // Ensure that the currentDisplayIndex does not go above the last index
      if (currentDisplayIndex >= deviceCount - 1) {
        currentDisplayIndex = deviceCount - 1;
      }
      updateDisplay();
      lastButtonPressTime = currentTime;
    }
    // Handle SELECT button press
    if (buttons & BUTTON_SELECT) {
      // An unsigned long variable that holds the time when the SELECT button was first pressed. 
      //It's used to check if the button has been pressed for a long enough duration (1000 milliseconds).     
      unsigned long pressStartTime = millis();
      // A boolean variable that indicates if the device ID and free SRAM should be displayed on the screen.
      bool displayID = false;

      // Check if the SELECT button is still being pressed
      while (lcd.readButtons() & BUTTON_SELECT) {
        // Check if the SELECT button has been pressed for more than 1000 milliseconds
        if (millis() - pressStartTime > 1000) {
          displayID = true;
          lcd.clear();
          lcd.setBacklight(PURPLE);
          lcd.print("F211372");
          lcd.setCursor(0,1);
          lcd.print("Free SRAM: ");
          lcd.print(freeMemory());
          break;
        }
      }

      // Wait for the SELECT button to be released
      while (lcd.readButtons() & BUTTON_SELECT);

      // Refresh the display after showing the device ID and free SRAM
      if (displayID) {
        delay(100); // Add a short delay before forcing an additional refresh
        lcd.clear();// Clear screen   
      }

      updateDisplay();
      lastButtonPressTime = currentTime;
    }

    // Handle RIGHT button press
    if (buttons & BUTTON_RIGHT) {
      rightButtonHandler();
      lastButtonPressTime = currentTime;
    }

    // Handle LEFT button press
    if (buttons & BUTTON_LEFT) {
    leftButtonHandler();
    lastButtonPressTime = currentTime;
    }
    
    }
    
    // Check if serial data is available
    if (Serial.available() > 0) {
      // A String variable that holds the incoming serial data until a newline character is encountered.
      String serialData = readSerialUntil('\n');
      
      serialData.replace("\r", ""); // Remove carriage return characters
      serialData.replace("\n", ""); // Remove newline characters

      // Convert the serialData to uppercase
      serialData.toUpperCase();

      //Serial.print(F("DEBUG:Data received:")); // Debug
      //Serial.println(serialData);// Add the length of the incoming data
      processSerialData(serialData); //Pass the input to the function and carry out the main phase
      updateDisplay(); // Refresh the display after processing the serial data
    }

    // Scroll text if the location string is longer than 11 characters
    if (currentMillis - previousMillis >= scrollInterval) {
      previousMillis = currentMillis;
      scrollPosition++;
      // Scroll the location text depending on the filter settings
      if (showOnlyOnDevices){
        scrollOnDevicesLocation();
        } else if (showOnlyOffDevices){
        scrollOffDevicesLocation();
        }else { 
          scrollLocation(); 
        }
    }
}