package com.example.smarthomeapp

import android.os.Bundle
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar // Обрати внимание, импорт изменился
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity

class HeatingActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_heating)

        // Инициализация элементов
        val btnBack = findViewById<ImageView>(R.id.btnBack)
        val sbTemperature = findViewById<SeekBar>(R.id.sbTemperature) // ТипSeekBar
        val tvTargetTemp = findViewById<TextView>(R.id.tvTargetTemp)
        val btnEco = findViewById<Button>(R.id.btnEco)
        val btnComfort = findViewById<Button>(R.id.btnComfort)
        val btnBoost = findViewById<Button>(R.id.btnBoost)

        // Кнопка Назад
        btnBack.setOnClickListener { finish() }

        // Логика обычного ползунка
        sbTemperature.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                // В обычном SeekBar прогресс сразу Int
                tvTargetTemp.text = "$progress°C"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        // Кнопки пресетов (теперь без 'f', так как прогресс Int)
        btnEco.setOnClickListener {
            sbTemperature.progress = 18
            tvTargetTemp.text = "18°C"
        }
        btnComfort.setOnClickListener {
            sbTemperature.progress = 23
            tvTargetTemp.text = "23°C"
        }
        btnBoost.setOnClickListener {
            sbTemperature.progress = 28
            tvTargetTemp.text = "28°C"
        }
    }
}