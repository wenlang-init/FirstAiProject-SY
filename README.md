# 兽音工具箱

一个跨平台小工具集合：**兽音加密、23 种哈希、文件编解码、Base64、进制转换**。
同一套 C 核心代码，同时提供 **Windows 桌面版（Qt）** 与 **Android 版（Kotlin + NDK）**。

## 下载

| 平台 | 文件 | 说明 |
| --- | --- | --- |
| Android | `兽音工具箱-v1.0.apk` | 约 5.8 MB，Android 8.0（API 26）及以上 |
| Windows | `兽音工具箱-v1.0.zip` | 约 31 MB，解压后直接运行 |

从 [Releases](https://github.com/wenlang-init/FirstAiProject-SY/releases) 页面获取安装包。

## 功能

### 兽音
把任意文本编码成 `嗷呜啊~` 风格的兽音，也能解码还原。支持 3 种编码模式与 4 字密令；
大文件走流式编解码，带进度条，可随时取消。

### HASH
23 种算法，文本与文件均可：

| 分类 | 算法 |
| --- | --- |
| 走 JNI（C 源码） | MD4、SHA3-224/256/384/512、Keccak-224/256/384/512、BLAKE2b-160/256/384/512、BLAKE2s-128/160/224/256 |
| 走系统 MessageDigest | MD5、SHA-1、SHA-224/256/384/512 |

结果直接写进输入框（便于整段复制），文件哈希在下方标签显示 **路径 + 耗时 + 吞吐速度**：

```
MD5 · MuMu 共享文件/Download/test.bin
耗时 66.00 ms · 121.21 MB/s
```

大文件计算过程中可点「停止计算」中断。

### 文件
- 编码 / 解码文件（兽音）
- 加密 / 解密文件（XOR）
- 文件叠加（两个文件顺序拼接）

输出位置由系统「另存为」对话框决定，全程带进度条与取消按钮。

### 代码
- Base64 编码 / 解码
- 生成代码 / 解码：把文本转成另一种编码形态，可指定 HEX 密钥或使用随机密钥

### 进制
2 ~ 36 进制任意互转，源进制与目标进制下拉选择。

## 目录结构

```
.
├─ main.cpp / src / CMakeLists.txt   Qt 桌面版源码
├─ android/                          Android 原生工程
│  ├─ app/src/main/cpp/
│  │  ├─ sy_bridge.cpp               JNI 桥接（9 个 native 方法）
│  │  ├─ core/                       autf8 / beastSound / edcrypt（与桌面版同一套）
│  │  └─ hash/                       syhash + 3rd(md4 / sha3 / blake2)
│  ├─ app/src/main/java/com/dsh/sy/  Kotlin 业务与 UI（13 个文件）
│  ├─ app/src/main/res/              Material3 配色 / 图标 / 布局
│  ├─ build_apk.bat                  一键打 Release 包
│  └─ gen_keystore.bat               生成正式签名
└─ release_out/                      发行产物（不入库，作为 GitHub Release 附件）
```

## 技术要点

- **核心代码共用**：`autf8.c` / `beastSound.c` / `edcrypt.c` 从桌面版原样移植到 Android，
  算法零改动，两端结果逐字节一致（已用标准测试向量校验，如 MD4("abc") = `a448017aaf21d8525fc10ae87aa6729d`）。
- **哈希双路径**：17 种少见算法走 JNI（沿用 Qt 同款 C 源码），6 种标准算法走 Java `MessageDigest`，
  后者由平台原生实现，更快也更稳。
- **Android 11+ 文件访问**：所有文件类 JNI 接口收 `fd`（`ParcelFileDescriptor`），不依赖真实文件路径。
- **兽音流式 API**：native 侧存在静态状态，JNI 层用 `std::mutex` 串行化，避免并发串数据。
- **进度与取消**：native 保存 `JavaVM*`，通过 `AttachCurrentThread` 回调 Kotlin 的 `ProgressCallback`，
  回调返回 `false` 即中断计算。
- **Material3**：DayNight 配色（跟随系统明暗自动切换）、自适应启动图标、
  `TextInputLayout` / `MaterialAutoCompleteTextView` / `MaterialButton` / `LinearProgressIndicator`。

## 编译

### Android

前置：JDK 17+、Android SDK（platform 35 / build-tools / NDK 27 / CMake 3.31）

```bat
cd android

rem 1) 告诉 Gradle SDK 在哪（写 local.properties 或设环境变量 ANDROID_HOME，二选一）
echo sdk.dir=你的SDK路径> local.properties

rem 2) 可选：生成正式签名。不配也没关系，会自动回落到 debug 签名
gen_keystore.bat

rem 3) 打包
gradlew.bat assembleRelease
```

产物：`android/app/build/outputs/apk/release/app-release.apk`

安装到手机：

```
adb install -r android/app/build/outputs/apk/release/app-release.apk
```

### Windows 桌面版

Qt 6.12 + MinGW 64-bit，用 Qt Creator 打开根目录的 `CMakeLists.txt`，构建 Release 即可。

## 说明

- `release_out/` 下的安装包不进仓库，只作为 GitHub Release 附件发布。
- 签名文件（`sy.jks`、`keystore.properties`）与本机 SDK 路径（`local.properties`）已在 `.gitignore` 中排除，不会入库。
- 仓库不含第三方哈希源码之外的依赖，构建仅需官方 Android SDK / Qt。
