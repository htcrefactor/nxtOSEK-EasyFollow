# nxtOSEK-EasyFollow
This repository contains the result of our implementation of Easy-Follow.  
Video of original Easy-Follow implementation by Henrik Koch: https://www.youtube.com/watch?v=4z3L1nXw8uE

## Prerequisites
- Lego Mindstorm EV2 with nxtOSEK
- ECRobot C API (http://lejos-osek.sourceforge.net/ecrobot_c_api.htm)
- Ubuntu build with appropriate toolchains preinstalled (http://lejos-osek.sourceforge.net/installation_linux.htm)

## How to build
1. Clone this repository to your Ubuntu machine.
2. Navigate to the directory you wish to build using the terminal.
3. Run `make all` to get a `.rxe` format file.
4. Copy the `.rxe` file to Windows, and use NextTool.exe to download to your Lego NXT.  
`NextTool.exe COM=usb -download=source_file`  
`source_file` should be replaced with the `.rxe` file including extension.
5. Run on your Lego NXT.
