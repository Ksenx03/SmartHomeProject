package com.example.smarthomeapp

import android.animation.ObjectAnimator
import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import java.text.SimpleDateFormat
import java.util.*

class SensorsActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler

    // Аниматоры для мерцания при тревоге
    private var waterAnimator: ObjectAnimator? = null
    private var gasAnimator: ObjectAnimator? = null

    // Переменные для фильтрации логов (чтобы не писать одно и то же каждые 5 сек)
    private var lastWaterStatus = ""
    private var lastGasStatus = ""

    // Цвет из твоего примера (Teal 800)
    private val mainTeal = Color.parseColor("#00695C")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sensors)

        val waterLogs = findViewById<LinearLayout>(R.id.waterLogsContainer)
        val gasLogs = findViewById<LinearLayout>(R.id.gasLogsContainer)

        // Кнопка назад
        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }

        // Настройка MQTT
        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe("makieta/sensors")
            },
            onMessage = { msg ->
                runOnUiThread {
                    handleMqttMessage(msg.trim())
                    updateLogViews(waterLogs, gasLogs)
                }
            }
        )

        // Обработка кнопок RESET (STOP)
        findViewById<Button>(R.id.btnStopWater).setOnClickListener {
            mqttHandler.publish("makieta/sensors/ustaw", "water_off")
        }
        findViewById<Button>(R.id.btnStopGas).setOnClickListener {
            mqttHandler.publish("makieta/sensors/ustaw", "gas_off")
        }

        // Первичная загрузка логов из памяти
        updateLogViews(waterLogs, gasLogs)
    }

    private fun handleMqttMessage(msg: String) {
        val time = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())

        when (msg) {
            "WATER_ALARM", "WATER_OK" -> {
                updateUI("water", msg == "WATER_ALARM")

                // Пишем в лог только если статус реально изменился
                if (msg != lastWaterStatus) {
                    val statusText = if (msg == "WATER_ALARM") "!!! LEAKAGE !!!" else "System Secure"
                    saveLog("water", "[$time] $statusText")
                    lastWaterStatus = msg
                }
            }
            "GAS_ALARM", "GAS_OK" -> {
                updateUI("gas", msg == "GAS_ALARM")

                if (msg != lastGasStatus) {
                    val statusText = if (msg == "GAS_ALARM") "!!! GAS ALERT !!!" else "System Clean"
                    saveLog("gas", "[$time] $statusText")
                    lastGasStatus = msg
                }
            }
        }
    }

    private fun updateUI(type: String, isAlarm: Boolean) {
        val zone = if (type == "water") findViewById<LinearLayout>(R.id.zoneWater) else findViewById<LinearLayout>(R.id.zoneGas)
        val text = if (type == "water") findViewById<TextView>(R.id.tvWaterStatus) else findViewById<TextView>(R.id.tvGasStatus)
        val icon = if (type == "water") findViewById<ImageView>(R.id.ivWaterIcon) else findViewById<ImageView>(R.id.ivGasIcon)

        // Сбрасываем старую анимацию и прозрачность
        if (type == "water") waterAnimator?.cancel() else gasAnimator?.cancel()
        zone.alpha = 1.0f

        if (isAlarm) {
            // РЕЖИМ ТРЕВОГИ
            if (type == "water") {
                zone.setBackgroundResource(R.drawable.bg_water_alarm) // Синий
                text.text = "WATER: !!! ALARM !!!"
                text.setTextColor(Color.WHITE)
                icon.setColorFilter(Color.WHITE)
                startBlinkingAnimation("water", zone)
            } else {
                zone.setBackgroundResource(R.drawable.bg_gas_alarm) // Желтый
                text.text = "GAS: !!! DANGER !!!"
                text.setTextColor(Color.BLACK)
                icon.setColorFilter(Color.BLACK)
                startBlinkingAnimation("gas", zone)
            }
        } else {
            // РЕЖИМ БЕЗОПАСНОСТИ (Твой бирюзовый полупрозрачный)
            zone.setBackgroundResource(R.drawable.bg_sensor_card)
            text.text = if (type == "water") "WATER: SECURE" else "GAS: CLEAN"
            text.setTextColor(mainTeal)
            icon.setColorFilter(mainTeal)
        }
    }

    private fun startBlinkingAnimation(type: String, view: View) {
        val animator = ObjectAnimator.ofFloat(view, "alpha", 1.0f, 0.4f).apply {
            duration = 500
            repeatCount = ValueAnimator.INFINITE
            repeatMode = ValueAnimator.REVERSE
            interpolator = AccelerateDecelerateInterpolator()
            start()
        }
        if (type == "water") waterAnimator = animator else gasAnimator = animator
    }

    // --- РАБОТА С ЛОГАМИ (SharedPrefs) ---
    private fun saveLog(type: String, entry: String) {
        val prefs = getSharedPreferences("SensorLogs", Context.MODE_PRIVATE)
        val key = if (type == "water") "waterLogs" else "gasLogs"
        val current = prefs.getString(key, "") ?: ""

        // Ограничиваем историю 5 записями
        val logList = (entry + "\n" + current).split("\n").filter { it.isNotBlank() }
        val updated = logList.take(5).joinToString("\n")

        prefs.edit().putString(key, updated).apply()
    }

    private fun updateLogViews(wContainer: LinearLayout, gContainer: LinearLayout) {
        fillContainer(wContainer, "waterLogs")
        fillContainer(gContainer, "gasLogs")
    }

    private fun fillContainer(container: LinearLayout, key: String) {
        container.removeAllViews()
        val logs = getSharedPreferences("SensorLogs", Context.MODE_PRIVATE).getString(key, "") ?: ""

        logs.split("\n").forEach { line ->
            if (line.isNotEmpty()) {
                val tv = TextView(this).apply {
                    text = line
                    textSize = 11f
                    // Если в строке есть "!!!", красим в красный, иначе в серый
                    setTextColor(if (line.contains("!!!")) Color.RED else Color.GRAY)
                    setPadding(0, 4, 0, 4)
                }
                container.addView(tv)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        waterAnimator?.cancel()
        gasAnimator?.cancel()
    }
}