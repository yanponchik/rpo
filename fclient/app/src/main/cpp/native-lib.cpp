#include <jni.h>
#include <string>
#include <android/log.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/des.h>

mbedtls_entropy_context entropy; //контекст источника энтропии для ГСПЧ
mbedtls_ctr_drbg_context ctr_drbg; //контекстГПСЧ
char *personalization = "fclient-sample-app";

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "fclient_ndk", __VA_ARGS__)
// макрос для вывода в logcat с тегом fclient_ndk
#define SLOG_INFO(...) android_logger->info( __VA_ARGS__ )
// макрос через spdlog
auto android_logger = spdlog::android_logger_mt("android", "fclient_ndk");
// глобальный логгер spdlog тег fclient_ndk
extern "C" JNIEXPORT jstring JNICALL // Объявление JNI-функции. extern "C" — имя без искажения. JNIEXPORT/JNICALL — макросы для экспорта. Имя жёстко: Java_<пакет>_<Класс>_<метод>. env — окружение JNI; второй аргумент — объект MainActivity (не используется).
Java_ru_iu3_fclient_MainActivity_stringFromJNI(
        JNIEnv* env, //окружение
        jobject /* this */ ) { //объект MainActivity
    std::string hello = "Hello from C++";
    LOG_INFO("Hello from c++ %d", 2026);
    SLOG_INFO("Hello from spdlog {0}", 2026);
    return env->NewStringUTF(hello.c_str()); //создание Java строки из С
}

extern "C" JNIEXPORT void JNICALL
Java_ru_iu3_fclient_MainActivity_logInfo(
        JNIEnv* env,
        jobject /* this */,
        jstring message) { // строка из джава которую нужно залогировать

    const char* msg = env->GetStringUTFChars(message, nullptr);
    //получение указателя на utf8 байты java-строки. ВТорой параметр -указатель на флаг копирования (не нужен т к ullptr)
    // Лог уровня INFO
    LOG_INFO("%s", msg);

    env->ReleaseStringUTFChars(message, msg); // свобождение ресурсов
}



extern "C" JNIEXPORT jint JNICALL
Java_ru_iu3_fclient_MainActivity_initRng(JNIEnv *env, jclass clazz) {
    mbedtls_entropy_init( &entropy ); // инициализация контекстов энтропии и ГПСЧ.
    mbedtls_ctr_drbg_init( &ctr_drbg );

    return mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy,
                                  (const unsigned char *) personalization,
                                  strlen( personalization ) );
} // Заполнение ГПСЧ энтропией и строкой персонализации. Возврат кода ошибки (0 = успех); это значение возвращается в Java как int res.
// Персонализация — не секретная произвольная строка (у тебя имя приложения).

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_randomBytes(JNIEnv *env, jclass, jint no) { //передаем еще кол-во байт
    uint8_t * buf = new uint8_t [no];
    mbedtls_ctr_drbg_random(&ctr_drbg, buf, no); //заполенние buf случайными байтами
    jbyteArray rnd = env->NewByteArray(no); //создание Java-массива byte[] длины no
    env->SetByteArrayRegion(rnd, 0, no, (jbyte *)buf); // копирование данных из buf в Java-массив
    delete[] buf; //очищаем
    return rnd;
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_encrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key); //получение длины
    jsize dsz = env->GetArrayLength(data); //получение длины
    if ((ksz != 16) || (dsz <= 0)) { // проверка размера
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx; //контекст 3des
    mbedtls_des3_init(&ctx); //инициализация

    jbyte * pkey = env->GetByteArrayElements(key, 0);
    //получение указателя на элементы массива keys

    // Паддинг PKCS#5
    int rst = dsz % 8;
    int sz = dsz + 8 - rst;
    uint8_t * buf = new uint8_t[sz];
    for (int i = 7; i > rst; i--)
        buf[dsz + i] = rst;
    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf); // копирование данных в буфер перед шифрованием
    mbedtls_des3_set2key_enc(&ctx, (uint8_t *)pkey); // установка ключа шифрования (2 ключа по 8 байт)
    int cn = sz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf + i*8);
    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf); //копирование в Java масиив
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0); //освобождение массивов
    return dout;
}
extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_decrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key);
    jsize dsz = env->GetArrayLength(data);
    if ((ksz != 16) || (dsz <= 0) || ((dsz % 8) != 0)) {
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte * pkey = env->GetByteArrayElements(key, 0);

    uint8_t * buf = new uint8_t[dsz];

    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf);
    mbedtls_des3_set2key_dec(&ctx, (uint8_t *)pkey);
    int cn = dsz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf +i*8);

    //PKCS#5. упрощено. по соображениям безопасности надо проверить каждый байт паддинга
    int sz = dsz - 8 + buf[dsz-1];

    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf);
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0);
    return dout;
}

