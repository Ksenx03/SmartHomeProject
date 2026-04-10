package com.example.smarthomeapp

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView

class DashboardActivity : AppCompatActivity() {

    // Объявляем нашего связиста
    private lateinit var mqttHandler: MqttHandler

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_dashboard)

        // Находим все элементы экрана
        val themeBtn: ImageView = findViewById(R.id.ivThemeToggle)
        val ivLogout: ImageView = findViewById(R.id.ivLogout)
        val btnHeating: CardView = findViewById(R.id.btnHeating)
        val btnVentilation: CardView = findViewById(R.id.btnVentilation)
        val btnWater: CardView = findViewById(R.id.btnWater)
        val btnAccess: CardView = findViewById(R.id.btnAccess)
        val btnLighting: CardView = findViewById(R.id.btnLighting)
        val btnBlinds: CardView = findViewById(R.id.btnBlinds)

        // Находим твое облачко по ID
        val ivCloud: ImageView = findViewById(R.id.ivConnectionStatus)

        // Инициализируем связиста
        mqttHandler = MqttHandler(this)

        // Даем команду на подключение
        mqttHandler.connect {
            // Как только Малинка ответит — выполняем перекраску!
            runOnUiThread {
                // Заливаем базовую картинку зеленым цветом 🟢
                ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
            }
        }

        // Логика переключения темы
        val currentMode = AppCompatDelegate.getDefaultNightMode()
        if (currentMode == AppCompatDelegate.MODE_NIGHT_YES) {
            themeBtn.setImageResource(R.drawable.ic_theme_dark)
        } else {
            themeBtn.setImageResource(R.drawable.ic_theme_light)
        }

        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }

        // Логика выхода (Logout)
        ivLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // Переходы по кнопкам
        btnHeating.setOnClickListener { startActivity(Intent(this, HeatingActivity::class.java)) }
        btnVentilation.setOnClickListener { startActivity(Intent(this, VentilationActivity::class.java)) }
        btnWater.setOnClickListener { startActivity(Intent(this, WaterSensorActivity::class.java)) }
        btnAccess.setOnClickListener { startActivity(Intent(this, AccessActivity::class.java)) }
        btnLighting.setOnClickListener { startActivity(Intent(this, LightingActivity::class.java)) }
        btnBlinds.setOnClickListener { startActivity(Intent(this, BlindsActivity::class.java)) }
    }
}