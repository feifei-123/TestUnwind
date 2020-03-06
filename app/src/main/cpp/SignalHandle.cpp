//
// Created by 飞飞 on 2020-02-03.
//

#include "SignalHandle.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include "mylog.h"

#include <dlfcn.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
//#include <trackpicker.h>

char* LINESEPERATOR = "\t\n";

/* Thread-specific crash handler structure. */
typedef struct native_code_handler_struct {
    /* Restore point context. */
    sigjmp_buf ctx;
    int ctx_is_set;
    int reenter;

    /* Alternate stack. */
    char *stack_buffer;
    size_t stack_buffer_size;
    stack_t stack_old;

    /* Signal code and info. */
    int code;
    siginfo_t si;
    ucontext_t uc;

    /* Uwind context. */
#if (defined(USE_CORKSCREW))
    backtrace_frame_t frames[BACKTRACE_FRAMES_MAX];
#elif (defined(USE_UNWIND))
    uintptr_t frames[BACKTRACE_FRAMES_MAX];
#endif
#ifdef USE_LIBUNWIND
    void* uframes[BACKTRACE_FRAMES_MAX];
#endif
    size_t frames_size;
    size_t frames_skip;

    /* Custom assertion failures. */
    const char *expression;
    const char *file;
    int line;

    /* Alarm was fired. */
    int alarm;
} native_code_handler_struct;
static ssize_t coffeecatch_unwind_signal(
                                         void** frames,
                                         size_t max_depth);


#define BACKTRACE_FRAMES_MAX 32
const int kExceptionSignals[] = {
        SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP
};

//--- 信号量个数
const int kNumHandledSignals =
        sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);

//--- 每个信号的原有信号处理函数
struct sigaction old_handlers[kNumHandledSignals];

/* Alternative stack size. */
#define SIG_STACK_BUFFER_SIZE (SIGSTKSZ)
char* alternate_stack;
void* uframes2[BACKTRACE_FRAMES_MAX] = {0};
native_code_handler_struct *const t =
        ( native_code_handler_struct *)calloc(sizeof(native_code_handler_struct), 1);


const char* getCrashSingalDes(int sig, int code);
const char * getCrashSingalCodeDes(int sig, int code);
const char * getCrashSingalNoDes(int sig, int code);
void printStack(int frames_size,void * frames[]);
void set_signal_handler_4_posix()
{

    ALOGD("set_signal_handler_4_posix,process:%d,thread:%d",getpid(),gettid());
    /* setup alternate stack */
    {
        stack_t stack;
        /* Initialize structure */
        t->stack_buffer_size = SIG_STACK_BUFFER_SIZE;
        t->stack_buffer = (char*)malloc(t->stack_buffer_size);
        if (t->stack_buffer == NULL) {

            ALOGD("t->stack_buffer  malloc failed");
        }

        /* Setup alternative stack. */
        memset(&stack, 0, sizeof(stack));
        stack.ss_sp = t->stack_buffer;
        stack.ss_size = t->stack_buffer_size;
        stack.ss_flags = 0;


        if (sigaltstack(&stack, NULL) != 0) {
            ALOGD("sigaltstack failed~");
            err(1, "sigaltstack");
        }

    }

    ALOGD("sigaltstack success~");
    /* register our signal handlers */
    {

        struct sigaction sig_action = {};
        memset(&sig_action, 0, sizeof(sig_action));
        sigemptyset(&sig_action.sa_mask);
        sig_action.sa_sigaction = posix_signal_handler;
        sig_action.sa_flags = SA_SIGINFO
                | SA_ONSTACK;

        for(int i = 0;i<kNumHandledSignals;i++ ){
            ALOGD("install sigaction for signal  %d\n",kExceptionSignals[i]);
            if(sigaction(kExceptionSignals[i], &sig_action, &old_handlers[i])==-1){
                ALOGD("set sinalaction failed,just return");
            }
        }
    }
}

void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
    ALOGD("enter posix_signal_handler,进程ID:%d,线程ID:%d",getpid(),gettid());
    ALOGD("posix_signal_handler singal no:%d (%s),sinal code:%d (%s) ----- :%s",siginfo->si_signo,
          getCrashSingalNoDes(siginfo->si_signo, siginfo->si_code),siginfo->si_code,
          getCrashSingalCodeDes(siginfo->si_signo,siginfo->si_code),
          getCrashSingalDes(siginfo->si_signo,siginfo->si_code));

    signal(sig, SIG_DFL);
