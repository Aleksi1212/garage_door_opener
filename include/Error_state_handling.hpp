#ifndef ERROR_STATE_HANDLING_HPP
#define ERROR_STATE_HANDLING_HPP
#include <gpio.hpp>


class error_status {
    private:
        GPIOPin led1;
        GPIOPin led2;
        GPIOPin led3;
    public:
        error_status();
        void report();
};




#endif //ERROR_STATE_HANDLING_H
