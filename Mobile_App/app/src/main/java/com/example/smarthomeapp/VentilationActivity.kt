package com.example.smarthomeapp

import android.content.Context
import android.content.SharedPreferences
import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Bundle
import android.view.animation.Animation
import android.view.animation.LinearInterpolator
import android.view.animation.RotateAnimation
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject

class VentilationActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var prefs: SharedPreferences

    private lateinit var ivFanIcon: ImageView
    private lateinit var tvStatus: TextView
    private lateinit var tvSpeedPercent: TextView
    private lateinit var sbSpeed: SeekBar
    private lateinit var modeButtons: List<Button>

    private val fanGreen = Color.parseColor("#00695C")
    private val buttonBgInactive = Color.parseColor("#4D00695C")

    private var isFanOn = "OFF"
    private var currentSpeed = 50
    private var currentMode = "none"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_ventilation)

        prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        ivFanIcon = findViewById(R.id.ivFanIcon)
        tvStatus = findViewById(R.id.tvFanStatus)
        tvSpeedPercent = findViewById(R.id.tvSpeedPercent)
        sbSpeed = findViewById(R.id.sbFanSpeed)

        modeButtons = listOf(
            findViewById(R.id.btnAuto),
            findViewById(R.id.btnNight),
            findViewById(R.id.btnTurbo)
        )

        loadSavedState()
        updateUIState()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe("makieta/status/systemy")
            },
            onMessage = { msg ->
                runOnUiThread {
                    handleSystemStatus(msg)
                }
            }
        )

        setupMainFanButton()
        setupSpeedControl()
        setupSpecialModes()
    }

    override fun onResume() {
        super.onResume()
        loadSavedState()
        updateUIState()
    }

    private fun handleSystemStatus(msg: String) {
        try {
            val clean = msg.trim()
            if (!clean.startsWith("{")) return
            val json = JSONObject(clean)

            if (json.has("status_vent")) {
                val remoteState = json.getString("status_vent")
                val remoteSpeed = json.optInt("fan_speed", currentSpeed)
                isFanOn = remoteState
                if (remoteState == "ON" && remoteSpeed > 0) {
                    currentSpeed = remoteSpeed
                }
                saveState()
                updateUIState()
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun setupMainFanButton() {
        ivFanIcon.setOnClickListener {
            isFanOn = if (isFanOn == "OFF") "ON" else "OFF"
            if (isFanOn == "ON" && currentSpeed == 0) currentSpeed = 50
            if (isFanOn == "OFF") currentMode = "none"

            saveState()
            updateUIState()
            sendCommand()
        }
    }

    private fun setupSpeedControl() {
        sbSpeed.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) {
                if (fromUser) {
                    currentSpeed = p
                    tvSpeedPercent.text = "$p%"
                    isFanOn = if (p > 0) "ON" else "OFF"
                    currentMode = "none"
                    if (isFanOn == "ON") startFanAnimation(p) else ivFanIcon.clearAnimation()
                }
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {
                saveState()
                updateUIState()
                sendCommand()
            }
        })
    }

    private fun setupSpecialModes() {
        findViewById<Button>(R.id.btnNight).setOnClickListener { applyMode("night", 30) }
        findViewById<Button>(R.id.btnAuto).setOnClickListener { applyMode("auto", 60) }
        findViewById<Button>(R.id.btnTurbo).setOnClickListener { applyMode("turbo", 100) }
    }

    private fun applyMode(modeKey: String, targetSpeed: Int) {
        if (currentMode == modeKey && isFanOn == "ON") {
            currentMode = "none"
        } else {
            currentMode = modeKey
            currentSpeed = targetSpeed
            isFanOn = "ON"
        }
        saveState()
        updateUIState()
        sendCommand()
    }

    private fun updateUIState() {
        tvStatus.text = if (isFanOn == "ON") "Running ($currentSpeed%)" else "Stopped"
        tvSpeedPercent.text = "$currentSpeed%"
        sbSpeed.progress = currentSpeed

        if (isFanOn == "ON" && currentSpeed > 0) {
            tvStatus.setTextColor(fanGreen)
            ivFanIcon.setColorFilter(fanGreen)
            startFanAnimation(currentSpeed)
        } else {
            tvStatus.setTextColor(Color.GRAY)
            ivFanIcon.setColorFilter(Color.LTGRAY)
            ivFanIcon.clearAnimation()
        }

        val btnMap = mapOf(
            "auto" to findViewById<Button>(R.id.btnAuto),
            "night" to findViewById<Button>(R.id.btnNight),
            "turbo" to findViewById<Button>(R.id.btnTurbo)
        )

        btnMap.forEach { (key, btn) ->
            if (currentMode == key && isFanOn == "ON") {
                btn.backgroundTintList = ColorStateList.valueOf(fanGreen)
                btn.setTextColor(Color.WHITE)
            } else {
                btn.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
                btn.setTextColor(fanGreen)
            }
        }
    }

    private fun startFanAnimation(speed: Int) {
        ivFanIcon.clearAnimation()
        if (speed <= 0) return
        val duration = (2000 - (speed * 18)).toLong().coerceAtLeast(100)
        val rotate = RotateAnimation(
            0f, 360f,
            Animation.RELATIVE_TO_SELF, 0.5f,
            Animation.RELATIVE_TO_SELF, 0.5f
        ).apply {
            this.duration = duration
            interpolator = LinearInterpolator()
            repeatCount = Animation.INFINITE
        }
        ivFanIcon.startAnimation(rotate)
    }

    private fun saveState() {
        prefs.edit().apply {
            putString("fanState", isFanOn)
            putInt("fanSpeed", currentSpeed)
            putString("fanMode", currentMode)
            putString("status_vent", isFanOn) // Aktualizuje również status na Dashboardzie
            apply()
        }
    }

    private fun loadSavedState() {
        isFanOn = prefs.getString("fanState", "OFF") ?: "OFF"
        currentSpeed = prefs.getInt("fanSpeed", 50)
        currentMode = prefs.getString("fanMode", "none") ?: "none"
    }

    private fun sendCommand() {
        val topic = "makieta/wentylator/ustaw"
        val payload = """{"state":"$isFanOn","speed":$currentSpeed,"mode":"$currentMode"}"""
        mqttHandler.publish(topic, payload, retained = true)
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttHandler.disconnect()
    }
}