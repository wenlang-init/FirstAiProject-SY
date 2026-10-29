package com.dsh.sy

import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.ProgressBar
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Tab3：文件编码/解码/加密/解密/叠加 */
class FileFragment : BaseFragment(R.layout.fragment_file) {

    private lateinit var etFile1: EditText
    private lateinit var etFile2: EditText
    private lateinit var etKey: EditText
    private lateinit var progress: ProgressBar
    private lateinit var tvInfo: TextView

    private val mainHandler = Handler(Looper.getMainLooper())
    private var job: Job? = null

    @Volatile
    private var cancelled = false

    private var uri1: Uri? = null
    private var uri2: Uri? = null

    /** 选择源文件 */
    private var pendingPick: ((Uri) -> Unit)? = null
    private val openDoc = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        uri?.let { pendingPick?.invoke(it) }
    }

    /** 选择输出文件 */
    private var pendingOutput: ((Uri) -> Unit)? = null
    private val createDoc = registerForActivityResult(ActivityResultContracts.CreateDocument("*/*")) { uri ->
        uri?.let { pendingOutput?.invoke(it) }
    }

    override fun inputView(): EditText? = null

    private fun setProgress(p: Int) {
        mainHandler.post { progress.progress = p }
    }

    private fun setInfo(msg: String) {
        mainHandler.post { tvInfo.text = msg }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        etFile1  = view.findViewById(R.id.etFile1)
        etFile2  = view.findViewById(R.id.etFile2)
        etKey    = view.findViewById(R.id.etKey)
        progress = view.findViewById(R.id.progress)
        tvInfo   = view.findViewById(R.id.tvInfo)

        view.findViewById<Button>(R.id.btnPick1).setOnClickListener {
            pendingPick = { u ->
                uri1 = u
                etFile1.setText(requireContext().contentResolver.nameOf(u))
            }
            openDoc.launch(arrayOf("*/*"))
        }

        view.findViewById<Button>(R.id.btnPick2).setOnClickListener {
            pendingPick = { u ->
                uri2 = u
                etFile2.setText(requireContext().contentResolver.nameOf(u))
            }
            openDoc.launch(arrayOf("*/*"))
        }

        view.findViewById<Button>(R.id.btnEncodeFile).setOnClickListener {
            val src = uri1

            if (src == null) {
                toast("请先选择文件1")
                return@setOnClickListener
            }
            val key = etKey.text.toString()
            val err = BeastRepo.checkKey(key)

            if (err != null) {
                setInfo("编码失败.$err")
                return@setOnClickListener
            }
            requestOutput("encoded.txt") { out -> runBeast(src, out, true, key) }
        }

        view.findViewById<Button>(R.id.btnDecodeFile).setOnClickListener {
            val src = uri1

            if (src == null) {
                toast("请先选择文件1")
                return@setOnClickListener
            }
            requestOutput("decoded.bin") { out -> runBeast(src, out, false, "") }
        }

        view.findViewById<Button>(R.id.btnEncryptFile).setOnClickListener {
            val src = uri1

            if (src == null) {
                toast("请先选择文件1")
                return@setOnClickListener
            }
            requestOutput("encrypted.bin") { out -> runXor(src, out, true) }
        }

        view.findViewById<Button>(R.id.btnDecryptFile).setOnClickListener {
            val src = uri1

            if (src == null) {
                toast("请先选择文件1")
                return@setOnClickListener
            }
            requestOutput("decrypted.bin") { out -> runXor(src, out, false) }
        }

        view.findViewById<Button>(R.id.btnMergeFile).setOnClickListener {
            val a = uri1
            val b = uri2

            if ((a == null) || (b == null)) {
                toast("请先选择文件1和文件2")
                return@setOnClickListener
            }
            requestOutput("merged.bin") { out -> runMerge(a, b, out) }
        }

        view.findViewById<Button>(R.id.btnCancel).setOnClickListener {
            cancelled = true
            NativeSy.nativeCancel()
            job?.cancel()
            setInfo("已取消")
        }
    }

    private fun requestOutput(suggestName: String, block: (Uri) -> Unit) {
        pendingOutput = block
        createDoc.launch(suggestName)
    }

    /** block 返回 (提示文案, 实际处理字节数)，这里统一补上耗时与吞吐 */
    private fun launchTask(block: suspend () -> Pair<String, Long>) {
        job?.cancel()
        cancelled = false
        progress.progress = 0

        job = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.Default) {
            val t0 = System.nanoTime()
            val (msg, bytes) = block()
            val ms = (System.nanoTime() - t0) / 1_000_000L

            withContext(Dispatchers.Main) { tvInfo.text = "$msg\n${formatCost(ms, bytes)}" }
        }
    }

    private fun callback(): ProgressCallback = ProgressCallback { pct ->
        setProgress(pct)
        !cancelled
    }

    private fun runBeast(src: Uri, out: Uri, encode: Boolean, key: String) {
        launchTask {
            val cr   = requireContext().contentResolver
            val size = maxOf(0L, cr.sizeOf(src))
            val ok   = cr.withWriteFd(out) { outFd ->
                cr.withReadFd(src) { inFd ->
                    if (encode) NativeSy.beastFileEncode(inFd, outFd, key, callback())
                    else NativeSy.beastFileDecode(inFd, outFd, callback())
                }
            }
            val name = cr.pathOf(out)

            if (ok) ("${if (encode) "编码" else "解码"}成功：$name" to size)
            else ("${if (encode) "编码" else "解码"}失败" to 0L)
        }
    }

    private fun runXor(src: Uri, out: Uri, encrypt: Boolean) {
        launchTask {
            val cr   = requireContext().contentResolver
            val size = maxOf(0L, cr.sizeOf(src))
            val ok   = cr.withWriteFd(out) { outFd ->
                cr.withReadFd(src) { inFd ->
                    NativeSy.xorFile(inFd, outFd, encrypt, callback())
                }
            }
            val name = cr.pathOf(out)

            if (ok) ("${if (encrypt) "加密" else "解密"}成功：$name" to size)
            else ("${if (encrypt) "加密" else "解密"}失败" to 0L)
        }
    }

    /** 文件叠加：文件1 + 文件2 -> 输出文件（纯 Kotlin，无需 JNI） */
    private fun runMerge(a: Uri, b: Uri, out: Uri) {
        launchTask {
            val cr = requireContext().contentResolver
            val buf = ByteArray(1024 * 64)
            var done = 0L

            val total = listOf(a, b).sumOf { maxOf(0L, cr.sizeOf(it)) }

            cr.openOutputStream(out)!!.use { os ->
                for (u in listOf(a, b)) {
                    cr.openInputStream(u)!!.use { ins ->
                        while (true) {
                            if (cancelled) return@launchTask ("已取消生成" to done)
                            val n = ins.read(buf)
                            if (n <= 0) break
                            os.write(buf, 0, n)
                            done += n
                            if (total > 0) setProgress((done * 100 / total).toInt())
                        }
                    }
                }
            }
            "生成成功：${cr.pathOf(a)} + ${cr.pathOf(b)} -> ${cr.pathOf(out)}" to done
        }
    }

    override fun onDestroyView() {
        cancelled = true
        NativeSy.nativeCancel()
        job?.cancel()
        super.onDestroyView()
    }
}
