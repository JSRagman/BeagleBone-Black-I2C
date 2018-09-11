```
/*
 * bbb-i2c.cpp
 *
 *  Created on: Aug 26, 2018
 *      Author: JSRagman
 *
 *  Revised:
 *    9 Sept, 2018
 *
 *  Description:
 *    Implements BeagleBone Black I2C bus and supporting classes.
 */


#include "bbb-i2c.hpp"

#include <exception>         // exception
#include <fcntl.h>           // open(), O_RDWR
#include <iomanip>           // hex, uppercase, setfill(), setw()
#include <linux/i2c.h>
#include <linux/i2c-dev.h>   // I2C_SLAVE
#include <mutex>             // mutex, lock_guard
#include <sstream>           // stringstream
#include <stdint.h>          // int8_t, uint8_t
#include <string>            // string
#include <sys/ioctl.h>       // ioctl
#include <unistd.h>          // close(), TEMP_FAILURE_RETRY


using namespace std;

namespace bbbi2c
{

// I2CException
// ------------------------------------------------------------------

/*
 * I2CException(const string& msg, const string& proc)
 *
 * Description:
 *   Constructor.  Sets the error message and the name of the
 *   exception's originating process. Records the time.
 *
 * Parameters:
 *   msg  - An error message. This is what will be returned
 *          by what() and why().
 *   proc - A procedure or function name. This is what will be
 *          returned by who().
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
I2CException::I2CException(const string& msg, const string& proc)
{
    message    = msg;
    procname   = proc;
    timeof_exc = time(nullptr);
}

/*
 * const char* I2CException::what()
 *
 * Description:
 *   A nod to the red-headed stepchild std::exception.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
const char* I2CException::what()
{
    return message.c_str();
}

/*
 * string I2CException::why()
 *
 * Description:
 *   Returns the error message. Functions the same as
 *   std::exception::what() but returns a string instead
 *   of a c_string.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
string I2CException::why()
{
    return message;
}

/*
 * string I2CException::who()
 *
 * Description:
 *   Returns the name of the exception's originating
 *   process (function).
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
string I2CException::who()
{
    return procname;
}

/*
 * time_t I2CException::when()
 *
 * Description:
 *   Returns the exception's time stamp.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
time_t I2CException::when()
{
    return timeof_exc;
}


// I2CNotFoundException
// ------------------------------------------------------------------

/*
 * I2CNotFoundException::I2CNotFoundException(const string& msg, const string& proc)
 *
 * Description:
 *   Constructor. Initializes the base I2CException.
 *
 *   The I2CNotFoundException's reason for existence is to distinguish
 *   between the severity of an I2C bus that won't open and an I2C device
 *   that doesn't respond.
 *
 * Parameters:
 *   msg  - An error message. This is what will be returned
 *          by what() and why().
 *   proc - A procedure or function name. This is what will be
 *          returned by who().
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
I2CNotFoundException::I2CNotFoundException(const string& msg, const string& proc)
    : I2CException(msg, proc)
{ }



// I2CBus Constructor, Destructor
// ------------------------------------------------------------------

/*
 * I2CBus::I2CBus(const char* bus)
 *
 * Description:
 *   Constructor.  Sets the bus file name. Initializes the I2C file
 *   descriptor to -1.
 *
 *   Does not attempt to open the bus or even verify that the
 *   bus exists.
 *
 * Parameters:
 *   bus - The I2C bus file name.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
I2CBus::I2CBus(const char* bus)
{
    busfile = bus;
    file    = -1;
}

/*
 * I2CBus::~I2CBus()
 *
 * Description:
 *   Destructor. Ensures that file is closed before
 *   going down the drain.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
I2CBus::~I2CBus()
{
    this->Close();
}


// I2CBus Protected
// ------------------------------------------------------------------

/*
 * void I2CBus::Open(uint8_t addr)
 *
 * Description:
 *   Opens a connection to the device specified by the addr
 *   parameter.
 *
 * Parameters:
 *   addr - I2C address of the target device
 *
 * Exceptions:
 *   I2CException
 *   I2CNotFoundException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Open(uint8_t addr)
{
    file = ::open(busfile, O_RDWR);
    if (file < 0)
    {
        stringstream ss;
        ss << "Unable to open I2C Bus file " << busfile;
        I2CException iexc("I2CBus::Open(addr)", ss.str());
        throw iexc;
    }

    int ioresult = ioctl(file, I2C_SLAVE, addr);
    if (ioresult < 0)
    {
        this->Close();
        stringstream ss;
        ss << "Unable to find device address ";
        ss << "0x" << hex << uppercase << setfill('0') << setw(2) << (unsigned int)addr;
        I2CNotFoundException nfexc("I2CBus::Open(addr)", ss.str());
        throw nfexc;
    }
}

/*
 * void I2CBus::Close()
 *
 * Description:
 *   Closes the bus connection (if it is open).
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Close()
{
    errno = 0;

    if (file != -1)
    {
        int closeresult = ::close(file);
        if (closeresult < 0)
        {
            if (errno == EINTR)
            {
                TEMP_FAILURE_RETRY (::close(file));
            }
        }

        file = -1;
    }
}



// I2CBus Public
// ------------------------------------------------------------------

/*
 * void I2CBus::Read(uint8_t* data, int len, uint8_t addr)
 *
 * Description:
 *   Acquires posession of the I2C bus and reads one or more bytes
 *   from the device at the specified address.
 *
 * Parameters:
 *   data - a buffer to receive data
 *   len  - the number of bytes to be read
 *   addr - I2C address of the target device
 *
 * Exceptions:
 *   I2CException
 *   I2CNotFoundException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Read(uint8_t* data, int len, uint8_t addr)
{
    lock_guard<mutex> lck(mtx);

    int recvd = 0;

    this->Open(addr);
    recvd = ::read(file, data, len);
    this->Close();

    if (recvd != len)
    {
        I2CException iexc("I2CBus::Read(data, len, addr)", "Read length error.");
        throw iexc;
    }
}

/*
 * void I2CBus::Write(uint8_t* data, int len, uint8_t addr)
 *
 * Description:
 *   Acquires posession of the I2C bus and writes one or more bytes
 *   to the device at the specified address.
 *
 * Parameters:
 *   data - a buffer containing data to be written
 *   len  - the number of bytes to be written
 *   addr - I2C address of the target device
 *
 * Exceptions:
 *   I2CException
 *   I2CNotFoundException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Write(uint8_t* data, int len, uint8_t addr)
{
    lock_guard<mutex> lck(mtx);

    int sent = 0;

    this->Open(addr);
    sent = ::write(file, data, len);
    this->Close();

    if (sent != len)
    {
        I2CException iexc("I2CBus::Write(data, len, addr)", "Write length error.");
        throw iexc;
    }
}

/*
 * void I2CBus::Write(const string& dat, uint8_t addr)
 *
 * Description:
 *   Acquires posession of the I2C bus and writes string data to the
 *   device at the specified address.
 *
 * Parameters:
 *   dat  - a string containing data to be written
 *   addr - I2C address of the target device
 *
 * Exceptions:
 *   I2CException
 *   I2CNotFoundException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Write(const string& dat, uint8_t addr)
{
    lock_guard<mutex> lck(mtx);

    int len  = dat.size();
    int sent = 0;

    this->Open(addr);
    sent = ::write(file, dat.c_str(), len);
    this->Close();

    if (sent != len)
    {
        I2CException iexc("I2CBus::Write(dat, addr)", "Write length error.");
        throw iexc;
    }
}

/*
 * void I2CBus::Xfer(uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr)
 *
 * Description:
 *   Acquires posession of the I2C bus, writes one or more bytes to
 *   the device, and then reads one or more bytes from the device at
 *   the specified address.
 *
 * Parameters:
 *   odat    - data buffer that contains the data to be written
 *   olen    - the number of bytes to be written
 *   idat    - buffer that will receive data read from the device
 *   ilen    - number of bytes to be read
 *   i2caddr - I2C address of the target device
 *
 * Exceptions:
 *   I2CException
 *   I2CNotFoundException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
void I2CBus::Xfer(uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr)
{
    lock_guard<mutex> lck(mtx);

    int count = 0;

    this->Open(i2caddr);
    count = ::write(file, odat, olen);
    if (count != olen)
    {
        this->Close();
        I2CException iexc("I2CConnection::Xfer(odat, olen, idat, ilen, i2caddr)", "Write length error.");
        throw iexc;
    }

    count = ::read(file, idat, ilen);
    this->Close();
    if (count != ilen)
    {
        I2CException iexc("I2CConnection::Xfer(odat, olen, idat, ilen, i2caddr)", "Read length error.");
        throw iexc;
    }
}

} // namespace bbbi2c
```
