@echo off
REM 打包 Release APK
REM 前置条件（本机均已就绪）：
REM   1) Android SDK  : D:\software\Android\Qt\commandlinetools-win-9123335_latest\SDK  (local.properties)
REM   2) NDK 27.2     : 已在 app/build.gradle.kts 中 ndkVersion 固定
REM   3) Gradle 8.9   : 优先用本机已解压的 D:\software\Android\Qt\gradle\gradle-8.9
REM   4) 签名         : 已执行 gen_keystore.bat（sy.jks + keystore.properties）
setlocal
cd /d "%~dp0"

REM 1) JDK：已设置 JAVA_HOME 就直接用，否则尝试常见安装位置
if "%JAVA_HOME%"=="" (
  if exist "C:\Program Files\Android\openjdk\jdk-21.0.8" (
    set "JAVA_HOME=C:\Program Files\Android\openjdk\jdk-21.0.8"
  ) else if exist "C:\Program Files\Android\Android Studio\jbr" (
    set "JAVA_HOME=C:\Program Files\Android\Android Studio\jbr"
  )
)
echo JAVA_HOME=%JAVA_HOME%

REM 2) SDK：local.properties 缺失时提示（AGP 也可读 ANDROID_HOME）
if not exist "%~dp0local.properties" (
  if "%ANDROID_HOME%"=="" (
    echo [提示] 缺少 local.properties 且未设置 ANDROID_HOME
    echo        请创建 local.properties：sdk.dir=你的SDK路径
  )
)

REM 3) Gradle：优先本机已解压的 Gradle（离线快），否则用仓库自带的 wrapper
set "GRADLE_EXE=D:\software\Android\Qt\gradle\gradle-8.9\bin\gradle.bat"

if exist "%GRADLE_EXE%" (
  call "%GRADLE_EXE%" --console=plain assembleRelease
) else (
  echo 未找到本机 Gradle，改用 ./gradlew（首次会自动下载分发包）
  call gradlew.bat --console=plain assembleRelease
)

:done
if errorlevel 1 (
  echo 构建失败
  goto :end
)

echo.
echo ==== 产物 ====
dir /b "%~dp0app\build\outputs\apk\release\*.apk"
echo.
echo 安装到手机：
echo   adb install -r "%~dp0app\build\outputs\apk\release\app-release.apk"

:end
endlocal
