//----------------------------------------------------------------------------
/** @file YException.hpp */
//----------------------------------------------------------------------------

#ifndef YEXCEPTION_H
#define YEXCEPTION_H

#include <string>
#include <sstream>
#include <exception>

//----------------------------------------------------------------------------

/** Base class for exceptions. 
    Usage examples:
    @verbatim
      YException("Message");
      YException() << "Message" << data << "more message.";
    @endverbatim
 */
class YException
    : public std::exception
{
public:
    /** Constructs an exception with no message. */
    YException()
        : std::exception(),
          m_stream("")
    { }

    /** Construct an exception with the given message. */
    YException(const std::string& message)
        : std::exception(),
          m_stream(message)
    { }

    /** Needed for operator<<. */
    YException(const YException& other)
        : std::exception()
    {
        m_stream << other.Response();
        m_stream.copyfmt(other.m_stream);
    }

    /** Destructor. */
    virtual ~YException() throw()
    { }

    /** Returns the error message. */
    const char* what() const throw()
    {
        // Cannot just do 'Response().c_str()' because the temporary will die
        // before the message is printed.
        m_what = Response(); 
        return m_what.c_str();
    }

    std::string Response() const
    { return m_stream.str(); }

    std::ostream& Stream()
    { return m_stream; }

private:
    std::ostringstream m_stream;

    mutable std::string m_what;
};

//----------------------------------------------------------------------------

/** @relates YException
    @note Returns a new object, see @ref YException
*/
template<typename TYPE>
YException operator<<(const YException& except, const TYPE& type)
{
    YException result(except);
    result.Stream() << type;
    return result;
}

/** @relates YException
    @note Returns a new object, see @ref YException
*/
template<typename TYPE>
YException operator<<(const YException& except, TYPE& type)
{
    YException result(except);
    result.Stream() << type;
    return result;
}

//----------------------------------------------------------------------------

#endif // YEXCEPTION_H
