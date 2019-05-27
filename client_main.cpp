//
// Created by roman on 20.05.19.
//

#include <iostream>
#include "client.h"
#include <stdio.h>

using std::cin;
using std::cout;
using std::string;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "client [server address] [port]" << std::endl;
        return 0;
    }
    try {
        client cl;
        try {
            cl.connect_to(argv[1], std::atoi(argv[2]));
        } catch (std::runtime_error &error) {
            std::cerr << error.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        try {
            cl.start();
        } catch (std::runtime_error &error) {
            std::cerr << error.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    } catch (std::runtime_error &constr_er) {
        std::cerr << constr_er.what() << std::endl;
        exit(EXIT_FAILURE);
    }


    return 0;
}