package com.example.smarthomeapp

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.os.Build
import android.os.Bundle
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.google.android.material.switchmaterial.SwitchMaterial
import java.util.Locale

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        // 1. Проверяем и применяем выбранный язык до отрисовки экрана
        loadLocale()

        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // 2. Запрос разрешений на уведомления
        checkNotificationPermission()

        // 3. Запуск фоновой службы MQTT
        val serviceIntent = Intent(this, MgttNotificationService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(serviceIntent)
        } else {
            startService(serviceIntent)
        }

        // Инициализация элементов логина
        val etLogin = findViewById<EditText>(R.id.etLogin)
        val etPassword = findViewById<EditText>(R.id.etPassword)
        val btnLogIn = findViewById<Button>(R.id.btnLogIn)

        // Инициализация и настройка свитча языка
        val switchLanguage = findViewById<SwitchMaterial>(R.id.switchLanguage)
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        val currentLang = prefs.getString("app_lang", "en") ?: "en"

        // Устанавливаем свитч в нужное положение на основе сохраненного языка
        switchLanguage.isChecked = (currentLang == "pl")
        switchLanguage.text = if (currentLang == "pl") "PL" else "EN"

        // Слушатель переключения языка
        switchLanguage.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                setAppLocale("pl")
            } else {
                setAppLocale("en")
            }
        }

        // Логика кнопки входа
        btnLogIn.setOnClickListener {
            val login = etLogin.text.toString()
            val password = etPassword.text.toString()

            if (login == "Patryk" && password == "1234") {
                val intent = Intent(this, DashboardActivity::class.java)
                startActivity(intent)
                finish()
            } else {
                // Текст берется из strings.xml, чтобы переводиться на ходу
                etPassword.error = getString(R.string.error_wrong_credentials)
            }
        }
    }

    // Изменение локали приложения
    private fun setAppLocale(languageCode: String) {
        val locale = Locale(languageCode)
        Locale.setDefault(locale)

        val config = Configuration()
        config.setLocale(locale)

        baseContext.resources.updateConfiguration(config, baseContext.resources.displayMetrics)

        // Сохраняем выбор в память телефона
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        prefs.edit().putString("app_lang", languageCode).apply()

        // Перезапускаем MainActivity для мгновенного обновления интерфейса
        val intent = intent
        finish()
        startActivity(intent)
    }

    // Загрузка сохраненного языка при старте приложения
    private fun loadLocale() {
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        val language = prefs.getString("app_lang", "") ?: ""
        if (language.isNotEmpty()) {
            val locale = Locale(language)
            Locale.setDefault(locale)
            val config = Configuration()
            config.setLocale(locale)
            baseContext.resources.updateConfiguration(config, baseContext.resources.displayMetrics)
        }
    }

    private fun checkNotificationPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.POST_NOTIFICATIONS), 101)
            }
        }
    }
}