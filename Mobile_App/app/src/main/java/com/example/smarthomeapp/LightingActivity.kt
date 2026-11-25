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
import com.google.android.material.switchmaterial.SwitchMaterial
import com.google.android.material.tabs.TabLayout
import org.json.JSONObject
import java.util.Locale

class LightingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var prefs: SharedPreferences

    private lateinit var ivLightButton: ImageView
    private lateinit var tvStatus: TextView
    private lateinit var tvBrightnessPercent: TextView
    private lateinit var sbBrightness: SeekBar
    private lateinit var modeButtons: List<Button>
    private lateinit var tabLayout: TabLayout

    // Widoki z nowego activity_lighting.xml
    private lateinit var layoutAutoMode: View
    private lateinit var layoutIndoorEffects: View
    private lateinit var switchAutoMode: SwitchMaterial
    private lateinit var tvZoneSubtitle: TextView
    private lateinit var tvLightSensorLux: TextView
    private lateinit var tvDayNightBadge: TextView

    private val heatingGreen = Color.parseColor("#00695C")
    private val buttonBgInactive = Color.parseColor("#4D00695C")

    private var currentTarget = "wew"

    private var isLightOn = "OFF"
    private var currentBrightness = 100
    private var currentR = 255
    private var currentG = 255
    private var currentB = 255
    private var currentMode = "none"
    private var isAutoNightEnabled = true

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

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        ivLightButton = findViewById(R.id.ivLightButton)
        tvStatus = findViewById(R.id.tvLightStatus)
        tvBrightnessPercent = findViewById(R.id.tvBrightnessPercent)
        sbBrightness = findViewById(R.id.brightnessSeekBar)
        tabLayout = findViewById(R.id.tabLightTarget)

        // Bezpieczne bindowanie widoków (CardView/LinearLayout jako View - zero ClassCastException)
        layoutAutoMode = findViewById(R.id.layoutAutoMode)
        layoutIndoorEffects = findViewById(R.id.layoutIndoorEffects)
        switchAutoMode = findViewById(R.id.switchAutoMode)
        tvZoneSubtitle = findViewById(R.id.tvZoneSubtitle)
        tvLightSensorLux = findViewById(R.id.tvLightSensorLux)
        tvDayNightBadge = findViewById(R.id.tvDayNightBadge)

        modeButtons = listOf(
            findViewById(R.id.btnDisco),
            findViewById(R.id.btnRelax),
            findViewById(R.id.btnStrobe)
        )

        loadSavedState("wew")
        loadCachedLux()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe("makieta/czujniki/swiatlo")
                mqttHandler.subscribe("makieta/status/systemy")
            },
            onMessage = { msg ->
                runOnUiThread {
                    handleIncomingMqtt(msg)
                }
            }
        )

        setupTargetTabs()
        setupMainLightButton()
        setupBrightnessControl()
        setupColorGrid()
        setupSpecialModes()
        setupAutoModeSwitch()

        updateUIState()
    }

    private fun loadCachedLux() {
        val cachedLux = prefs.getFloat("cached_lux", -1f)
        if (cachedLux >= 0f) {
            updateLuxDisplay(cachedLux.toDouble())
        }
    }

    private fun updateLuxDisplay(lux: Double) {
        tvLightSensorLux.text = String.format(Locale.US, "%.2f lx", lux)
        if (lux < 100.0) {
            tvDayNightBadge.text = "\uD83C\uDF19 NIGHT"
        } else {
            tvDayNightBadge.text = "\u2600\uFE0F DAY"
        }
    }

    private fun handleIncomingMqtt(msg: String) {
        try {
            val clean = msg.trim()
            if (!clean.startsWith("{")) return
            val json = JSONObject(clean)

            if (json.has("lux")) {
                val lux = json.optDouble("lux", 0.0)
                prefs.edit().putFloat("cached_lux", lux.toFloat()).apply()
                updateLuxDisplay(lux)
            }

            if (json.has("status_auto_light")) {
                isAutoNightEnabled = (json.optString("status_auto_light", "ON") == "ON")
                prefs.edit().putBoolean("zew_auto_night", isAutoNightEnabled).apply()
                if (currentTarget == "zew") {
                    switchAutoMode.isChecked = isAutoNightEnabled
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
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
            if (isLightOn == "ON" && currentBrightness == 0) currentBrightness = 100

            saveState(currentTarget)
            updateUIState()
            sendCommand()
        }
    }

    private fun setupBrightnessControl() {
        sbBrightness.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) {
                currentBrightness = p
                tvBrightnessPercent.text = "$p%"
                if (fromUser) {
                    if (p > 0 && isLightOn == "OFF") isLightOn = "ON"
                    if (p == 0) isLightOn = "OFF"
                    updateBulbIconVisual()
                }
            }
            override fun onStartTrackingTouch(s: SeekBar?) {}
            override fun onStopTrackingTouch(s: SeekBar?) {
                saveState(currentTarget)
                updateUIState()
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
            val params = GridLayout.LayoutParams().apply {
                width = size
                height = size
                setMargins(margin, margin, margin, margin)
            }
            colorView.layoutParams = params

            val circleDrawable = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.parseColor(colorHex))
                setStroke((1.5f * density).toInt(), Color.parseColor("#3300695C"))
            }
            colorView.background = circleDrawable

            colorView.setOnClickListener {
                currentMode = "none"
                isLightOn = "ON"
                if (currentBrightness == 0) currentBrightness = 100

                val color = Color.parseColor(colorHex)
                currentR = Color.red(color)
                currentG = Color.green(color)
                currentB = Color.blue(color)

                saveState(currentTarget)
                updateUIState()
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
                val clickedMode = btn.text.toString().lowercase(Locale.ROOT)
                currentMode = if (currentMode == clickedMode) "none" else clickedMode
                if (currentMode != "none") {
                    isLightOn = "ON"
                    if (currentBrightness == 0) currentBrightness = 100
                }

                saveState(currentTarget)
                updateUIState()
                sendCommand()
            }
        }
    }

    private fun setupAutoModeSwitch() {
        switchAutoMode.setOnCheckedChangeListener { buttonView, isChecked ->
            if (buttonView.isPressed) {
                isAutoNightEnabled = isChecked
                currentMode = if (isChecked) "auto" else "none"
                prefs.edit().putBoolean("zew_auto_night", isChecked).apply()
                saveState(currentTarget)
                updateUIState()

                val autoPayload = if (isChecked) "ON" else "OFF"
                mqttHandler.publish("makieta/oswietlenie/automatyka", autoPayload, retained = true)
            }
        }
    }

    private fun updateBulbIconVisual() {
        if (isLightOn == "ON" && currentBrightness > 0) {
            val activeColor = Color.rgb(currentR, currentG, currentB)
            val normalizedAlpha = 0.25f + (currentBrightness.coerceIn(0, 100) / 100f) * 0.75f

            ivLightButton.setColorFilter(activeColor)
            ivLightButton.imageAlpha = (normalizedAlpha * 255).toInt()

            val glowAlpha = (30 + (currentBrightness.coerceIn(0, 100) * 1.1f)).toInt().coerceIn(30, 140)
            val glowColor = Color.argb(glowAlpha, Color.red(activeColor), Color.green(activeColor), Color.blue(activeColor))
            ivLightButton.backgroundTintList = ColorStateList.valueOf(glowColor)
        } else {
            ivLightButton.setColorFilter(Color.parseColor("#6600695C"))
            ivLightButton.imageAlpha = 120
            ivLightButton.backgroundTintList = ColorStateList.valueOf(Color.parseColor("#1A00695C"))
        }
    }

    private fun updateUIState() {
        if (currentTarget == "zew") {
            tvZoneSubtitle.text = "Strefa: Oświetlenie elewacji (LED Zew)"
            layoutIndoorEffects.visibility = View.GONE
            layoutAutoMode.visibility = View.VISIBLE
            switchAutoMode.isChecked = isAutoNightEnabled
        } else {
            tvZoneSubtitle.text = "Strefa: Oświetlenie salonu (LED Wew)"
            layoutIndoorEffects.visibility = View.VISIBLE
            layoutAutoMode.visibility = View.GONE
        }

        tvStatus.text = when {
            currentTarget == "zew" && isAutoNightEnabled && isLightOn == "OFF" -> "AUTO (STANDBY)"
            currentTarget == "zew" && isAutoNightEnabled && isLightOn == "ON" -> "AUTO ($currentBrightness%)"
            isLightOn == "ON" -> "ON ($currentBrightness%)"
            else -> "OFF"
        }

        if (isLightOn == "ON" && currentBrightness > 0) {
            tvStatus.setTextColor(heatingGreen)
        } else {
            tvStatus.setTextColor(Color.GRAY)
        }

        updateBulbIconVisual()

        modeButtons.forEach { btn ->
            val btnMode = btn.text.toString().lowercase(Locale.ROOT)
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
        val otherTarget = if (target == "wew") "zew" else "wew"
        val otherState = prefs.getString("${otherTarget}_lightState", "OFF") ?: "OFF"
        val overallStatus = if (isLightOn == "ON" || otherState == "ON") "ON" else "OFF"

        prefs.edit().apply {
            putString("${target}_lightState", isLightOn)
            putInt("${target}_brightness", currentBrightness)
            putInt("${target}_r", currentR)
            putInt("${target}_g", currentG)
            putInt("${target}_b", currentB)
            putString("${target}_mode", currentMode)
            putString("status_light", overallStatus)
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
        isAutoNightEnabled = prefs.getBoolean("zew_auto_night", true)
    }

    private fun sendCommand() {
        val topic = "makieta/oswietlenie/ustaw"
        val payload = """{"target":"$currentTarget","state":"$isLightOn","mode":"$currentMode","brightness":$currentBrightness,"color":{"r":$currentR,"g":$currentG,"b":$currentB}}"""
        mqttHandler.publish(topic, payload, retained = true)
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttHandler.disconnect()
    }
}