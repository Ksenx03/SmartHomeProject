package com.example.smarthomeapp

import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import java.text.SimpleDateFormat
import java.util.*

class SensorsActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private val heatingGreen = Color.parseColor("#00695C")
    private val alertBlue = Color.parseColor("#1976D2")
    private val alertYellow = Color.parseColor("#FFC107")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sensors)

        val waterLogs = findViewById<LinearLayout>(R.id.waterLogsContainer)
        val gasLogs = findViewById<LinearLayout>(R.id.gasLogsContainer)

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = { },
            onMessage = { msg ->
                runOnUiThread {
                    handleMqttMessage(msg)
                    updateLogViews(waterLogs, gasLogs)
                }
            }
        )

        findViewById<Button>(R.id.btnStopWater).setOnClickListener {
            mqttHandler.publish("makieta/sensors/ustaw", "water_off")
        }
        findViewById<Button>(R.id.btnStopGas).setOnClickListener {
            mqttHandler.publish("makieta/sensors/ustaw", "gas_off")
        }

        updateLogViews(waterLogs, gasLogs)
    }

    private fun handleMqttMessage(msg: String) {
        val time = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        when (msg) {
            "WATER_ALARM" -> { updateUI("water", true); saveLog("water", "[$time] !!! LEAKAGE !!!") }
            "WATER_OK" -> { updateUI("water", false); saveLog("water", "[$time] System Secure") }
            "GAS_ALARM" -> { updateUI("gas", true); saveLog("gas", "[$time] !!! GAS ALERT !!!") }
            "GAS_OK" -> { updateUI("gas", false); saveLog("gas", "[$time] System Clean") }
        }
    }

    private fun updateUI(type: String, isAlarm: Boolean) {
        if (type == "water") {
            val zone = findViewById<LinearLayout>(R.id.zoneWater)
            val text = findViewById<TextView>(R.id.tvWaterStatus)
            val icon = findViewById<ImageView>(R.id.ivWaterIcon)
            if (isAlarm) {
                zone.setBackgroundColor(alertBlue)
                text.text = "WATER: !!! ALARM !!!"; text.setTextColor(Color.WHITE); icon.setColorFilter(Color.WHITE)
            } else {
                zone.setBackgroundColor(Color.parseColor("#0D00695C"))
                text.text = "WATER: SECURE"; text.setTextColor(heatingGreen); icon.setColorFilter(heatingGreen)
            }
        } else {
            val zone = findViewById<LinearLayout>(R.id.zoneGas)
            val text = findViewById<TextView>(R.id.tvGasStatus)
            val icon = findViewById<ImageView>(R.id.ivGasIcon)
            if (isAlarm) {
                zone.setBackgroundColor(alertYellow)
                text.text = "GAS: !!! DANGER !!!"; text.setTextColor(Color.BLACK); icon.setColorFilter(Color.BLACK)
            } else {
                zone.setBackgroundColor(Color.TRANSPARENT)
                text.text = "GAS: CLEAN"; text.setTextColor(heatingGreen); icon.setColorFilter(heatingGreen)
            }
        }
    }

    private fun saveLog(type: String, entry: String) {
        val prefs = getSharedPreferences("SensorLogs", Context.MODE_PRIVATE)
        val key = if (type == "water") "waterLogs" else "gasLogs"
        val current = prefs.getString(key, "") ?: ""
        val updated = (entry + "\n" + current).split("\n").take(15).joinToString("\n")
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
                    text = line; textSize = 12f
                    setTextColor(if (line.contains("!!!")) Color.RED else Color.DKGRAY)
                }
                container.addView(tv)
            }
        }
    }
}