package com.dsh.sy

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.RadioGroup
import android.widget.TextView
import androidx.annotation.LayoutRes
import androidx.fragment.app.Fragment

abstract class BaseFragment(@LayoutRes contentLayoutId: Int) : Fragment(contentLayoutId) {

    /** 当前页的主输入/输出框，供 Activity 的菜单（拷贝/粘贴/清空）操作 */
    abstract fun inputView(): EditText?

    protected fun toast(msg: String) {
        view?.post {
            android.widget.Toast.makeText(requireContext(), msg, android.widget.Toast.LENGTH_SHORT).show()
        }
    }
}

/** Tab1：兽音文本编解码 */
class BeastFragment : BaseFragment(R.layout.fragment_beast) {

    private lateinit var etInput: EditText
    private lateinit var etKey: EditText
    private lateinit var tvInfo: TextView
    private lateinit var rgMode: RadioGroup

    override fun inputView(): EditText = etInput

    private fun currentMode(): Int = when (rgMode.checkedRadioButtonId) {
        R.id.rbMode2 -> 2
        R.id.rbMode3 -> 3
        else         -> 1
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        etInput = view.findViewById(R.id.etInput)
        etKey   = view.findViewById(R.id.etKey)
        tvInfo  = view.findViewById(R.id.tvInfo)
        rgMode  = view.findViewById(R.id.rgMode)

        view.findViewById<Button>(R.id.btnEncode).setOnClickListener {
            tvInfo.text = ""

            val key = etKey.text.toString()
            val err = BeastRepo.checkKey(key)

            if (err != null) {
                tvInfo.text = err
                return@setOnClickListener
            }

            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            val out = BeastRepo.encode(text, currentMode(), key)

            if (out == null) {
                tvInfo.text = if (currentMode() == 1)
                    "编码错误:字符串或词典字不能包含unicode大于16bit的字"
                else
                    "编码错误"
            } else {
                etInput.setText(String(out, Charsets.UTF_8))
            }
        }

        view.findViewById<Button>(R.id.btnDecode).setOnClickListener {
            tvInfo.text = ""

            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            val out = BeastRepo.decode(text, currentMode())

            if (out == null) tvInfo.text = "解码错误"
            else etInput.setText(String(out, Charsets.UTF_8))
        }
    }
}
