//
// Created by 飞飞 on 2020-02-03.
//



#ifndef TRPROBE_SIGNALHANDLE_H
#define TRPROBE_SIGNALHANDLE_H
#include <signal.h>


#endif //TRPROBE_SIGNALHANDLE_H

extern void set_signal_handler_4_posix();

extern void posix_signal_handler(int sig, siginfo_t *siginfo, void *context);