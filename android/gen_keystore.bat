@echo off
REM 生成正式签名 keystore（首次执行一次即可）
REM 需要 JDK 的 keytool 在 PATH 中
setlocal
set KS=%~dp0sy.jks

rem 定位 keytool：优先 PATH，其次 JAVA_HOME，最后 Android Studio 内置 JDK
set KT=keytool.exe
if not "%JAVA_HOME%"=="" if exist "%JAVA_HOME%\bin\keytool.exe" set KT=%JAVA_HOME%\bin\keytool.exe
if not exist "%KT%" if exist "C:\Program Files\Android\openjdk\jdk-21.0.8\bin\keytool.exe" (
  set KT=C:\Program Files\Android\openjdk\jdk-21.0.8\bin\keytool.exe
)
echo keytool: %KT%

if exist "%KS%" (
  echo keystore 已存在: %KS%
  goto :writecfg
)

"%KT%" -genkeypair -v -keystore "%KS%" -keyalg RSA -keysize 2048 -validity 10000 -alias sy -storepass sy123456 -keypass sy123456 -dname "CN=sy, OU=dev, O=dsh, L=, S=, C=CN"
if errorlevel 1 goto :end

:writecfg
> "%~dp0keystore.properties" (
  echo storeFile=sy.jks
  echo storePassword=sy123456
  echo keyAlias=sy
  echo keyPassword=sy123456
)
echo 已写入 keystore.properties

:end
endlocal
