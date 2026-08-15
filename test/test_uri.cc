#include "../sylar/uri.h"
#include <iostream>
int main()
{
    mysylar::Uri::ptr uri =
        mysylar::Uri::Create("http://www.baidu.com:8080/index.html?name=sylar#tag1");
    std::cout << uri->toString() << std::endl;
    std::cout << uri->getScheme() << std::endl;
    std::cout << uri->getUserinfo() << std::endl;
    std::cout << uri->getHost() << std::endl;
    std::cout << uri->getPort() << std::endl;
    std::cout << uri->getPath() << std::endl;
    std::cout << uri->getQuery() << std::endl;
    std::cout << uri->getFragment() << std::endl;

    auto addr = uri->createAddress();
    std::cout << addr->toString() << std::endl;

    return 0;
}