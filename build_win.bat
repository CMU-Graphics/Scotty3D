@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" > NUL 2> NUL
mkdir build
pushd build
cmake ..
if "%1"=="configure" exit
if [%1]==[] (cmake --build . --config RelWithDebInfo)
if not [%1]==[] (cmake --build . --config %1)
popd
