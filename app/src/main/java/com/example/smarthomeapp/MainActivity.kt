package com.example.smarthomeapp

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val etLogin = findViewById<EditText>(R.id.etLogin)
        val etPassword = findViewById<EditText>(R.id.etPassword)
        val btnLogIn = findViewById<Button>(R.id.btnLogIn)

        btnLogIn.setOnClickListener {
            val login = etLogin.text.toString()
            val password = etPassword.text.toString()

            // Простая проверка (Позже вынесем в отдельный класс-валидатор по SOLID)
            if (login == "Patryk" && password == "1234") {
                val intent = Intent(this, DashboardActivity::class.java)
                startActivity(intent)
                finish() // Закрываем экран логина, чтобы нельзя было вернуться назад кнопкой
            } else {
                // Выводим ошибку, если данные не те
                etPassword.error = "Wrong login or password"
            }
        }
    }
}