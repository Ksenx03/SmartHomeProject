package com.example.smarthomeapp

import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.os.Bundle
import android.view.View
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.cardview.widget.CardView

class LightingActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_lighting)

        val btnBack = findViewById<ImageView>(R.id.btnBack)
        val colorPreview = findViewById<CardView>(R.id.colorPreviewCard)
        val sbBrightness = findViewById<SeekBar>(R.id.sbBrightness)
        val tvBrightness = findViewById<TextView>(R.id.tvBrightnessLabel)

        btnBack.setOnClickListener { finish() }

        // Список ID наших цветных квадратиков
        val colorIds = listOf(
            R.id.colorRed, R.id.colorGreen, R.id.colorBlue, R.id.colorYellow,
            R.id.colorPurple, R.id.colorOrange, R.id.colorCyan, R.id.colorWhite
        )

        // Вешаем один обработчик на все цвета
        for (id in colorIds) {
            findViewById<View>(id).setOnClickListener { view ->
                val color = (view.background as ColorDrawable).color
                colorPreview.setCardBackgroundColor(color)
            }
        }

        // Логика яркости
        sbBrightness.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                tvBrightness.text = "Brightness: $progress%"
                // Меняем прозрачность карточки (имитация яркости)
                colorPreview.alpha = progress / 100f
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
    }
}