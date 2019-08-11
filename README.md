# simFFB
![simFFB](https://raw.githubusercontent.com/joeyjojojunior/simffb/master/simffb_pic.png)

### Description
simFFB is an invaluable utility that provides joystick force feedback effects beyond those typically offered in games, such as damper and friction, as well as enabling both progressive and instantaneous trimming modes. 

Originally developed by average_pilot and posted to the ED forums (https://forums.eagle.ru/showthread.php?t=84883), it was later modified by Jazzerman to add support for re-initializing dinput with a keybinding. Since average_pilot graciously posted the source code, I've added some extra features that improve on the original which I hope you will enjoy.

### New Features
* **Toggle Trim Button**: allows toggling instantaneous trimmer on and off (as opposed to hold).
* **Center Trim Button**: centers the joystick (only when instantaneous trimmer off).
* **Separate Damper/Friction Settings**: allows for independent damper/friction settings for instantaneous trimmer on/off.
  * **Damper Force / Friction Force**: applied when instantaneous trimmer on.
  * **Damper Force 2 / Friction Force 2**: applied when instantaneous trimmer off.
* **Save dinput Key**: saves the selected dinput re-initialization key in the options file.

### Building
Open solution file in Visual Studio 2019 and hit build :)
