package com.example.smarthomeapp

import android.content.Intent
import android.os.Bundle
import android.widget.ImageView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView

class DashboardActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_dashboard)

        // 1. Находим элементы
        val themeBtn: ImageView = findViewById(R.id.ivThemeToggle)
        val ivLogout: ImageView = findViewById(R.id.ivLogout)
        val btnHeating: CardView = findViewById(R.id.btnHeating)
        val btnVentilation: CardView = findViewById(R.id.btnVentilation)
        val btnWater: CardView = findViewById(R.id.btnWater)
        val btnAccess: CardView = findViewById(R.id.btnAccess)
        val btnLighting: CardView = findViewById(R.id.btnLighting)
        val btnBlinds: CardView = findViewById(R.id.btnBlinds)

        // 2. Устанавливаем правильную иконку темы ПРИ ЗАГРУЗКЕ
        // MODE_NIGHT_YES = 2, MODE_NIGHT_NO = 1
        val currentMode = AppCompatDelegate.getDefaultNightMode()
        if (currentMode == AppCompatDelegate.MODE_NIGHT_YES) {
            themeBtn.setImageResource(R.drawable.ic_theme_dark) // Луна
        } else {
            themeBtn.setImageResource(R.drawable.ic_theme_light) // Солнце
        }

        // 3. Логика переключения темы
        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
            // После этой команды Activity перезапустится сама
        }

        // 4. Логика выхода (Logout)
        ivLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // 5. Переход на Heating (Отопление)
        btnHeating.setOnClickListener {
            val intent = Intent(this, HeatingActivity::class.java)
            startActivity(intent)
        }

        btnVentilation.setOnClickListener {
            val intent = Intent(this, VentilationActivity::class.java)
            startActivity(intent)
        }

        btnWater.setOnClickListener {
            val intent = Intent(this, WaterSensorActivity::class.java)
            startActivity(intent)
        }

        btnAccess.setOnClickListener {
            startActivity(Intent(this, AccessActivity::class.java))
        }

        btnLighting.setOnClickListener {
            startActivity(Intent(this, LightingActivity::class.java))
        }

        btnBlinds.setOnClickListener {
            startActivity(Intent(this, BlindsActivity::class.java))
        }

    }
}