```
/*
 * bbb-i2c.hpp
 *
 *  Created on: Sep 2, 2018
 *      Author: JSRagman
 *
 *  Description:
 *    BeagleBone Black I2C bus header.
 */

#ifndef BBB_I2C_HPP_
#define BBB_I2C_HPP_


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
 *   Provides an error message, the name of the process
 *   where the mishap occurred, and a time stamp.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
 */
class I2CException : public std::exception
{
  protected:
    string message;
    string procname;
    time_t timeof_exc;

  public:
    I2CException (const string& msg, const string& proc);

    const char* what();
    string who ();
    string why ();
    time_t when ();
};


class I2CNotFoundException : public I2CException
{
  public:
    I2CNotFoundException ( const string& msg, const string& proc );
};


/*
 * class I2CBus
 *
 * Description:
 *   Represents a BeagleBone Black I2C bus.
 *
 *   The intention here is that I2C Read, Write, and Xfer
 *   functions should open a connection, do their business,
 *   and then close the connection on exit.
 *
 * Namespace:
 *   bbbi2c
 *
 * Header File(s):
 *   bbb-i2c.hpp
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

    void Read  ( uint8_t* data, int len, uint8_t i2caddr );
    void Write ( uint8_t* data, int len, uint8_t i2caddr );
    void Write ( const string& dat, uint8_t i2caddr );
    void Xfer  ( uint8_t* odat, int olen, uint8_t* idat, int ilen, uint8_t i2caddr );
    void Xfer  ( uint8_t  addr, uint8_t* idat, int ilen, uint8_t i2caddr );

}; // class I2CBus

} // namespace bbbi2c

#endif /* BBB_I2C_HPP_ */
```
