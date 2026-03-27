package com.example.smarthomeapp

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class BlindsActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_blinds)

        val blindsView = findViewById<View>(R.id.blindsView)
        val sbBlinds = findViewById<SeekBar>(R.id.sbBlinds)
        val tvStatus = findViewById<TextView>(R.id.tvStatus)
        val btnBack = findViewById<ImageView>(R.id.btnBack)

        btnBack.setOnClickListener { finish() }

        // Функция для обновления высоты шторы
        fun updateBlinds(progress: Int) {
            tvStatus.text = "Closed: $progress%"
            val params = blindsView.layoutParams
            // Высота окна у нас 250dp. Переводим проценты в высоту.
            // Для простоты теста используем множитель (в реале нужны px/dp конвертации)
            val density = resources.displayMetrics.density
            params.height = (progress * 2.5 * density).toInt()
            blindsView.layoutParams = params
        }

        sbBlinds.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) { updateBlinds(p) }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {}
        })

        // Кнопки режимов
        findViewById<Button>(R.id.btnOpen).setOnClickListener { sbBlinds.progress = 0 }
        findViewById<Button>(R.id.btnClosed).setOnClickListener { sbBlinds.progress = 100 }
        findViewById<Button>(R.id.btn30).setOnClickListener { sbBlinds.progress = 30 }
        findViewById<Button>(R.id.btn60).setOnClickListener { sbBlinds.progress = 60 }

        // Устанавливаем начальное значение
        updateBlinds(50)
    }
}