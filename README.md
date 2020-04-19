# simFFB
![simFFB](https://raw.githubusercontent.com/joeyjojojunior/simffb/master/simffb_pic.png)

### Description
simFFB is an invaluable utility that provides joystick force feedback effects beyond those typically offered in games, such as damper and friction, as well as enabling both progressive and instantaneous trimming modes. 

Originally developed by average_pilot and posted to the ED forums (https://forums.eagle.ru/showthread.php?t=84883), it was later modified by Jazzerman to add support for re-initializing dinput with a keybinding. Since average_pilot graciously posted the source code, I've added some extra features that improve on the original which I hope you will enjoy.

### New Features
* **Toggle Trim Button**: allows toggling instantaneous trimmer on and off (as opposed to hold).
* **Toggle Trim Indicator**: asterisk beside Spring Force/Spring Force 2 label indicates active force settings.
* **Center Trim Button**: centers the joystick (only when instantaneous trimmer off).
* **Separate Spring/Damper/Friction Settings**: allows for independent settings for instantaneous trimmer on/off.
  * **Spring Force / Damper Force / Friction Force**: applied when instantaneous trimmer on.
  * **Spring Force 2 / Damper Force 2 / Friction Force 2**: applied when instantaneous trimmer off.
* **Progressive Trim POV Hat Dropdown**: select the joystick POV hat you want to use for trimming.
* **Save dinput Key**: saves the selected dinput re-initialization key in the options file.
* **Cycle Trim Key**: cycles through the various trim modes.

### Documentation
* **Trim Modes**
  * **None:** No trimming functionality.
  * **Instant:** Trims instantly using button presses, typically used for helicopter trim.
  * **Progressive:** Use a POV hat to trim in increments, typically used for airplane trim.
  * **Both:** Both instant and progressive trim modes are active.
* **Spring Force 2/Damper Force 2/Friction Force 2**
  * This second set of FFB force settings are used in conjunction with the instant or both trim mode. They allow you to switch between two different force profiles with the button assignments documented below. Setting Spring Force 2 to zero will replicate helicopter like trimming in instant trim mode. 
* **Hold/Toggle/Center Button Assignments**
  * These dropdown menus are button assignments that only work in instant or both trim mode. The first joystick selection dropdown menu at the top left selects the joystick for these button assignments.
  * **Hold:** When this button is held down, Spring Force 2/Damper Force 2/Friction Force 2 are active. With Spring Force 2 set to zero, this acts like your typical trim button in a helicopter.
  * **Toggle:** When this button is pressed, it toggles between Spring Force/Damper Force/Friction Force 1 and 2. With Spring Force 2 sett to zero, this acts like the force trim switch in a helicopter. 
  * **Center:** When Spring Force 1 is active, this button will center the joystick.  
* **Joystick POV Hat Selection**
  * The second joystick selection dropdown selects which joystick's POV switch controls the progressive trim function. This is useful if you have modded a different grip onto your joystick and need to use its POV hat to control trim.  
* **Swap Axis Checkbox**
  * This swaps the x and y axis for force feedback effects. Should be checked if using the Microsoft Sidewinder FFB2. 
* **Init Hotkey Dropdown**
  * This dropdown menu allows you to assign a hotkey to re-initialize DirectInput.
* **Trim Hotkey Dropdown**
  * This dropdown menu allows you to assign a hotkey that will cycle through the 4 trim modes. 


### Building
Open solution file in Visual Studio 2019 and hit build :)
