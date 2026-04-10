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
        loadSavedState()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect { }

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

        setupMainFanButton()
        setupSpeedControl()
        setupSpecialModes()

        updateUIState()
    }

    private fun setupMainFanButton() {
        ivFanIcon.setOnClickListener {
            isFanOn = if (isFanOn == "OFF") "ON" else "OFF"
            if (isFanOn == "OFF") currentMode = "none"

            updateUIState()
            saveState()
            sendCommand()
        }
    }

    private fun setupSpeedControl() {
        sbSpeed.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, f: Boolean) {
                currentSpeed = p
                tvSpeedPercent.text = "$p%"
                if (isFanOn == "ON") startFanAnimation(p)
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {
                saveState()
                sendCommand()
            }
        })
    }

    private fun setupSpecialModes() {
        modeButtons.forEach { btn ->
            btn.setOnClickListener {
                val clickedMode = btn.text.toString().lowercase()
                currentMode = if (currentMode == clickedMode) "none" else clickedMode

                when (currentMode) {
                    "turbo" -> { currentSpeed = 100; isFanOn = "ON" }
                    "night" -> { currentSpeed = 30; isFanOn = "ON" }
                    "auto" -> { isFanOn = "ON" }
                }

                updateUIState()
                saveState()
                sendCommand()
            }
        }
    }

    private fun updateUIState() {
        // Updated to English statuses
        tvStatus.text = if (isFanOn == "ON") "Running" else "Stopped"
        tvSpeedPercent.text = "$currentSpeed%"
        sbSpeed.progress = currentSpeed

        if (isFanOn == "ON") {
            tvStatus.setTextColor(fanGreen)
            ivFanIcon.setColorFilter(fanGreen)
            startFanAnimation(currentSpeed)
        } else {
            tvStatus.setTextColor(Color.GRAY)
            ivFanIcon.setColorFilter(Color.LTGRAY)
            ivFanIcon.clearAnimation()
        }

        modeButtons.forEach { btn ->
            val btnMode = btn.text.toString().lowercase()
            if (currentMode == btnMode && isFanOn == "ON") {
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
        val rotate = RotateAnimation(0f, 360f, Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f).apply {
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
        val payload = """{
            "state":"$isFanOn",
            "speed":$currentSpeed,
            "mode":"$currentMode"
        }""".trimIndent()
        mqttHandler.publish(topic, payload)
    }
}