package com.example.smarthomeapp

import android.content.Context
import android.content.SharedPreferences
import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject
import java.util.Locale

class HeatingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private lateinit var prefs: SharedPreferences

    // Tematy MQTT zgodne z projekt.ino na ESP32
    private val topicSetState = "makieta/sterowanie/ogrzewanie/stan"
    private val topicSetTemp = "makieta/sterowanie/ogrzewanie/temperatura"

    // Elementy UI
    private lateinit var ivHeatingIcon: ImageView
    private lateinit var tvHeatingStatus: TextView
    private lateinit var tvCurrentTemp: TextView
    private lateinit var tvTargetTemp: TextView
    private lateinit var sbTemperature: SeekBar

    private lateinit var btnModeAuto: Button
    private lateinit var btnModeManual: Button
    private lateinit var btnEco: Button
    private lateinit var btnComfort: Button
    private lateinit var btnBoost: Button
    private lateinit var presetButtons: List<Button>

    // Kolory
    private val mainTeal = Color.parseColor("#00695C")
    private val buttonBgInactive = Color.parseColor("#4D00695C")
    private val inactiveGray = Color.parseColor("#808080")
    private val circleBgActive = Color.parseColor("#2600695C")
    private val circleBgInactive = Color.parseColor("#12000000")

    // Zmienne stanu
    private var isHeatingOn = "OFF"       // "ON" lub "OFF"
    private var controlMode = "auto"      // "auto" lub "manual"
    private var currentPreset = "none"    // "eco", "comfort", "boost", "none"
    private var targetTemp = 22
    private var currentRoomTemp = 21.0

    // Znacznik czasu ostatniej akcji użytkownika (chroni przed nadpisaniem przez stary pakiet MQTT)
    private var lastUserInteractionTime = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_heating)

        prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        loadSavedState()

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        ivHeatingIcon = findViewById(R.id.ivHeatingIcon)
        tvHeatingStatus = findViewById(R.id.tvHeatingStatus)
        tvCurrentTemp = findViewById(R.id.tvCurrentTemp)
        tvTargetTemp = findViewById(R.id.tvTargetTemp)
        sbTemperature = findViewById(R.id.sbTemperature)

        btnModeAuto = findViewById(R.id.btnModeAuto)
        btnModeManual = findViewById(R.id.btnModeManual)
        btnEco = findViewById(R.id.btnEco)
        btnComfort = findViewById(R.id.btnComfort)
        btnBoost = findViewById(R.id.btnBoost)

        presetButtons = listOf(btnEco, btnComfort, btnBoost)

        tvCurrentTemp.text = String.format(Locale.US, "%.2f \u00B0C", currentRoomTemp)

        setupModeSwitchButtons()
        setupCampfireClick()
        setupTemperatureSeekBar()
        setupPresetButtons()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe("makieta/czujniki/srodowisko/#")
                mqttHandler.subscribe("makieta/status/systemy")
            },
            onMessage = { msg ->
                runOnUiThread {
                    handleIncomingMqtt(msg)
                }
            }
        )

        if (controlMode == "auto") {
            evaluateAutoHeatingState()
        }
        updateUIState()
    }

    override fun onResume() {
        super.onResume()
        loadSavedState()
        updateUIState()
    }

    // 1. Przełącznik trybu AUTO / MANUAL
    private fun setupModeSwitchButtons() {
        btnModeAuto.setOnClickListener {
            lastUserInteractionTime = System.currentTimeMillis()
            controlMode = "auto"
            evaluateAutoHeatingState()
            saveState()
            updateUIState()
            sendHeatingCommandsToEsp()
        }

        btnModeManual.setOnClickListener {
            lastUserInteractionTime = System.currentTimeMillis()
            if (controlMode != "manual") {
                controlMode = "manual"
                currentPreset = "none"
                // Kliknięcie MANUAL od razu włącza grzanie ręczne (chyba że już było ON)
                isHeatingOn = "ON"
            } else {
                // Ponowne kliknięcie MANUAL przełącza ON / OFF
                isHeatingOn = if (isHeatingOn == "ON") "OFF" else "ON"
            }
            saveState()
            updateUIState()
            sendHeatingCommandsToEsp()
        }
    }

    // 2. Kliknięcie w ikonę ogniska (ręczne włączenie/wyłączenie grzania)
    private fun setupCampfireClick() {
        ivHeatingIcon.setOnClickListener {
            lastUserInteractionTime = System.currentTimeMillis()
            controlMode = "manual"
            currentPreset = "none"
            isHeatingOn = if (isHeatingOn == "ON") "OFF" else "ON"

            saveState()
            updateUIState()
            sendHeatingCommandsToEsp()
        }
    }

    // 3. Suwak temperatury docelowej
    private fun setupTemperatureSeekBar() {
        sbTemperature.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                targetTemp = progress
                tvTargetTemp.text = "${progress}\u00B0C"
                if (fromUser) {
                    lastUserInteractionTime = System.currentTimeMillis()
                    currentPreset = "none"
                    if (controlMode == "auto") {
                        evaluateAutoHeatingState()
                    }
                    updateUIState()
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                lastUserInteractionTime = System.currentTimeMillis()
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                lastUserInteractionTime = System.currentTimeMillis()
                saveState()
                sendHeatingCommandsToEsp()
            }
        })
    }

    // 4. Presety: Eco (18°C), Comfort (22°C), Boost (26°C)
    private fun setupPresetButtons() {
        btnEco.setOnClickListener { applyPreset("eco", 18) }
        btnComfort.setOnClickListener { applyPreset("comfort", 22) }
        btnBoost.setOnClickListener { applyPreset("boost", 26) }
    }

    private fun applyPreset(presetName: String, tempValue: Int) {
        lastUserInteractionTime = System.currentTimeMillis()
        if (currentPreset == presetName) {
            currentPreset = "none"
        } else {
            currentPreset = presetName
            targetTemp = tempValue
            // Wybór presetu temperatury aktywuje termostat AUTO (zgodnie z ClimateManager na ESP32)
            controlMode = "auto"
            evaluateAutoHeatingState()
        }
        saveState()
        updateUIState()
        sendHeatingCommandsToEsp()
    }

    // Lokalna ocena stanu termostatu w trybie AUTO (z uwzględnieniem histerezy 0.5°C z ESP32)
    private fun evaluateAutoHeatingState() {
        if (controlMode == "auto") {
            if (currentRoomTemp < (targetTemp - 0.5)) {
                isHeatingOn = "ON"
            } else if (currentRoomTemp >= targetTemp) {
                isHeatingOn = "OFF"
            }
        }
    }

    private fun updateUIState() {
        sbTemperature.progress = targetTemp
        tvTargetTemp.text = "${targetTemp}\u00B0C"

        // A. Przyciski AUTO / MANUAL
        if (controlMode == "auto") {
            btnModeAuto.backgroundTintList = ColorStateList.valueOf(mainTeal)
            btnModeAuto.setTextColor(Color.WHITE)
            btnModeManual.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
            btnModeManual.setTextColor(mainTeal)
        } else {
            btnModeManual.backgroundTintList = ColorStateList.valueOf(mainTeal)
            btnModeManual.setTextColor(Color.WHITE)
            btnModeAuto.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
            btnModeAuto.setTextColor(mainTeal)
        }

        // B. Ikona ogniska (płonące vs zgaszone)
        if (isHeatingOn == "ON") {
            ivHeatingIcon.setImageResource(R.drawable.ic_campfire_on)
            ivHeatingIcon.setColorFilter(mainTeal)
            ivHeatingIcon.backgroundTintList = ColorStateList.valueOf(circleBgActive)

            tvHeatingStatus.text = if (controlMode == "auto") "AUTO: HEATING ON" else "MANUAL: HEATING ON"
            tvHeatingStatus.setTextColor(mainTeal)
        } else {
            ivHeatingIcon.setImageResource(R.drawable.ic_campfire_off)
            ivHeatingIcon.setColorFilter(inactiveGray)
            ivHeatingIcon.backgroundTintList = ColorStateList.valueOf(circleBgInactive)

            tvHeatingStatus.text = if (controlMode == "auto") "AUTO: STANDBY (OFF)" else "MANUAL: OFF"
            tvHeatingStatus.setTextColor(inactiveGray)
        }

        // C. Podświetlenie aktywnego presetu (Eco / Comfort / Boost)
        presetButtons.forEach { btn ->
            val name = btn.text.toString().lowercase(Locale.ROOT)
            if (currentPreset == name) {
                btn.backgroundTintList = ColorStateList.valueOf(mainTeal)
                btn.setTextColor(Color.WHITE)
            } else {
                btn.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
                btn.setTextColor(mainTeal)
            }
        }
    }

    // Odbiór danych z czujników oraz statusu z ESP32
    private fun handleIncomingMqtt(payload: String) {
        try {
            val cleanMsg = payload.trim()
            if (!cleanMsg.startsWith("{")) return

            val jsonObject = JSONObject(cleanMsg)

            // 1. Odczyt temperatury wewnętrznej z makieta/czujniki/srodowisko/wew
            if (jsonObject.has("indoor")) {
                val indoor = jsonObject.getJSONObject("indoor")
                if (indoor.has("temperature")) {
                    currentRoomTemp = indoor.optDouble("temperature", currentRoomTemp)
                    prefs.edit().putFloat("cached_indoor_temp", currentRoomTemp.toFloat()).apply()
                    tvCurrentTemp.text = String.format(Locale.US, "%.2f \u00B0C", currentRoomTemp)

                    if (controlMode == "auto") {
                        evaluateAutoHeatingState()
                        saveState()
                        updateUIState()
                    }
                }
            }

            // 2. Synchronizacja stanu przekaźnika z makieta/status/systemy (jeśli minęło >2.5s od kliknięcia)
            if (jsonObject.has("status_heating") && (System.currentTimeMillis() - lastUserInteractionTime > 2500)) {
                val remoteHeatingState = jsonObject.getString("status_heating")
                if (isHeatingOn != remoteHeatingState) {
                    isHeatingOn = remoteHeatingState
                    saveState()
                    updateUIState()
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun saveState() {
        prefs.edit().apply {
            putString("status_heating", isHeatingOn)
            putString("heating_control_mode", controlMode)
            putString("heating_preset", currentPreset)
            putInt("heating_target_temp", targetTemp)
            putFloat("cached_indoor_temp", currentRoomTemp.toFloat())
            putLong("last_heating_cmd_time", lastUserInteractionTime)
            apply()
        }
    }

    private fun loadSavedState() {
        isHeatingOn = prefs.getString("status_heating", "OFF") ?: "OFF"
        controlMode = prefs.getString("heating_control_mode", "auto") ?: "auto"
        currentPreset = prefs.getString("heating_preset", "none") ?: "none"
        targetTemp = prefs.getInt("heating_target_temp", 22)
        currentRoomTemp = prefs.getFloat("cached_indoor_temp", 21.0f).toDouble()
    }

    // Wysyłanie komend na dokładne tematy zdefiniowane w projekt.ino na ESP32
    private fun sendHeatingCommandsToEsp() {
        if (controlMode == "auto") {
            mqttHandler.publish(topicSetTemp, targetTemp.toString(), retained = true)
            mqttHandler.publish(topicSetState, "AUTO", retained = true)
        } else {
            // W trybie MANUAL najpierw aktualizujemy temperaturę, a na końcu wysyłamy ON/OFF,
            // aby ClimateManager::setActive() wyłączył flagę autoMode na ESP32
            mqttHandler.publish(topicSetTemp, targetTemp.toString(), retained = true)
            mqttHandler.publish(topicSetState, isHeatingOn, retained = true)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttHandler.disconnect()
    }
}