# BeagleBone Black I2C
## Description:
Header and source files for interacting with the BeagleBone Black I2C bus.
## Platform
BeagleBone Black, Rev C, running Debian 9.3 (iot)
## Details
### Xfer
The Xfer function is intended to be used with devices that require a
register address to be written to the device prior to reading from
that register.

Xfer also allows for sequentially reading additional registers,
starting with the register specified by the addr parameter.

### Threading
Public functions Read, Write, and Xfer are thread-safe, as each of them
locks the mutex associated with the bus before proceeding.

Protected functions Open and Close operate under the assumption that
exclusive use of the bus has already been obtained.