//    void* uframes2[BACKTRACE_FRAMES_MAX] = {0};
    ssize_t  n = coffeecatch_unwind_signal(uframes2,BACKTRACE_FRAMES_MAX);
    printStack(n,uframes2);
//    _Exit(1);

}


const char * getCrashSingalNoDes(int sig, int code){

    switch(sig) {
        case SIGILL:
            return "SIGILL";
        case SIGFPE:
            return "SIGFPE";
            break;
        case SIGSEGV:
            return "SIGSEGV";
            break;
        case SIGBUS:
            return "SIGBUS";
            break;
        case SIGTRAP:
            return "SIGTRAP";
            break;
        case SIGCHLD:
            return "SIGCHLD";
            break;
        case SIGPOLL:
            return "SIGPOLL";
        case SIGABRT:
            return "SIGABRT";
        case SIGALRM:
            return "SIGALRM";
        case SIGCONT:
            return "SIGCONT";
        case SIGHUP:
            return "SIGHUP";
        case SIGINT:
            return "SIGINT";
        case SIGKILL:
            return "SIGKILL";
        case SIGPIPE:
            return "SIGPIPE";
        case SIGQUIT:
            return "SIGQUIT";
        case SIGSTOP:
            return "SIGSTOP";
        case SIGTERM:
            return "SIGTERM";
        case SIGTSTP:
            return "SIGTSTP";
        case SIGTTIN:
            return "SIGTTIN";
        case SIGTTOU:
            return "SIGTTOU";
        case SIGUSR1:
            return "SIGUSR1";
        case SIGUSR2:
            return "SIGUSR2";
        case SIGPROF:
            return "SIGPROF";
        case SIGSYS:
            return "SIGSYS";
        case SIGVTALRM:
            return "SIGVTALRM";
        case SIGURG:
            return "SIGURG";
        case SIGXCPU:
            return "SIGXCPU";
        case SIGXFSZ:
            return "SIGXFSZ";
        default:
            return "unkown";
            break;
    }
}

