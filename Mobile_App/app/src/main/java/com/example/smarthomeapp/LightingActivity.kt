package com.example.smarthomeapp

import android.content.Context
import android.content.SharedPreferences
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.google.android.material.switchmaterial.SwitchMaterial
import com.google.android.material.tabs.TabLayout

class LightingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var prefs: SharedPreferences

    // Элементы интерфейса
    private lateinit var ivLightButton: ImageView
    private lateinit var tvStatus: TextView
    private lateinit var tvBrightnessPercent: TextView
    private lateinit var sbBrightness: SeekBar
    private lateinit var modeButtons: List<Button>
    private lateinit var tabLayout: TabLayout
    private lateinit var layoutAutoMode: LinearLayout
    private lateinit var switchAutoMode: SwitchMaterial

    // Цвета из дизайна
    private val heatingGreen = Color.parseColor("#00695C")
    private val buttonBgInactive = Color.parseColor("#4D00695C")

    // Текущая выбранная зона управления ("wew" или "zew")
    private var currentTarget = "wew"

    // Переменные состояния
    private var isLightOn = "OFF"
    private var currentBrightness = 100
    private var currentR = 255; private var currentG = 255; private var currentB = 255
    private var currentMode = "none"

    // Палитра без черного цвета (заменили на уютный теплый белый)
    private val colorPalette = arrayOf(
        "#FF0000", "#FF4500", "#FF8C00", "#FFA500", "#FFD700", "#FFFF00",
        "#CCFF00", "#80FF00", "#00FF00", "#00FF80", "#00FFFF", "#00CCFF",
        "#0066FF", "#0000FF", "#4B0082", "#7B00FF", "#B000FF", "#FF00FF",
        "#FF0080", "#FF0040", "#8B4513", "#708090", "#FFFFFF", "#FFE4B5"
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_lighting)

        prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        loadSavedState("wew")

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = { Log.d("MQTT", "Lighting Activity Connected") },
            onMessage = { msg -> Log.d("MQTT", "Received light status: $msg") }
        )

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        ivLightButton = findViewById(R.id.ivLightButton)
        tvStatus = findViewById(R.id.tvLightStatus)
        tvBrightnessPercent = findViewById(R.id.tvBrightnessPercent)
        sbBrightness = findViewById(R.id.brightnessSeekBar)
        tabLayout = findViewById(R.id.tabLightTarget)
        layoutAutoMode = findViewById(R.id.layoutAutoMode)
        switchAutoMode = findViewById(R.id.switchAutoMode)

        modeButtons = listOf(
            findViewById(R.id.btnDisco),
            findViewById(R.id.btnRelax),
            findViewById(R.id.btnStrobe)
        )

        setupTargetTabs()
        setupMainLightButton()
        setupBrightnessControl()
        setupColorGrid()
        setupSpecialModes()
        setupAutoModeSwitch()

        updateUIState()
    }

    private fun setupTargetTabs() {
        tabLayout.addOnTabSelectedListener(object : TabLayout.OnTabSelectedListener {
            override fun onTabSelected(tab: TabLayout.Tab?) {
                currentTarget = if (tab?.position == 0) "wew" else "zew"
                loadSavedState(currentTarget)
                updateUIState()
            }
            override fun onTabUnselected(tab: TabLayout.Tab?) {}
            override fun onTabReselected(tab: TabLayout.Tab?) {}
        })
    }

    private fun setupMainLightButton() {
        ivLightButton.setOnClickListener {
            isLightOn = if (isLightOn == "OFF") "ON" else "OFF"
            if (isLightOn == "OFF") currentMode = "none"

            updateUIState()
            saveState(currentTarget)
            sendCommand()
        }
    }

    private fun setupBrightnessControl() {
        sbBrightness.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, f: Boolean) {
                currentBrightness = p
                tvBrightnessPercent.text = "$p%"
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {
                saveState(currentTarget)
                sendCommand()
            }
        })
    }

    private fun setupColorGrid() {
        val grid = findViewById<GridLayout>(R.id.colorGrid)
        grid.removeAllViews()

        val density = resources.displayMetrics.density
        val size = (40 * density).toInt()
        val margin = (5 * density).toInt()

        for (colorHex in colorPalette) {
            val colorView = View(this)
            val params = GridLayout.LayoutParams()
            params.width = size
            params.height = size
            params.setMargins(margin, margin, margin, margin)
            colorView.layoutParams = params

            val drawable = ContextCompat.getDrawable(this, R.drawable.color_circle)?.mutate() as GradientDrawable
            drawable.setColor(Color.parseColor(colorHex))
            colorView.background = drawable

            colorView.setOnClickListener {
                currentMode = "none"
                isLightOn = "ON"

                val color = Color.parseColor(colorHex)
                currentR = Color.red(color)
                currentG = Color.green(color)
                currentB = Color.blue(color)

                updateUIState()
                saveState(currentTarget)
                sendCommand()

                colorView.animate().scaleX(0.8f).scaleY(0.8f).setDuration(100).withEndAction {
                    colorView.animate().scaleX(1f).scaleY(1f).setDuration(100).start()
                }.start()
            }
            grid.addView(colorView)
        }
    }

    private fun setupSpecialModes() {
        modeButtons.forEach { btn ->
            btn.setOnClickListener {
                val clickedMode = btn.text.toString().lowercase()
                currentMode = if (currentMode == clickedMode) "none" else clickedMode
                if (currentMode != "none") isLightOn = "ON"

                updateUIState()
                saveState(currentTarget)
                sendCommand()
            }
        }
    }

    // Логика работы переключателя автоматического режима
    private fun setupAutoModeSwitch() {
        switchAutoMode.setOnCheckedChangeListener { buttonView, isChecked ->
            // Sprawdzamy czy zmiana pochodzi od fizycznego kliknięcia użytkownika
            if (buttonView.isPressed) {
                currentMode = if (isChecked) "auto" else "none"
                updateUIState()
                saveState(currentTarget)
                sendCommand()
            }
        }
    }

    private fun updateUIState() {
        // Показываем блок переключателя авторежима ТОЛЬКО для Outdoor (zew)
        if (currentTarget == "zew") {
            layoutAutoMode.visibility = View.VISIBLE
            switchAutoMode.isChecked = (currentMode == "auto")
        } else {
            layoutAutoMode.visibility = View.GONE
        }

        // Если включен авторежим, пишем AUTO, иначе текущее состояние (ON/OFF)
        tvStatus.text = if (currentMode == "auto" && isLightOn == "OFF") "AUTO" else if (currentMode == "auto" && isLightOn == "ON") "AUTO (ACTIVE)" else isLightOn

        // Лампочка загорается ТОЛЬКО если реальный статус света ON (неважно, ручной это режим или авто)
        if (isLightOn == "ON") {
            tvStatus.setTextColor(heatingGreen)
            // Если горит в авторежиме — сделаем красивый золотой, если вручную — RGB цвет
            if (currentMode == "auto") {
                ivLightButton.setColorFilter(Color.parseColor("#FFD700"))
            } else {
                ivLightButton.setColorFilter(Color.rgb(currentR, currentG, currentB))
            }
        } else {
            // Если свет OFF (даже при включенном авторежиме, пока светло) — лампочка серая!
            tvStatus.setTextColor(Color.GRAY)
            ivLightButton.setColorFilter(Color.parseColor("#4400695C"))
        }

        modeButtons.forEach { btn ->
            val btnMode = btn.text.toString().lowercase()
            if (currentMode == btnMode && isLightOn == "ON") {
                btn.backgroundTintList = ColorStateList.valueOf(heatingGreen)
                btn.setTextColor(Color.WHITE)
            } else {
                btn.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
                btn.setTextColor(heatingGreen)
            }
        }

        tvBrightnessPercent.text = "$currentBrightness%"
        sbBrightness.progress = currentBrightness
    }

    private fun saveState(target: String) {
        prefs.edit().apply {
            putString("${target}_lightState", isLightOn)
            putInt("${target}_brightness", currentBrightness)
            putInt("${target}_r", currentR)
            putInt("${target}_g", currentG)
            putInt("${target}_b", currentB)
            putString("${target}_mode", currentMode)
            apply()
        }
    }

    private fun loadSavedState(target: String) {
        isLightOn = prefs.getString("${target}_lightState", "OFF") ?: "OFF"
        currentBrightness = prefs.getInt("${target}_brightness", 100)
        currentR = prefs.getInt("${target}_r", 255)
        currentG = prefs.getInt("${target}_g", 255)
        currentB = prefs.getInt("${target}_b", 255)
        currentMode = prefs.getString("${target}_mode", "none") ?: "none"
    }

    private fun sendCommand() {
        // 1. Standardowa komenda JSON do sterownika LED
        val topic = "makieta/oswietlenie/ustaw"
        val payload = """{
        "target":"$currentTarget",
        "state":"$isLightOn",
        "mode":"$currentMode",
        "brightness":$currentBrightness,
        "color":{"r":$currentR,"g":$currentG,"b":$currentB}
    }""".trimIndent()
        mqttHandler.publish(topic, payload)

        // 2. KRYTYCZNE DLA TWOJEGO PROBLEMU: Sterowanie flagą automatyki w ESP32
        if (currentTarget == "zew") {
            val autoTopic = "makieta/oswietlenie/automatyka"
            // Jeśli currentMode to "auto", wysyłamy "ON", w przeciwnym razie "OFF"
            val autoPayload = if (currentMode == "auto") "ON" else "OFF"
            mqttHandler.publish(autoTopic, autoPayload)
        }
    }
}