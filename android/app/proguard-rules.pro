# 保留 JNI 入口
-keep class com.dsh.sy.NativeSy { *; }
# JNI 进度回调接口及其实现类
-keep interface com.dsh.sy.ProgressCb { *; }
-keep class com.dsh.sy.ProgressCallback { *; }
