set ANDROID_NDK=C:\Users\yan\AppData\Local\Android\Sdk\ndk\28.2.13676358
set TOOL_CHAIN=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
set CMAKE=C:\Users\yan\AppData\Local\Android\Sdk\cmake\3.22.1\bin\cmake.exe

REM Initialize mbedtls submodule (framework) if not yet fetched
cd /d "%~dp0..\mbedtls"
git submodule update --init
cd /d "%~dp0"

for %%A in (armeabi-v7a x86 arm64-v8a x86_64) do (

    mkdir %%A
    cd %%A

    %CMAKE% ..\..\mbedtls -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=%TOOL_CHAIN% ^
    -DANDROID_ABI=%%A ^
    -DANDROID_PLATFORM=android-21 ^
    -DENABLE_TESTING=OFF ^
    -DENABLE_PROGRAMS=OFF ^
    -DUSE_SHARED_MBEDTLS_LIBRARY=ON

    %CMAKE% --build .

    cd ..
)
pause