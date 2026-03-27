package com.example.smarthomeapp

import android.os.Bundle
import android.view.Gravity
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import java.text.SimpleDateFormat
import java.util.*

class AccessActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_access)

        val btnBack = findViewById<ImageView>(R.id.btnBack)
        val btnSimulate = findViewById<Button>(R.id.btnSimulateCard)
        val logsContainer = findViewById<LinearLayout>(R.id.logsContainer)
        val tvDoorStatus = findViewById<TextView>(R.id.tvDoorStatus)

        btnBack.setOnClickListener { finish() }

        // Список наших "разрешенных" ID
        val validCards = listOf("4422", "8855")

        btnSimulate.setOnClickListener {
            // Имитируем прикладывание карты 4422
            val cardId = "4422"
            addLog(cardId, logsContainer)

            // "Открываем" дверь на 2 секунды
            tvDoorStatus.text = "DOOR OPENED"
            tvDoorStatus.setTextColor(android.graphics.Color.BLUE)

            btnSimulate.postDelayed({
                tvDoorStatus.text = "DOOR CLOSED"
                tvDoorStatus.setTextColor(android.graphics.Color.parseColor("#2E7D32"))
            }, 2000)
        }
    }

    private fun addLog(id: String, container: LinearLayout) {
        val sdf = SimpleDateFormat("dd/MM/yyyy HH:mm:ss", Locale.getDefault())
        val currentDate = sdf.format(Date())

        val textView = TextView(this)
        textView.text = "ID: $id | $currentDate | Authorized"
        textView.setPadding(0, 10, 0, 10)
        textView.textSize = 14f

                // Добавляем новую запись в начало списка
                container.addView(textView, 0)
    }
}