/*
 * C++
 * bbb-i2c.h
 *
 *  Created on: Aug 26, 2018
 *      Author: JSRagman
 *
 *  Description:
 *    BeagleBone Black I2C bus and supporting classes.
 *
 *  NOTE:
 *    This header was written by JSRagman, who is NOT a professional
 *    programmer.  Use it, if you like, but don't stake your life on it.
 */

#ifndef BBB_I2C_H_
#define BBB_I2C_H_


#include <exception>
#include <mutex>
#include <stdint.h>
#include <string>


using std::string;


// BBB I2C Bus File Names
#define BBB_I2C0_FILE  "/dev/i2c-0"
#define BBB_I2C1_FILE  "/dev/i2c-1"
#define BBB_I2C2_FILE  "/dev/i2c-2"


namespace bbbi2c
{

/*
 * class I2CException : public exception
 *
 * Description:
 *   Represents an I2C process error.
 *
 *   Provides an error message and the name of the process
 *   where the mishap occurred.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
class I2CException : public std::exception
{
  protected:
	string message;
	string procname;

  public:
	I2CException (const string& msg, const string& proc);

    const char* what();
	string where ();
	string why   ();
};



/*
 * class I2CBus
 *
 * Description:
 *   Represents a BeagleBone Black I2C bus.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.h
 */
class I2CBus
{
  protected:

	const char*  busfile;        // I2C bus file name.
	int          file;           // File descriptor.

	void Open  ( uint8_t addr );
	void Close ();

  public:

	std::mutex mtx;

	I2CBus ( const char* bus );
   ~I2CBus ();

    void Read ( uint8_t* data, int len, uint8_t i2caddr );

    void Write ( uint8_t* data, int len, uint8_t i2caddr );
    void Write ( const string& dat, uint8_t i2caddr );

    void Xfer ( uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr );
    void Xfer ( uint8_t  addr, uint8_t* idat, int ilen, uint8_t i2caddr);

}; // class I2CBus



} // namespace bbbi2c

#endif /* BBB_I2C_H_ */
