# Development Guide - Ubuntu

## C/C++ Installation

<https://medium.com/@ppatil/avr-programing-using-avrdude-in-ubuntu-93734c26ad19>

## VS Code

### Install VS Code (Ubuntu)

- Open the `App Center`
- Search for `code`
- Install

### Install VS Code Plugins

<https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>
<https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-themes>
<https://marketplace.visualstudio.com/items?itemName=geeebe.duplicate>
<https://marketplace.visualstudio.com/items?itemName=chadalen.vscode-jetbrains-icon-theme>
<https://marketplace.visualstudio.com/items?itemName=DavidAnson.vscode-markdownlint>
<https://marketplace.visualstudio.com/items?itemName=redhat.vscode-yaml>
<https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide>

### Install PlatformIO Rules

<https://docs.platformio.org/en/latest/core/installation/udev-rules.html> 

## CLion Installation (Incomplete)

<https://blog.jetbrains.com/clion/2020/08/arduino-from-hobby-to-prof-p1/>

### Install PlatformIO (Ubuntu)

```shell
sudo apt-get update
sudo apt-get install gcc build-essential
sudo apt-get install gcc-avr binutils-avr avr-libc gdb-avr
sudo apt-get install avrdude
sudo apt-get install libusb-dev

sudo apt  install curl

mkdir -p /home/$USER/Downloads
cd /home/$USER/Downloads

curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py

python3 get-platformio.py

echo -e 'export PATH=$PATH:$HOME/.local/bin\n' >> /home/$USER/.profile

mkdir -p ~/.local/bin/

ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/platformio
ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio
ln -s ~/.platformio/penv/bin/piodebuggdb ~/.local/bin/piodebuggdb
```

### Install PlatformIO plugin for CLion

1. Open CLion
2. `Ctrl + Alt + S` > Plugins
3. Marketplace > Search `PlatformIO`
4. Install > Restart IDE

### Setup CLion

1. `Ctrl + Alt + S`  > `Build, Execution, Deployment`
2. `Toolchains` > `+` > `System`
    - Name: `Arduino`
    - C Compiler: `/home/<user>/.platformio/packages/toolchain-atmelavr/bin/avr-gcc`
    - C++ Compiler: `/home/<user>/.platformio/packages/toolchain-atmelavr/bin/avr-g++`
3. `CMake` > `+`
    - Name: `Nano`
    - Build Type: `nano`
    - Toolchain: `Arduino`
