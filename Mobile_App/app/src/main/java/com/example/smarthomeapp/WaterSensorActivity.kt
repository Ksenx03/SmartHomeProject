package com.example.smarthomeapp

import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.cardview.widget.CardView

class WaterSensorActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_water_sensor)

        // Проверь, чтобы эти ID были в activity_water_sensor.xml
        val statusCard = findViewById<CardView>(R.id.statusCard)
        val statusIcon = findViewById<ImageView>(R.id.ivStatusIcon)
        val statusText = findViewById<TextView>(R.id.tvStatusText)
        val btnSimulate = findViewById<Button>(R.id.btnSimulate)
        val btnBack = findViewById<ImageView>(R.id.btnBack)

        btnBack.setOnClickListener { finish() }

        var isLeakage = false

        btnSimulate.setOnClickListener {
            isLeakage = !isLeakage
            if (isLeakage) {
                statusCard.setCardBackgroundColor(Color.parseColor("#FFEBEE"))
                statusIcon.setColorFilter(Color.RED)
                statusText.text = "LEAKAGE DETECTED!"
                statusText.setTextColor(Color.RED)
                btnSimulate.text = "Reset System"
            } else {
                statusCard.setCardBackgroundColor(Color.parseColor("#E0F2F1"))
                statusIcon.setColorFilter(Color.parseColor("#00695C"))
                statusText.text = "SYSTEM OK"
                statusText.setTextColor(Color.parseColor("#00695C"))
                btnSimulate.text = "Simulate Leakage"
            }
        }
    }
}