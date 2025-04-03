#!/bin/bash

# Exit on error
set -e

echo "Copying: /mnt/remote/r5/zephyr_mcu_neopixel.elf /lib/firmware/"
sudo cp /mnt/remote/r5/zephyr_mcu_neopixel.elf /lib/firmware/

echo "Loading: zephyr_mcu_neopixel.elf to /sys/class/remoteproc/remoteproc2/firmware"
echo zephyr_mcu_neopixel.elf | sudo tee /sys/class/remoteproc/remoteproc2/firmware

echo "Starting: /sys/class/remoteproc/remoteproc2/state"
echo start | sudo tee /sys/class/remoteproc/remoteproc2/state

echo "WORK AROUND: Reading/Writing the pins to set the MUX / direction"
gpioset gpiochip0 9=1   # PYMNL.9
gpioset gpiochip0 7=1   # LED Stick
gpioget gpiochip0 10    # Rotary Encoder - Push
