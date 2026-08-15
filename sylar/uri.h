#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__
#include "address.h"
#include <memory>
#include <stdint.h>
#include <string>
namespace mysylar
{
/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/

class Uri
{
  public:
    typedef std::shared_ptr<Uri> ptr;

    static Uri::ptr Create(const std::string& uri);

    Uri();

    const std::string& getScheme()
    {
        return m_scheme;
    }
    const std::string& getUserinfo()
    {
        return m_userinfo;
    }
    const std::string& getHost()
    {
        return m_host;
    }
    int32_t getPort() const;
    const std::string& getPath() const;
    const std::string& getQuery()
    {
        return m_query;
    }
    const std::string& getFragment()
    {
        return m_fragment;
    }

    void setScheme(const std::string& v)
    {
        m_scheme = v;
    }
    void setUserinfo(const std::string& v)
    {
        m_userinfo = v;
    }
    void setHost(const std::string& v)
    {
        m_host = v;
    }
    void setPort(const int32_t& v)
    {
        m_port = v;
    }
    void setPath(const std::string& v)
    {
        m_path = v;
    }
    void setQuery(const std::string& v)
    {
        m_query = v;
    }
    void setFragment(const std::string& v)
    {
        m_fragment = v;
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    Address::ptr createAddress() const;

  private:
    bool isDefaultPort() const;

  private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    int32_t m_port;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
};

}; // namespace mysylar

#endif // __SYLAR_URI_H__