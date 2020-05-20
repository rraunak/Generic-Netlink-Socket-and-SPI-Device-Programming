Raunak
ID- 1217240245


##############################################################################
################################## OUTPUT ###################################

Welcome to distance measurement and pattern display
Enter the chip select pin
12
Enter the echo pin
5
Enter the trigger pin
10
Setting the pins
User: cs_pin=12
User: trig_pin=10
User: echo_pin=5
Userhcsr: size=3
Pins set
Enter 1 to start the distance measurement and pattern display
1
User: jsize=4
After: inside jumping man
sequence 1
sequence 2
sequence 3
sequence 4
User: dist_enable=1
Userde: size=1
Kernel Says: 0
sequence 5
sequence 6

##############################################################################
########################### INSTRUCTIONS ####################################

1. Run the make file to build the modules.

2. Connect the board to the PC by following the setting_up_galileo_gen2 pdf uploaded on the canvas.

3. Transfer the modules, spi_tester and genl_drv.ko to the board through the scp command.

e.g. sudo scp -r raunak@192.168.1.233:/home/raunak/eosi/gpio/assign2/hscr04.ko root@192.168.1.5:/home/hcsrf

4. Once the modules are transferred, do insmod to install the genl_drv.ko module.

e.g. insmod genl_drv.ko

5. Now, run the spi_tester, it will ask for chip select, echo and trigger pins. Enter the pins.

6. Now, it will ask for the measurement request, Enter 1 to start the measurement.

7. Based on the distance, it will display the pattern.

8. stop the distance measurement and pattern display by pressing ctrl+z.

9. Remove the module using rmmod genl_drv.ko.


