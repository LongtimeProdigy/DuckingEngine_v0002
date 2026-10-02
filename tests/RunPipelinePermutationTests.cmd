@echo off
setlocal
rem Run from a Visual Studio x64 Native Tools command prompt.
pushd "%~dp0.."
if not exist x64\PermutationTests mkdir x64\PermutationTests
cl /nologo /EHsc /std:c++17 /I. tests\PipelinePermutationTests.cpp lib\tinyXML\tinyxml.cpp lib\tinyXML\tinyxmlparser.cpp lib\tinyXML\tinyxmlerror.cpp lib\tinyXML\tinystr.cpp /Fo:x64\PermutationTests\ /Fe:x64\PermutationTests\PipelinePermutationTests.exe
if errorlevel 1 (popd & exit /b 1)
x64\PermutationTests\PipelinePermutationTests.exe
set result=%errorlevel%
popd
exit /b %result%