const char * getCrashSingalCodeDes(int sig, int code){

    switch(sig) {
        case SIGILL:
            switch(code) {
                case ILL_ILLOPC:
                    return "IILL_ILLOPC";
                case ILL_ILLOPN:
                    return "ILL_ILLOPN";
                case ILL_ILLADR:
                    return "ILL_ILLADR";
                case ILL_ILLTRP:
                    return "ILL_ILLTRP";
                case ILL_PRVOPC:
                    return "ILL_PRVOPC";
                case ILL_PRVREG:
                    return "ILL_PRVREG";
                case ILL_COPROC:
                    return "ILL_COPROC";
                case ILL_BADSTK:
                    return "ILL_BADSTK";
                default:
                    return "ILL_BADSTK";
            }
            break;
        case SIGFPE:
            switch(code) {
                case FPE_INTDIV:
                    return "FPE_INTDIV";
                case FPE_INTOVF:
                    return "IFPE_INTOVF";
                case FPE_FLTDIV:
                    return "FPE_FLTDIV";
                case FPE_FLTOVF:
                    return "FPE_FLTOVF";
                case FPE_FLTUND:
                    return "FPE_FLTUND";
                case FPE_FLTRES:
                    return "FPE_FLTRES";
                case FPE_FLTINV:
                    return "FPE_FLTINV";
                case FPE_FLTSUB:
                    return "FPE_FLTSUB";
                default:
                    return "Floating-point";
            }
            break;
        case SIGSEGV:
            switch(code) {
                case SEGV_MAPERR:
                    return "SEGV_MAPERR";
                case SEGV_ACCERR:
                    return "SEGV_ACCERR";
                default:
                    return "Segmentation violation";
            }
            break;
        case SIGBUS:
            switch(code) {
                case BUS_ADRALN:
                    return "BUS_ADRALN";
                case BUS_ADRERR:
                    return "BUS_ADRALN";
                case BUS_OBJERR:
                    return "BUS_OBJERR";
                default:
                    return "Bus error";
            }
            break;
        case SIGTRAP:
            switch(code) {
                case TRAP_BRKPT:
                    return "TRAP_BRKPT";
                case TRAP_TRACE:
                    return "TRAP_TRACE";
                default:
                    return "Trap";
            }
            break;
        case SIGCHLD:
            switch(code) {
                case CLD_EXITED:
                    return "CLD_EXITED";
                case CLD_KILLED:
                    return "CLD_KILLED";
                case CLD_DUMPED:
                    return "CLD_DUMPED";
                case CLD_TRAPPED:
                    return "CLD_TRAPPED";
                case CLD_STOPPED:
                    return "CLD_STOPPED";
                case CLD_CONTINUED:
                    return "CLD_CONTINUED";
                default:
                    return "Child";
            }
            break;
        case SIGPOLL:
            switch(code) {
                case POLL_IN:
                    return "POLL_IN";
                case POLL_OUT:
                    return "POLL_OUT";
                case POLL_MSG:
                    return "POLL_MSG";
                case POLL_ERR:
                    return "POLL_ERR";
                case POLL_PRI:
                    return "POLL_PRI";
                case POLL_HUP:
                    return "POLL_HUP";
                default:
                    return "Pool";
            }
            break;
        case SIGABRT:
            return "SIGABRT";
        case SIGALRM:
            return "SIGALRM";
        case SIGCONT:
            return "SIGCONT";
        case SIGHUP:
            return "SIGHUP";
        case SIGINT:
            return "SIGINT";
        case SIGKILL:
            return "SIGKILL";
        case SIGPIPE:
            return "SIGPIPE";
        case SIGQUIT:
            return "SIGQUIT";
        case SIGSTOP:
            return "SIGSTOP";
        case SIGTERM:
            return "SIGTERM";
        case SIGTSTP:
            return "SIGTSTP";
        case SIGTTIN:
            return "SIGTTIN";
        case SIGTTOU:
            return "SIGTTOU";
        case SIGUSR1:
            return "SIGUSR1";
        case SIGUSR2:
            return "SIGUSR2";
        case SIGPROF:
            return "SIGPROF";
        case SIGSYS:
            return "SIGSYS";
        case SIGVTALRM:
            return "SIGVTALRM";
        case SIGURG:
            return "SIGURG";
        case SIGXCPU:
            return "SIGXCPU";
        case SIGXFSZ:
            return "SIGXFSZ";
        default:
            switch(code) {
                case SI_USER:
                    return "SI_USER";
                case SI_QUEUE:
                    return "SI_QUEUE";
                case SI_TIMER:
                    return "SI_TIMER";
                case SI_ASYNCIO:
                    return "SI_ASYNCIO";
                case SI_MESGQ:
                    return
                            "SI_MESGQ";
                default:
                    return "Unknown signal";
            }
            break;
    }
}
const char* getCrashSingalDes(int sig, int code) {
    switch(sig) {
        case SIGILL:
            switch(code) {
                case ILL_ILLOPC:
                    return "Illegal opcode";
                case ILL_ILLOPN:
                    return "Illegal operand";
                case ILL_ILLADR:
                    return "Illegal addressing mode";
                case ILL_ILLTRP:
                    return "Illegal trap";
                case ILL_PRVOPC:
                    return "Privileged opcode";
                case ILL_PRVREG:
                    return "Privileged register";
                case ILL_COPROC:
                    return "Coprocessor error";
                case ILL_BADSTK:
                    return "Internal stack error";
                default:
                    return "Illegal operation";
            }
            break;
        case SIGFPE:
            switch(code) {
                case FPE_INTDIV:
                    return "Integer divide by zero";
                case FPE_INTOVF:
                    return "Integer overflow";
                case FPE_FLTDIV:
                    return "Floating-point divide by zero";
                case FPE_FLTOVF:
                    return "Floating-point overflow";
                case FPE_FLTUND:
                    return "Floating-point underflow";
                case FPE_FLTRES:
                    return "Floating-point inexact result";
                case FPE_FLTINV:
                    return "Invalid floating-point operation";
                case FPE_FLTSUB:
                    return "Subscript out of range";
                default:
                    return "Floating-point";
            }
            break;
        case SIGSEGV:
            switch(code) {
                case SEGV_MAPERR:
                    return "Address not mapped to object";
                case SEGV_ACCERR:
                    return "Invalid permissions for mapped object";
                default:
                    return "Segmentation violation";
            }
            break;
        case SIGBUS:
            switch(code) {
                case BUS_ADRALN:
                    return "Invalid address alignment";
                case BUS_ADRERR:
                    return "Nonexistent physical address";
                case BUS_OBJERR:
                    return "Object-specific hardware error";
                default:
                    return "Bus error";
            }
            break;
        case SIGTRAP:
            switch(code) {
                case TRAP_BRKPT:
                    return "Process breakpoint";
                case TRAP_TRACE:
                    return "Process trace trap";
                default:
                    return "Trap";
            }
            break;
        case SIGCHLD:
            switch(code) {
                case CLD_EXITED:
                    return "Child has exited";
                case CLD_KILLED:
                    return "Child has terminated abnormally and did not create a core file";
                case CLD_DUMPED:
                    return "Child has terminated abnormally and created a core file";
                case CLD_TRAPPED:
                    return "Traced child has trapped";
                case CLD_STOPPED:
                    return "Child has stopped";
                case CLD_CONTINUED:
                    return "Stopped child has continued";
                default:
                    return "Child";
            }
            break;
        case SIGPOLL:
            switch(code) {
                case POLL_IN:
                    return "Data input available";
                case POLL_OUT:
                    return "Output buffers available";
                case POLL_MSG:
                    return "Input message available";
                case POLL_ERR:
                    return "I/O error";
                case POLL_PRI:
                    return "High priority input available";
                case POLL_HUP:
                    return "Device disconnected";
                default:
                    return "Pool";
            }
            break;
        case SIGABRT:
            return "Process abort signal";
        case SIGALRM:
            return "Alarm clock";
        case SIGCONT:
            return "Continue executing, if stopped";
        case SIGHUP:
            return "Hangup";
        case SIGINT:
            return "Terminal interrupt signal";
        case SIGKILL:
            return "Kill";
        case SIGPIPE:
            return "Write on a pipe with no one to read it";
        case SIGQUIT:
            return "Terminal quit signal";
        case SIGSTOP:
            return "Stop executing";
        case SIGTERM:
            return "Termination signal";
        case SIGTSTP:
            return "Terminal stop signal";
        case SIGTTIN:
            return "Background process attempting read";
        case SIGTTOU:
            return "Background process attempting write";
        case SIGUSR1:
            return "User-defined signal 1";
        case SIGUSR2:
            return "User-defined signal 2";
        case SIGPROF:
            return "Profiling timer expired";
        case SIGSYS:
            return "Bad system call";
        case SIGVTALRM:
            return "Virtual timer expired";
        case SIGURG:
            return "High bandwidth data is available at a socket";
        case SIGXCPU:
            return "CPU time limit exceeded";
        case SIGXFSZ:
            return "File size limit exceeded";
        default:
            switch(code) {
                case SI_USER:
                    return "Signal sent by kill()";
                case SI_QUEUE:
                    return "Signal sent by the sigqueue()";
                case SI_TIMER:
                    return "Signal generated by expiration of a timer set by timer_settime()";
                case SI_ASYNCIO:
                    return "Signal generated by completion of an asynchronous I/O request";
                case SI_MESGQ:
                    return
                            "Signal generated by arrival of a message on an empty message queue";
                default:
                    return "Unknown signal";
            }
            break;
    }


}

