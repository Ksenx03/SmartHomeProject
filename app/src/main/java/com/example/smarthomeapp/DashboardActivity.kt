package com.example.smarthomeapp

import android.content.Intent
import android.os.Bundle
import android.widget.ImageView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

class DashboardActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_dashboard)

        val themeBtn: ImageView = findViewById(R.id.ivThemeToggle)

// Проверяем текущую тему ПРИ ЗАПУСКЕ, чтобы поставить нужную иконку
        val currentMode = AppCompatDelegate.getDefaultNightMode()
        if (currentMode == AppCompatDelegate.MODE_NIGHT_YES) {
            themeBtn.setImageResource(R.drawable.ic_theme_dark) // ставим луну
        } else {
            themeBtn.setImageResource(R.drawable.ic_theme_light)   // ставим солнце
        }

        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }

        val ivLogout = findViewById<ImageView>(R.id.ivLogout)

        ivLogout.setOnClickListener {
            // Создаем намерение вернуться на главный экран
            val intent = Intent(this, MainActivity::class.java)

            // ОЧЕНЬ ВАЖНО для SOLID и безопасности:
            // Эти флаги очищают историю переходов, чтобы нельзя было нажать "Назад" и вернуться в систему без логина
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK

            startActivity(intent)
            finish() // Закрываем текущий экран
        }
    }
}