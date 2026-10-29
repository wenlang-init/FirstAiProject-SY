package com.dsh.sy

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import com.google.android.material.textfield.MaterialAutoCompleteTextView

/** Tab5：2–36 进制转换 */
class RadixFragment : BaseFragment(R.layout.fragment_radix) {

    private lateinit var etInput: EditText
    private lateinit var etResult: EditText
    private lateinit var acFrom: MaterialAutoCompleteTextView
    private lateinit var acTo: MaterialAutoCompleteTextView

    override fun inputView(): EditText = etInput

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        etInput  = view.findViewById(R.id.etInput)
        etResult = view.findViewById(R.id.etResult)
        acFrom   = view.findViewById(R.id.spFrom)
        acTo     = view.findViewById(R.id.spTo)

        val items = (2..36).map { it.toString() }.toTypedArray()
        acFrom.setSimpleItems(items)
        acTo.setSimpleItems(items)
        acFrom.setText(items[8], false)   // 10
        acTo.setText(items[0], false)     // 2，与桌面版默认值一致

        view.findViewById<Button>(R.id.btnConvert).setOnClickListener {
            val from = acFrom.text.toString().toIntOrNull() ?: return@setOnClickListener
            val to   = acTo.text.toString().toIntOrNull() ?: return@setOnClickListener

            if (from == to) return@setOnClickListener

            val num = RadixRepo.parse(etInput.text.toString(), from)

            if (num == null) {
                etResult.setText("输入错误")
                return@setOnClickListener
            }
            etResult.setText(RadixRepo.format(num, to))
        }
    }
}