JavaVM* gJvm = nullptr; // глобальный указатель на вирт. машину Java.
// нужен, чтобы в своём потоке получить JNIEnv*
// вызывается при загрузке нативной библиотеки. Сохраняем pjvm в gJvm, возвращаем версию JNI.
JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM* pjvm, void* reserved)
{
    gJvm = pjvm;
    return JNI_VERSION_1_6;
}
//получение JNIEnv* для текущего потока
JNIEnv* getEnv (bool& detach)
{
    JNIEnv* env = nullptr;
    int status = gJvm->GetEnv ((void**)&env, JNI_VERSION_1_6);
    //получение JNIEnv* для текущего потока.
    detach = false;
    if (status == JNI_EDETACHED)
    {
        status = gJvm->AttachCurrentThread (&env, NULL);
        //привязка нативного потока к JVM (для потоков, созданных в C++).
        if (status < 0)
        {
            return nullptr;
        }
        detach = true;
    }
    return env;
}
//Если поток был привязан в getEnv, отвязывает его через DetachCurrentThread.
void releaseEnv (bool detach, JNIEnv* env)
{
    if (detach && (gJvm != nullptr))
    {
        gJvm->DetachCurrentThread ();
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_ru_iu3_fclient_MainActivity_transaction(JNIEnv *xenv, jobject xthiz, jbyteArray xtrd) {
    jobject thiz  = xenv->NewGlobalRef(xthiz);
    jbyteArray trd  = (jbyteArray)xenv->NewGlobalRef(xtrd);
    //глобальные ссылки, чтобы объекты не собирались GC, пока с ними работает другой поток.
    std::thread t([thiz, trd] { //запуск потока
        bool detach = false;
        JNIEnv *env = getEnv(detach); //получаем env
        if (!env) {
            return;
        }
        jclass cls = env->GetObjectClass(thiz); //получаем класс mainactivity
        jmethodID id = env->GetMethodID( //получаем метод enterpin
                cls, "enterPin", "(ILjava/lang/String;)Ljava/lang/String;");

        //TRD 9F0206000000000100 = amount = 1р
        uint8_t *p = (uint8_t *) env->GetByteArrayElements(trd, 0);
        jsize sz = env->GetArrayLength(trd);
        //проверка формата trd иначе все очищаем и заканчиваем
        if ((sz != 9) || (p[0] != 0x9F) || (p[1] != 0x02) || (p[2] != 0x06)) {
            env->ReleaseByteArrayElements(trd, (jbyte *) p, 0);
            env->DeleteGlobalRef(thiz);
            env->DeleteGlobalRef(trd);
            releaseEnv(detach, env);
            return;
        }
        char buf[13];
        for (int i = 0; i < 6; i++) {
            uint8_t n = *(p + 3 + i);
            buf[i * 2] = ((n & 0xF0) >> 4) + '0';
            buf[i * 2 + 1] = (n & 0x0F) + '0';
        }
        buf[12] = 0x00; // превращаем в строку из 12 символов (каждый байт - 2 цифры)
        jstring jamount = (jstring) env->NewStringUTF(buf); //строка суммы для передачи в enterpin
        int ptc = 3;
        while (ptc > 0) {

            jstring pin = (jstring) env->CallObjectMethod(thiz, id, ptc, jamount);
            //вызов Вызов mainActivity.enterPin(ptc, amount) — открывается Pinpad, поток ждёт в wait().
            if (pin) { // проверка pin
                const char *utf = env->GetStringUTFChars(pin, nullptr);
                if ((utf != nullptr) && (strcmp(utf, "1234") == 0)) {
                    env->ReleaseStringUTFChars(pin, utf);
                    break;
                }
                if (utf) env->ReleaseStringUTFChars(pin, utf);
            }
            ptc--;
        }

        id = env->GetMethodID(cls, "transactionResult", "(Z)V");
        //получаю id метода из класса
        if (id) env->CallVoidMethod(thiz, id, ptc > 0);
        // вызываю метод из java и если остались попытки значит пин совпал и выводим OK
        env->ReleaseByteArrayElements(trd, (jbyte *) p, 0);
        env->DeleteGlobalRef(thiz);
        env->DeleteGlobalRef(trd);
        releaseEnv(detach, env); //освобождение JNI и отвязка потока
    });
    t.detach(); // Поток продолжает работу независимо
    //.detach() значит только одно: поток T1(java) больше не будет ждать T2 через join().
    return true; // cразу возврат в Java, заканчиваю поток в java(транзакция идёт в фоне).
}