static ssize_t coffeecatch_unwind_signal(
                                         void** frames,
                                         size_t max_depth) {
    ALOGD("coffeecatch_unwind_signal dopen,thread:%d,%s",gettid(),getprogname());

    void *libunwind = dlopen("libunwind.so", RTLD_LAZY | RTLD_LOCAL);
    ALOGD("coffeecatch_unwind_signal dopen success");
    if (libunwind != NULL) {
        int (*backtrace)(void **buffer, int size) =
        (int (*)(void **buffer, int size))dlsym(libunwind, "unw_backtrace");
        if (backtrace != NULL) {
            ALOGD("before backtrace,frames.size:%s,max_depth:%d",frames,max_depth);
            int nb = backtrace(frames, max_depth);
            ALOGD("after backtrace,nb:%d",nb);
            if (nb > 0) {
            }
            return nb;
        } else {
            ALOGD("symbols not found in libunwind.so\n");
        }
        dlclose(libunwind);
    } else {
        ALOGD("libunwind.so could not be loaded,error:%s",dlerror());
    }
    return -1;
}



int coffeecatch_is_dll(const char *name) {
    size_t i;
    for(i = 0; name[i] != '\0'; i++) {
        if (name[i + 0] == '.' &&
            name[i + 1] == 's' &&
            name[i + 2] == 'o' &&
            ( name[i + 3] == '\0' || name[i + 3] == '.') ) {
            return 1;
        }
    }
    return 0;
}

