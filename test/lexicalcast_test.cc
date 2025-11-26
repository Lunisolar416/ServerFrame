#include <boost/lexical_cast.hpp>
#include <iostream>
int main()
{

    const char* str = "2952";

    int i = 0;
    i = boost::lexical_cast<int>(str);
    std::cout << "number:" << i << std::endl;
    return 0;
}