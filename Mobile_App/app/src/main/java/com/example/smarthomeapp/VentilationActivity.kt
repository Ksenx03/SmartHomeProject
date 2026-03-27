package com.example.smarthomeapp

import android.os.Bundle
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class VentilationActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_ventilation)

        val btnBack: ImageView = findViewById(R.id.btnBack)
        val sbFanSpeed: SeekBar = findViewById(R.id.sbFanSpeed)
        val tvFanStatus: TextView = findViewById(R.id.tvFanStatus)
        val btnOff: Button = findViewById(R.id.btnOff)
        val btnMax: Button = findViewById(R.id.btnMax)
        btnBack.setOnClickListener {
            finish() // Просто закрывает этот экран и возвращает на Dashboard
        }

        // Слушатель для ползунка
        sbFanSpeed.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                tvFanStatus.text = "Speed: $progress%"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

// Кнопки пресетов
        btnOff.setOnClickListener {
            sbFanSpeed.progress = 0
            tvFanStatus.text = "Speed: 0%"
        }
        btnMax.setOnClickListener {
            sbFanSpeed.progress = 100
            tvFanStatus.text = "Speed: 100%"
        }
    }
}