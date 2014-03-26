pcsensor2
=========

Reads temperature from a TEMPerV1.2 USB thermometer (VID/PID 0c45:7401).

The source code is a nice and clean example of using libusb in simple blocking mode.

The posttemp subdirectory contains scripts which read 2 Tempers and send their values to a PHP script using curl. For the scripts you'll need my nlogrotate script which can be found in a separate git repo [here](https://github.com/nonoo/nlogrotate).
