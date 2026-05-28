package com.example.smarthomeapp

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject

class HeatingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var tvCurrentTemp: TextView
    private lateinit var tvTargetTemp: TextView
    private lateinit var sbTemperature: SeekBar

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_heating)

        // Инициализация элементов
        val btnBack = findViewById<ImageView>(R.id.btnBack)
        sbTemperature = findViewById(R.id.sbTemperature)
        tvTargetTemp = findViewById(R.id.tvTargetTemp)
        val btnEco = findViewById<Button>(R.id.btnEco)
        val btnComfort = findViewById<Button>(R.id.btnComfort)
        val btnBoost = findViewById<Button>(R.id.btnBoost)

        // Поле для отображения актуальной температуры с датчика макета
        tvCurrentTemp = findViewById(R.id.tvCurrentTemp)

        // Кнопка Назад
        btnBack.setOnClickListener { finish() }

        // Инициализация MQTT и подписка на датчик среды
        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                Log.d("MQTT", "Heating Activity Connected")
                // Слушаем строго топик температуры Патрика
                mqttHandler.subscribe("makieta/czujniki/srodowisko")
            },
            onMessage = { msg ->
                runOnUiThread {
                    parseHeatingData(msg)
                }
            }
        )

        // Логика ползунка целевой температуры
        sbTemperature.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                tvTargetTemp.text = "$progress°C"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                sendTargetTemperature()
            }
        })

        // Кнопки пресетов
        btnEco.setOnClickListener {
            sbTemperature.progress = 18
            tvTargetTemp.text = "18°C"
            sendTargetTemperature()
        }
        btnComfort.setOnClickListener {
            sbTemperature.progress = 23
            tvTargetTemp.text = "23°C"
            sendTargetTemperature()
        }
        btnBoost.setOnClickListener {
            sbTemperature.progress = 28
            tvTargetTemp.text = "28°C"
            sendTargetTemperature()
        }
    }

    // Ловим и отображаем текущую температуру с макета
// Ловим и отображаем текущую температуру с макета
    private fun parseHeatingData(msg: String) {
        try {
            val cleanMsg = msg.trim()
            if (cleanMsg.startsWith("{")) {
                val json = JSONObject(cleanMsg)
                if (json.has("temperature")) {
                    val temperature = json.optString("temperature", "--")

                    // Добавляем " °C" прямо здесь, чтобы оно склеивалось с живой цифрой!
                    tvCurrentTemp.text = "$temperature °C"
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    // Отправка Патрику целевой температуры, которую выбрал пользователь
    private fun sendTargetTemperature() {
        val target = sbTemperature.progress
        val topic = "makieta/ogrzewanie/ustaw"
        val payload = """{
            "target_temp": $target
        }""".trimIndent()
        mqttHandler.publish(topic, payload)
    }
}