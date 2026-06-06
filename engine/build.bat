@echo off
setlocal

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set ENGINE_DIR=%PROJECT_ROOT%\engine

set TARGET=%1
if "%TARGET%"=="" set TARGET=engine

if "%MYSQL_C_API_INCLUDE%"=="" (
  if exist "D:\ampps\Ampps\mysql\include\mysql.h" (
    set MYSQL_C_API_INCLUDE=D:\ampps\Ampps\mysql\include
  )
)

if "%MYSQL_C_API_LIB%"=="" (
  if exist "D:\ampps\Ampps\mysql\lib\libmysql.lib" (
    set MYSQL_C_API_LIB=D:\ampps\Ampps\mysql\lib
  )
)

if "%MYSQL_C_API_INCLUDE%"=="" (
  echo MYSQL_C_API_INCLUDE is not set.
  echo Example: set MYSQL_C_API_INCLUDE=D:\ampps\Ampps\mysql\include
  exit /b 1
)

if "%MYSQL_C_API_LIB%"=="" (
  echo MYSQL_C_API_LIB is not set.
  echo Example: set MYSQL_C_API_LIB=D:\ampps\Ampps\mysql\lib
  exit /b 1
)

if not exist "%ENGINE_DIR%\bin" mkdir "%ENGINE_DIR%\bin"

set COMMON_FLAGS=-std=c++17 -Wall -Wextra -I"%ENGINE_DIR%\include" -I"%MYSQL_C_API_INCLUDE%"
set SOURCES="%ENGINE_DIR%\src\ConfigLoader.cpp" "%ENGINE_DIR%\src\Graph.cpp" "%ENGINE_DIR%\src\MySQLManager.cpp" "%ENGINE_DIR%\src\PathFinder.cpp"
set LIBS="%MYSQL_C_API_LIB%\libmysql.lib"
set MYSQL_RUNTIME_DLL=%MYSQL_C_API_LIB%\libmysql.dll

if "%TARGET%"=="engine" (
  g++ %COMMON_FLAGS% "%ENGINE_DIR%\src\main.cpp" %SOURCES% -o "%ENGINE_DIR%\bin\l_engine.exe" %LIBS%
  if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
  if exist "%MYSQL_RUNTIME_DLL%" copy /Y "%MYSQL_RUNTIME_DLL%" "%ENGINE_DIR%\bin\libmysql.dll" >nul
  exit /b %ERRORLEVEL%
)

if "%TARGET%"=="test" (
  g++ %COMMON_FLAGS% "%ENGINE_DIR%\src\test_connection.cpp" %SOURCES% -o "%ENGINE_DIR%\bin\test_connection.exe" %LIBS%
  if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
  if exist "%MYSQL_RUNTIME_DLL%" copy /Y "%MYSQL_RUNTIME_DLL%" "%ENGINE_DIR%\bin\libmysql.dll" >nul
  exit /b %ERRORLEVEL%
)

echo Unknown target: %TARGET%
exit /b 1