/* Extract a line information on a PC address. */
void format_pc_address_cb(uintptr_t pc,
                          void (*fun)(void *arg, const char *module,
                                      uintptr_t addr,
                                      const char *function,
                                      uintptr_t offset), void *arg) {
    if (pc != 0) {
        Dl_info info;
        void * const addr = (void*) pc;
        /* dladdr() returns 0 on error, and nonzero on success. */
        if (dladdr(addr, &info) != 0 && info.dli_fname != NULL) {
            const uintptr_t near = (uintptr_t) info.dli_saddr;
            const uintptr_t offs = pc - near;
            const uintptr_t addr_rel = pc - (uintptr_t) info.dli_fbase;
            /* We need the absolute address for the main module (?).
               TODO FIXME to be investigated. */
            const uintptr_t addr_to_use = coffeecatch_is_dll(info.dli_fname)
                                          ? addr_rel : pc;
            //ALOGD("format_pc_address_cb 1 :%llx,fname:%s,sname:%s",pc,info.dli_fname,info.dli_sname);
            fun(arg, info.dli_fname, addr_to_use, info.dli_sname, offs);
        } else {
            //ALOGD("format_pc_address_cb 2");
            fun(arg, NULL, pc, NULL, 0);
        }
    }
}


typedef struct t_print_fun {
    char *buffer;
    size_t buffer_size;
} t_print_fun;

void print_fun(void *arg, const char *module, uintptr_t uaddr,
               const char *function, uintptr_t offset) {
    t_print_fun *const t = (t_print_fun*) arg;
    char *const buffer = t->buffer;
    const size_t buffer_size = t->buffer_size;

    const void*const addr = (void*) uaddr;
//     ALOGD("feifei_test print_fun buffer_size:%d,buffer orignal content:%s",buffer_size,buffer);
    int ret = 0;
    if (module == NULL) {
        ret = snprintf(buffer, buffer_size, "[at %p]%s", addr,LINESEPERATOR);
    } else if (function != NULL) {
        ret = snprintf(buffer, buffer_size, "[at %s:%p (%s+0x%x)]%s", module, addr,
                       function, (int) offset,LINESEPERATOR);
    } else {
        ret =snprintf(buffer, buffer_size, "[at %s:%p]%s", module, addr,LINESEPERATOR);
    }
//    ALOGD("feifei_test print_fun currentSize:%d,buffersize:%d,cotent:%s,result:%d",strlen(buffer),buffer_size,buffer,ret);
}

/* Format a line information on a PC address. */
void format_pc_address(char *buffer, size_t buffer_size, uintptr_t pc) {
    t_print_fun t;
    t.buffer = buffer;
    t.buffer_size = buffer_size;
    format_pc_address_cb(pc, print_fun, &t);
}

void printStack(int frames_size,void * frames[]){
    // 第七次就打印不出来了
    int stack_buffer_size = SIG_STACK_BUFFER_SIZE;
    char *buffer = (char *)malloc(stack_buffer_size);
    int buffer_offset = 0;
    int buffer_size = stack_buffer_size;
    int buffer_leftsize = buffer_size - buffer_offset;
    for(int i = 0 ; i < frames_size ; i++) {
        const uintptr_t pc = (uintptr_t)frames[i];
        format_pc_address(&buffer[buffer_offset], buffer_leftsize, pc);
        buffer_offset = strlen(buffer);
        buffer_leftsize = buffer_size - buffer_offset;
    }

    ALOGD("printStack:\n%s",buffer);


}
