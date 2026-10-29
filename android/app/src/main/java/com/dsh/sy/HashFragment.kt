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
import com.google.android.material.textfield.MaterialAutoCompleteTextView
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** Tab2：哈希（23 种算法） */
class HashFragment : BaseFragment(R.layout.fragment_hash) {

    private lateinit var etInput: EditText
    private lateinit var acAlg: MaterialAutoCompleteTextView
    private lateinit var progress: ProgressBar
    private lateinit var tvInfo: TextView

    private val mainHandler = Handler(Looper.getMainLooper())
    private var job: Job? = null

    @Volatile
    private var cancelled = false

    private val openDoc = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri != null) startFileHash(uri)
    }

    override fun inputView(): EditText = etInput

    private fun setProgress(p: Int) {
        mainHandler.post { progress.progress = p }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        etInput  = view.findViewById(R.id.etInput)
        acAlg    = view.findViewById(R.id.spAlg)
        progress = view.findViewById(R.id.progress)
        tvInfo   = view.findViewById(R.id.tvInfo)

        // M3 下拉菜单：setSimpleItems 内部使用非过滤的 ArrayAdapter
        acAlg.setSimpleItems(HashAlg.labels.toTypedArray())
        acAlg.setText(HashAlg.labels[1], false)   // 默认 MD5，与桌面版一致

        /** 当前选中的算法；文本异常时回落到 MD5 */
        fun currentAlg(): HashAlg {
            val i = HashAlg.labels.indexOf(acAlg.text.toString())
            return HashAlg.of(if (i < 0) 1 else i)
        }

        view.findViewById<Button>(R.id.btnHashInput).setOnClickListener {
            val alg  = currentAlg()
            val data = etInput.text.toString().toByteArray(Charsets.UTF_8)

            val (hex, ms) = timed { HashRepo.ofText(data, alg) }

            // 结果框只放哈希值，方便直接复制；文件名/耗时/速度走 tvInfo
            etInput.setText(hex)
            tvInfo.text = "${alg.label} · ${data.size} B · ${formatCost(ms, data.size.toLong())}"
        }

        view.findViewById<Button>(R.id.btnHashFile).setOnClickListener {
            openDoc.launch(arrayOf("*/*"))
        }

        view.findViewById<Button>(R.id.btnHashStop).setOnClickListener {
            cancelled = true
            NativeSy.nativeCancel()
            job?.cancel()
        }
    }

    private fun startFileHash(uri: Uri) {
        val i   = HashAlg.labels.indexOf(acAlg.text.toString())
        val alg = HashAlg.of(if (i < 0) 1 else i)
        val cr  = requireContext().contentResolver

        job?.cancel()
        cancelled = false
        progress.progress = 0
        progress.visibility = View.VISIBLE

        job = viewLifecycleOwner.lifecycleScope.launch(Dispatchers.Default) {
            val size = maxOf(0L, cr.sizeOf(uri))

            val (result, ms) = timed {
                if (alg.native) {
                    cr.withReadFd(uri) { fd ->
                        NativeSy.hashFd(fd, alg.index, ProgressCallback { pct ->
                            setProgress(pct)
                            !cancelled
                        })
                    }
                } else {
                    HashRepo.ofStream(
                        { cr.openInputStream(uri)!! },
                        cr.sizeOf(uri),
                        alg
                    ) { pct ->
                        setProgress(pct)
                        !cancelled
                    }
                }
            }

            withContext(Dispatchers.Main) {
                progress.visibility = View.GONE

                if (result == null) {
                    tvInfo.text = "已取消计算 · ${formatCost(ms, size)}"
                } else {
                    // 只显示哈希结果，文件名（含路径）放到 tvInfo
                    etInput.setText(result)
                    tvInfo.text = "${alg.label} · ${cr.pathOf(uri)}\n${formatCost(ms, size)}"
                }
            }
        }
    }

    override fun onDestroyView() {
        cancelled = true
        NativeSy.nativeCancel()
        job?.cancel()
        super.onDestroyView()
    }
}
