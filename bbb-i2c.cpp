/*
 * bbb-i2c.cpp
 *
 *  Created on: Aug 26, 2018
 *      Author: JSRagman
 *
 *  Description:
 *    Implements BeagleBone Black I2C bus and supporting classes.
 *
 *    Note the promiscuous use of local variables. Some day my
 *    code will be more lean and terse, but that day is not yet.
 */


#include "bbb-i2c.h"

#include <fcntl.h>
#include <iomanip>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>


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
 *   exception's originating process (function).
 *
 * Parameters:
 *   msg  - An error message. This is what will be returned
 *          by what() and why().
 *   proc - A procedure or function name. This is what will be
 *          returned by where().
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
I2CException::I2CException(const string& msg, const string& proc)
{
	message  = msg;
	procname = proc;
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
 *   bbb-i2c.h
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
 *   bbb-i2c.h
 */
string I2CException::why()
{
	return message;
}

/*
 * string I2CException::where()
 *
 * Description:
 *   Returns the name of the exception's originating
 *   process (function).
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
string I2CException::where()
{
	return procname;
}



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
 *   bbb-i2c.h
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
 *   bbb-i2c.h
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
 *   parameter. Throws an I2CException if the bus cannot be
 *   opened or if the target device does not respond.
 *
 *   Consider using a separate I2C "Not Found" exception
 *   to distinguish the lesser severity of a device that
 *   does not respond.
 *
 * Parameters:
 *   addr - A device address.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Open(uint8_t addr)
{
	string proc = "I2CBus::Open(uint8_t addr)";

	file = ::open(busfile, O_RDWR);
	if (file < 0)
	{
		stringstream ss;
		ss << "Unable to open I2C Bus file " << busfile;
		I2CException iexc(proc, ss.str());
		throw iexc;
	}

	int ioresult = ioctl(file, I2C_SLAVE, addr);
	if (ioresult < 0)
	{
		this->Close();

		stringstream ss;
		ss << "Unable to find device address ";
		ss << "0x" << hex << uppercase << setfill('0') << setw(2) << ((unsigned int)addr);
		I2CException iexc(proc, ss.str());
		throw iexc;
	}
}

/*
 * void I2CBus::Close()
 *
 * Description:
 *   Closes the bus connection (if it is open).
 *
 *   The closeresult test is taken straight from the close
 *   function documentation.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
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
 *   Gets posession of the I2CBus and then reads one or more
 *   bytes from the device at the specified address.
 *
 * Parameters:
 *   data - a buffer to receive data
 *   len  - the number of bytes to be read
 *   addr - the target device's I2C address.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Read(uint8_t* data, int len, uint8_t addr)
{
	lock_guard<mutex> lck(mtx);

	string proc  = "I2CBus::Read(data, len, addr)";
	int    recvd =  0;

	this->Open(addr);
	recvd = ::read(file, data, len);
	this->Close();

	if (recvd != len)
	{
		I2CException iexc(proc, "Read length error.");
		throw iexc;
	}
}

/*
 * void I2CBus::Write(uint8_t* data, int len, uint8_t addr)
 *
 * Description:
 *   Gets posession of the I2CBus and then writes one or more
 *   bytes to the device at the specified address.
 *
 * Parameters:
 *   data - a buffer containing data to be written.
 *   len  - the number of bytes to be written.
 *   addr - the target device's I2C address.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Write(uint8_t* data, int len, uint8_t addr)
{
	lock_guard<mutex> lck(mtx);

	string proc = "I2CBus::Write(data, len, addr)";
	int    sent = 0;

	this->Open(addr);
	sent = ::write(file, data, len);
	this->Close();

	if (sent != len)
	{
		I2CException iexc(proc, "Write length error.");
		throw iexc;
	}
}

/*
 * void I2CBus::Write(const string& dat, uint8_t addr)
 *
 * Description:
 *   Gets posession of the I2CBus and then writes string
 *   data to the device at the specified address.
 *
 * Parameters:
 *   dat  - a string containing data to be written to an I2C device.
 *   addr - the address of the target device.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Write(const string& dat, uint8_t addr)
{
	lock_guard<mutex> lck(mtx);

	string proc = "I2CBus::Write(const string& dat, uint8_t addr)";
	int sent = 0;
	int len  = dat.size();

	this->Open(addr);
	sent = ::write(file, dat.c_str(), len);
	this->Close();

	if (sent != len)
	{
		I2CException iexc(proc, "Write length error.");
		throw iexc;
	}
}

/*
 * void I2CBus::Xfer(uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr)
 *
 * Description:
 *   Gets posession of the I2CBus, writes one or more bytes to the device, and
 *   then reads one or more bytes from the device.
 *
 * Parameters:
 *   odat    - data buffer that contains the data to be written.
 *   olen    - the number of bytes to be written.
 *   idat    - buffer that will receive data read from the device.
 *   ilen    - number of bytes to be read from the device.
 *   i2caddr - address of the target device.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Xfer(uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr)
{
	int    count = 0;
	string proc  = "I2CConnection::Xfer(odat, olen, idat, ilen, i2caddr)";

	lock_guard<mutex> lck(mtx);

	this->Open(i2caddr);
	count = ::write(file, odat, olen);
	if (count != olen)
	{
		this->Close();
		I2CException iexc(proc, "Write length error.");
		throw iexc;
	}

	count = ::read(file, idat, ilen);
	this->Close();

	if (count != ilen)
	{
		I2CException iexc(proc, "Read length error.");
		throw iexc;
	}
}

/*
 * void I2CBus::Xfer(uint8_t startaddr, uint8_t* idat, int ilen, uint8_t i2caddr)
 *
 * Description:
 *   Gets posession of the I2CBus, writes one byte to the device, and
 *   then reads one or more bytes from the device.
 *
 * Parameters:
 *   startaddr - one byte of data to be written.
 *   idat      - a buffer that will receive data read from the device.
 *   ilen      - the number of bytes to be read from the bus.
 *   i2caddr   - the address of the target device.
 *
 * Exceptions:
 *   I2CException
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
void I2CBus::Xfer(uint8_t addr, uint8_t* idat, int ilen, uint8_t i2caddr)
{
	lock_guard<mutex> lck(mtx);

	string proc = "I2CConnection::Xfer(uint8_t addr, uint8_t* idat, int ilen, uint8_t i2caddr)";

	int sent  =  0;
	int recvd =  0;

	this->Open(i2caddr);
	sent = ::write(file, &addr, 1);
	if (sent != 1)
	{
		this->Close();
		I2CException iexc(proc, "Write length error.");
		throw iexc;
	}

	recvd = ::read(file, idat, ilen);
	this->Close();

	if (recvd != ilen)
	{
		I2CException iexc(proc, "Read length error.");
		throw iexc;
	}
}



} // namespace bbbi2c
