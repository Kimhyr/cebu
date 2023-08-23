#include <cebu/parser.h>
#include <cebu/lexer.h> 
#include <cebu/syntax.h>

using namespace cebu;

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    parser parser;
    parser.load("/home/king/cebu/test3");
    do {
        parser.consume();
        std::cout << std::format("{}", parser.token()) << std::endl;
    } while (parser.token() != token_type::end);
}
