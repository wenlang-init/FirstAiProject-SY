package com.dsh.sy

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.bottomnavigation.BottomNavigationView

class MainActivity : AppCompatActivity() {

    private lateinit var viewPager: ViewPager2
    private lateinit var bottomNav: BottomNavigationView
    private lateinit var pagerAdapter: PagerAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // 主题为 NoActionBar，这里手动挂上 M3 顶栏（承载 overflow 菜单）
        setSupportActionBar(findViewById(R.id.toolbar))

        viewPager    = findViewById(R.id.viewPager)
        bottomNav    = findViewById(R.id.bottomNav)
        pagerAdapter = PagerAdapter(this)

        viewPager.adapter = pagerAdapter

        bottomNav.setOnItemSelectedListener { item ->
            viewPager.currentItem = PagerAdapter.indexOf(item.itemId)
            true
        }

        viewPager.registerOnPageChangeCallback(object : ViewPager2.OnPageChangeCallback() {
            override fun onPageSelected(position: Int) {
                bottomNav.menu.getItem(position).isChecked = true
            }
        })
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        val et: EditText = pagerAdapter.at(viewPager.currentItem).inputView() ?: run {
            Toast.makeText(this, "当前页没有输入框", Toast.LENGTH_SHORT).show()
            return true
        }

        when (item.itemId) {
            R.id.action_copy -> {
                val cm = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                cm.setPrimaryClip(ClipData.newPlainText("sy", et.text.toString()))
                Toast.makeText(this, "已拷贝", Toast.LENGTH_SHORT).show()
            }

            R.id.action_paste -> {
                val cm = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                val text = cm.primaryClip?.getItemAt(0)?.text?.toString()
                if (text != null) et.setText(text)
            }

            R.id.action_clear -> et.setText("")
        }
        return true
    }

    private class PagerAdapter(activity: AppCompatActivity) : FragmentStateAdapter(activity) {

        private val fragments = listOf(
            BeastFragment(),
            HashFragment(),
            FileFragment(),
            CodeFragment(),
            RadixFragment()
        )

        override fun getItemCount(): Int = fragments.size

        override fun createFragment(position: Int): androidx.fragment.app.Fragment = fragments[position]

        fun at(position: Int): BaseFragment = fragments[position]

        companion object {
            fun indexOf(itemId: Int): Int = when (itemId) {
                R.id.nav_hash  -> 1
                R.id.nav_file  -> 2
                R.id.nav_code  -> 3
                R.id.nav_radix -> 4
                else           -> 0
            }
        }
    }
}
