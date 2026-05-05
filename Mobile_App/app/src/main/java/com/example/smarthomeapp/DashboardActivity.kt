package com.example.smarthomeapp

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView

class DashboardActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        // Элементы навигации
        val themeBtn = findViewById<ImageView>(R.id.ivThemeToggle)
        val ivLogout = findViewById<ImageView>(R.id.ivLogout)
        val ivCloud = findViewById<ImageView>(R.id.ivConnectionStatus)

        // Инициализация карточек (ID берем из твоего XML)
        val btnHeating = findViewById<CardView>(R.id.btnHeating)
        val btnVentilation = findViewById<CardView>(R.id.btnVentilation)
        val btnSensors = findViewById<CardView>(R.id.btnWater) // Это наша кнопка Sensors
        val btnAccess = findViewById<CardView>(R.id.btnAccess)
        val btnLighting = findViewById<CardView>(R.id.btnLighting)
        val btnBlinds = findViewById<CardView>(R.id.btnBlinds)

        // Инициализация MQTT
        mqttHandler = MqttHandler(this)

        mqttHandler.connect(
            onConnected = {
                runOnUiThread {
                    // Облачко становится зеленым при успехе
                    ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
                }
            },
            onMessage = { msg ->
                // На дашборде сообщения не обрабатываем
            }
        )

        // --- КНОПКИ ПЕРЕХОДА ---

        // Твоя исправленная кнопка Sensors
        btnSensors.setOnClickListener {
            val intent = Intent(this, SensorsActivity::class.java)
            startActivity(intent)
        }

        btnAccess.setOnClickListener {
            startActivity(Intent(this, AccessActivity::class.java))
        }

        btnLighting.setOnClickListener {
            startActivity(Intent(this, LightingActivity::class.java))
        }

        btnVentilation.setOnClickListener {
            startActivity(Intent(this, VentilationActivity::class.java))
        }

        // Добавим остальные, чтобы не пустовали (если файлы созданы)
        btnHeating.setOnClickListener {
            startActivity(Intent(this, HeatingActivity::class.java))
        }

        btnBlinds.setOnClickListener {
            startActivity(Intent(this, BlindsActivity::class.java))
        }

        // Логика выхода
        ivLogout.setOnClickListener {
            startActivity(Intent(this, MainActivity::class.java))
            finish()
        }

        // Логика переключения темы
        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }
    }
}