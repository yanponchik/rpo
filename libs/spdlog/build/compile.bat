set ANDROID_NDK=C:\Users\yan\AppData\Local\Android\Sdk\ndk\28.2.13676358
set TOOL_CHAIN=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
set CMAKE=C:\Users\yan\AppData\Local\Android\Sdk\cmake\3.22.1\bin\cmake.exe

for %%A in (armeabi-v7a x86 arm64-v8a x86_64) do (

    mkdir %%A
    cd %%A

    %CMAKE% ..\..\spdlog -GNinja ^
    -DCMAKE_SYSTEM_NAME=Android ^
    -DCMAKE_SYSTEM_VERSION=21 ^
    -DANDROID_ABI=%%A ^
    -DCMAKE_TOOLCHAIN_FILE=%TOOL_CHAIN% ^
    -DCMAKE_CXX_FLAGS=-DSPDLOG_COMPILED_LIB

    %CMAKE% --build .

    cd ..
)