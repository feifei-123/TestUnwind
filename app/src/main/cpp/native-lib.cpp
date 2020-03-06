#include <jni.h>
#include <string>
#include "coffeecatch.h"
#include "SignalHandle.h"
#include "mylog.h"


int getCrash2(){
    int i = 0;
    int j = 10/i;
}

void go2Crash3(){
    getCrash2();
}

void go2Crash4(){
    go2Crash3();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testunwind2_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_testunwind2_MainActivity_installSingal(JNIEnv *env, jobject instance) {

    set_signal_handler_4_posix();

}extern "C"
JNIEXPORT void JNICALL
Java_com_example_testunwind2_MainActivity_go2Crash(JNIEnv *env, jobject instance) {
    go2Crash4();
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_testunwind2_MainActivity_go2CrashCoffeeCatch(JNIEnv *env, jobject instance) {

    if (coffeecatch_inside() || \
      (coffeecatch_setup() == 0 \
       && sigsetjmp(*coffeecatch_get_ctx(), 1) == 0)){
        go2Crash4();
    }else{
        const char*const message = coffeecatch_get_message();
        ALOGD("feifei----- enter COFFEE_CATCH :%s",message);
    }coffeecatch_cleanup();


 COFFEE_TRY(){
     go2Crash4();
 }COFFEE_CATCH(){
        const char*const message = coffeecatch_get_message();
        ALOGD("feifei----- enter COFFEE_CATCH :%s",message);
 }COFFEE_END();

}

#define COFFEE_TRY()                                \
  if (coffeecatch_inside() || \
      (coffeecatch_setup() == 0 \
       && sigsetjmp(*coffeecatch_get_ctx(), 1) == 0))
#define COFFEE_CATCH() else
#define COFFEE_END() coffeecatch_cleanup()
/** End of internal functions & definitions. **/


extern "C"
JNIEXPORT void JNICALL
Java_com_example_testunwind2_MainActivity_installCoffeCatch(JNIEnv *env, jobject instance) {

   if(coffeecatch_inside() == false){
       int result = coffeecatch_setup();
       ALOGD("coffeecatch_setup was called,result:%d", result);
   }

}