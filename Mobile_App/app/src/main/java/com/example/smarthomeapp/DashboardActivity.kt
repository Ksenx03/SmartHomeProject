package com.example.smarthomeapp

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView
import org.json.JSONObject

class DashboardActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var tvDashboardTemp: TextView
    private lateinit var tvDashboardLight: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        // Элементы навигации
        val themeBtn = findViewById<ImageView>(R.id.ivThemeToggle)
        val ivLogout = findViewById<ImageView>(R.id.ivLogout)
        val ivCloud = findViewById<ImageView>(R.id.ivConnectionStatus)

        // Инициализация полей климата и освещения
        tvDashboardTemp = findViewById(R.id.tvDashboardTemp)
        tvDashboardLight = findViewById(R.id.tvDashboardLight)

        // Инициализация карточек
        val btnHeating = findViewById<CardView>(R.id.btnHeating)
        val btnVentilation = findViewById<CardView>(R.id.btnVentilation)
        val btnSensors = findViewById<CardView>(R.id.btnWater)
        val btnAccess = findViewById<CardView>(R.id.btnAccess)
        val btnLighting = findViewById<CardView>(R.id.btnLighting)
        val btnBlinds = findViewById<CardView>(R.id.btnBlinds)

        // Инициализация MQTT
        mqttHandler = MqttHandler(this)

        mqttHandler.connect(
            onConnected = {
                runOnUiThread {
                    // Облачко становится зеленым при успешном подключении
                    ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
                }

                // Подписываемся строго на актуальные топики прошивки Патрика
                mqttHandler.subscribe("makieta/czujniki/srodowisko")
                mqttHandler.subscribe("makieta/czujniki/swiatlo")
            },
            onMessage = { msg ->
                runOnUiThread {
                    parseClimateData(msg)
                }
            }
        )

        // --- КНОПКИ ПЕРЕХОДА ---

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

    // Разбор климатических данных строго под структуру C++ файлов Патрика
    private fun parseClimateData(msg: String) {
        try {
            val cleanMsg = msg.trim()
            if (cleanMsg.startsWith("{")) {
                val json = JSONObject(cleanMsg)

                // Проверяем температуру (ищет и temp, и temperature на всякий случай)
                if (json.has("temp") || json.has("temperature")) {
                    val key = if (json.has("temp")) "temp" else "temperature"
                    val temperature = json.optString(key, "--")
                    tvDashboardTemp.text = "$temperature °C"
                }

                // Проверяем люксы с датчика освещенности BH1750
                if (json.has("lux")) {
                    val light = json.optString("lux", "--")
                    tvDashboardLight.text = "$light lx"
                }
            } else {
                // Запасной вариант: если данные склеены строкой "24.5;450"
                if (cleanMsg.contains(";")) {
                    val parts = cleanMsg.split(";")
                    tvDashboardTemp.text = "${parts[0]} °C"
                    tvDashboardLight.text = "${parts[1]} lx"
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}