# OpenAWELC
Open ELC (LED Effects) Firmware for Alienware machines.
Status: Working on the [m18 R2](https://www.dell.com/en-us/shop/dell-laptops/alienware-m18-r2-gaming-laptop/spd/alienware-m18-r2-laptop).

# How to use
Programming the embedded LED controller is possible, but it does require a kernel module. Fortunately, that is not very hard to set up.

```bash
(cd module && make && doas insmod ./alienware-elc-dfu.ko)
echo dfu >  /sys/kernel/alienware_elc_dfu/mode
```
Then you can use STM32CubeProgrammer to program the chip as usual. Open the file, select 'USB' in the interface, and click 'Download'. Booting the chip is also strightforward.

```bash
echo dfu >  /sys/kernel/alienware_elc_dfu/mode
```

# Customising colors
While it would be interesting to have HID communication working, it's not really a necessity. Patches are welcome.
However, a simpler approach is simply to edit the STM32CubeIDE project, compile and program yourself the project.

# Bugs or requests
Shoot me an email:  [Email Me](mailto:philmb3487@proton.me)
