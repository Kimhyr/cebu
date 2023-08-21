#include <cebu/parser.h>
#include <cebu/lexer.h> 
#include <cebu/syntax.h>

using namespace cebu;

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    parser parser;
    parser.load("/home/king/cebu/test3");
    std::cout << parser.source() << std::endl;
    method_definition out;
    parser.parse<method_definition>(out);
    std::cout << std::format("{}", out) << std::endl;
}
