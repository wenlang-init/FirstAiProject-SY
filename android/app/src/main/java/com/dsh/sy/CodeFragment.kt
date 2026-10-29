package com.dsh.sy

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.TextView
import java.util.Base64

/** Tab4：Base64 + 生成代码/解码 */
class CodeFragment : BaseFragment(R.layout.fragment_code) {

    private lateinit var etInput: EditText
    private lateinit var etKeyHex: EditText
    private lateinit var cbRandom: CheckBox
    private lateinit var tvInfo: TextView

    override fun inputView(): EditText = etInput

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        etInput  = view.findViewById(R.id.etInput)
        etKeyHex = view.findViewById(R.id.etKeyHex)
        cbRandom = view.findViewById(R.id.cbRandom)
        tvInfo   = view.findViewById(R.id.tvInfo)

        cbRandom.setOnCheckedChangeListener { _, checked ->
            etKeyHex.isEnabled = !checked
        }
        etKeyHex.isEnabled = !cbRandom.isChecked

        view.findViewById<Button>(R.id.btnB64Encode).setOnClickListener {
            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            etInput.setText(Base64.getEncoder().encodeToString(text.toByteArray(Charsets.UTF_8)))
        }

        view.findViewById<Button>(R.id.btnB64Decode).setOnClickListener {
            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            try {
                val data = Base64.getDecoder().decode(text.trim())
                etInput.setText(String(data, Charsets.UTF_8))
            } catch (e: IllegalArgumentException) {
                tvInfo.text = "Base64 解码失败"
            }
        }

        view.findViewById<Button>(R.id.btnGenCode).setOnClickListener {
            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            val key = etKeyHex.text.toString()
                .removePrefix("0x")
                .removePrefix("0X")
                .toIntOrNull(16) ?: 0x0C

            etInput.setText(UseCodeRepo.getCode(text, cbRandom.isChecked, key))
        }

        view.findViewById<Button>(R.id.btnDecodeCode).setOnClickListener {
            val text = etInput.text.toString()

            if (text.isEmpty()) return@setOnClickListener

            val out = UseCodeRepo.getDecode(text)

            if (out == null) tvInfo.text = "解码失败"
            else etInput.setText(out)
        }
    }
}
