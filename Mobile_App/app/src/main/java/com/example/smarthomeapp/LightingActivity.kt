package com.example.smarthomeapp

import android.content.Context
import android.content.SharedPreferences
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat

class LightingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var prefs: SharedPreferences

    // Элементы интерфейса
    private lateinit var ivLightButton: ImageView
    private lateinit var tvStatus: TextView
    private lateinit var tvBrightnessPercent: TextView
    private lateinit var sbBrightness: SeekBar
    private lateinit var modeButtons: List<Button>

    // Цвета из твоего идеального дизайна (Heating)
    private val heatingGreen = Color.parseColor("#00695C") // Насыщенный бирюзовый
    private val buttonBgInactive = Color.parseColor("#4D00695C") // Полупрозрачный бирюзовый

    // Переменные состояния
    private var isLightOn = "OFF"
    private var currentBrightness = 100
    private var currentR = 255; private var currentG = 255; private var currentB = 255
    private var currentMode = "none"

    // Палитра цветов (24 штуки)
    private val colorPalette = arrayOf(
        "#FF0000", "#FF4500", "#FF8C00", "#FFA500", "#FFD700", "#FFFF00",
        "#CCFF00", "#80FF00", "#00FF00", "#00FF80", "#00FFFF", "#00CCFF",
        "#0066FF", "#0000FF", "#4B0082", "#7B00FF", "#B000FF", "#FF00FF",
        "#FF0080", "#FF0040", "#8B4513", "#708090", "#FFFFFF", "#333333"
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_lighting)

        // 1. Инициализация памяти и MQTT
        prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        loadSavedState()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect { }

        // 2. Поиск элементов на экране
        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        ivLightButton = findViewById(R.id.ivLightButton)
        tvStatus = findViewById(R.id.tvLightStatus)
        tvBrightnessPercent = findViewById(R.id.tvBrightnessPercent)
        sbBrightness = findViewById(R.id.brightnessSeekBar)

        // Список кнопок спецэффектов для удобного управления
        modeButtons = listOf(
            findViewById(R.id.btnDisco),
            findViewById(R.id.btnRelax),
            findViewById(R.id.btnStrobe)
        )

        // 3. Настройка слушателей
        setupMainLightButton()
        setupBrightnessControl()
        setupColorGrid()
        setupSpecialModes()

        // 4. Первичное обновление экрана
        updateUIState()
    }

    private fun setupMainLightButton() {
        ivLightButton.setOnClickListener {
            isLightOn = if (isLightOn == "OFF") "ON" else "OFF"
            // Если выключаем свет, сбрасываем и режим
            if (isLightOn == "OFF") currentMode = "none"

            updateUIState()
            saveState()
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
                saveState()
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
                // При выборе цвета режим "Диско" и т.д. выключается
                currentMode = "none"
                isLightOn = "ON"

                val color = Color.parseColor(colorHex)
                currentR = Color.red(color)
                currentG = Color.green(color)
                currentB = Color.blue(color)

                updateUIState()
                saveState()
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

                // Логика переключателя: если нажали на уже активный — выключаем
                currentMode = if (currentMode == clickedMode) "none" else clickedMode

                // Если режим выбран, свет должен быть включен
                if (currentMode != "none") isLightOn = "ON"

                updateUIState()
                saveState()
                sendCommand()
            }
        }
    }

    private fun updateUIState() {
        // Статус ON/OFF
        tvStatus.text = isLightOn

        // Вид лампочки
        if (isLightOn == "ON") {
            tvStatus.setTextColor(heatingGreen)
            ivLightButton.setColorFilter(Color.rgb(currentR, currentG, currentB))
        } else {
            tvStatus.setTextColor(Color.GRAY)
            ivLightButton.setColorFilter(Color.parseColor("#4400695C"))
        }

        // Обновление кнопок спецэффектов (Выделение цветом)
        modeButtons.forEach { btn ->
            val btnMode = btn.text.toString().lowercase()
            if (currentMode == btnMode && isLightOn == "ON") {
                // АКТИВНЫЙ РЕЖИМ: Темно-зеленый фон, белый текст
                btn.backgroundTintList = ColorStateList.valueOf(heatingGreen)
                btn.setTextColor(Color.WHITE)
            } else {
                // НЕАКТИВНЫЙ РЕЖИМ: Полупрозрачный фон, зеленый текст
                btn.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
                btn.setTextColor(heatingGreen)
            }
        }

        tvBrightnessPercent.text = "$currentBrightness%"
        sbBrightness.progress = currentBrightness
    }

    private fun saveState() {
        prefs.edit().apply {
            putString("lightState", isLightOn)
            putInt("brightness", currentBrightness)
            putInt("r", currentR); putInt("g", currentG); putInt("b", currentB)
            putString("mode", currentMode)
            apply()
        }
    }

    private fun loadSavedState() {
        isLightOn = prefs.getString("lightState", "OFF") ?: "OFF"
        currentBrightness = prefs.getInt("brightness", 100)
        currentR = prefs.getInt("r", 255); currentG = prefs.getInt("g", 255); currentB = prefs.getInt("b", 255)
        currentMode = prefs.getString("mode", "none") ?: "none"
    }

    private fun sendCommand() {
        val topic = "makieta/oswietlenie/ustaw"
        val payload = """{
        "target":"wew",
        "state":"$isLightOn",
        "mode":"$currentMode",
        "brightness":$currentBrightness,
        "color":{"r":$currentR,"g":$currentG,"b":$currentB}
    }""".trimIndent()
        mqttHandler.publish(topic, payload)
    }
}