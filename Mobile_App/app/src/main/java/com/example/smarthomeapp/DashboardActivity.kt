package com.example.smarthomeapp

import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView
import org.json.JSONObject

class DashboardActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler

    // Pola klimatu (Tylko te, które faktycznie mają ID w XML!)
    private lateinit var tvDashboardTemp: TextView
    private lateinit var tvDashboardTempOut: TextView
    private lateinit var tvDashboardHum: TextView
    private lateinit var tvDashboardHumOut: TextView
    private lateinit var tvDashboardLight: TextView // Tylko zewnątrz
    private lateinit var tvDashboardPress: TextView // Tylko wewnątrz

    // Pola statusu systemów
    private lateinit var statusLight: TextView
    private lateinit var statusHeating: TextView
    private lateinit var statusVent: TextView
    private lateinit var statusSecurity: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        // Elementy nawigacji
        val themeBtn = findViewById<ImageView>(R.id.ivThemeToggle)
        val ivLogout = findViewById<ImageView>(R.id.ivLogout)
        val ivCloud = findViewById<ImageView>(R.id.ivConnectionStatus)

        // Inicjalizacja pól klimatu
        tvDashboardTemp = findViewById(R.id.tvDashboardTemp)
        tvDashboardTempOut = findViewById(R.id.tvDashboardTempOut)
        tvDashboardHum = findViewById(R.id.tvDashboardHum)
        tvDashboardHumOut = findViewById(R.id.tvDashboardHumOut)
        tvDashboardLight = findViewById(R.id.tvDashboardLight) // Zewnątrz
        tvDashboardPress = findViewById(R.id.tvDashboardPress) // Wewnątrz

        // Inicjalizacja statusów systemów
        statusLight = findViewById(R.id.statusLight)
        statusHeating = findViewById(R.id.statusHeating)
        statusVent = findViewById(R.id.statusVent)
        statusSecurity = findViewById(R.id.statusSecurity)

        // Przyciski menu
        val btnHeating = findViewById<CardView>(R.id.btnHeating)
        val btnVentilation = findViewById<CardView>(R.id.btnVentilation)
        val btnSensors = findViewById<CardView>(R.id.btnWater)
        val btnAccess = findViewById<CardView>(R.id.btnAccess)
        val btnLighting = findViewById<CardView>(R.id.btnLighting)
        val btnBlinds = findViewById<CardView>(R.id.btnBlinds)

        // Inicjalizacja MQTT
        mqttHandler = MqttHandler(this)

        mqttHandler.connect(
            onConnected = {
                runOnUiThread {
                    ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
                }
                mqttHandler.subscribe("makieta/czujniki/srodowisko/#")
                mqttHandler.subscribe("makieta/czujniki/swiatlo")
                mqttHandler.subscribe("makieta/status/systemy")
            },
            onMessage = { msg ->
                runOnUiThread {
                    parseDashboardData(msg)
                }
            }
        )

        // --- NAWIGACJA ---
        btnSensors.setOnClickListener { startActivity(Intent(this, SensorsActivity::class.java)) }
        btnAccess.setOnClickListener { startActivity(Intent(this, AccessActivity::class.java)) }
        btnLighting.setOnClickListener { startActivity(Intent(this, LightingActivity::class.java)) }
        btnVentilation.setOnClickListener { startActivity(Intent(this, VentilationActivity::class.java)) }
        btnHeating.setOnClickListener { startActivity(Intent(this, HeatingActivity::class.java)) }
        btnBlinds.setOnClickListener { startActivity(Intent(this, BlindsActivity::class.java)) }

        ivLogout.setOnClickListener {
            startActivity(Intent(this, MainActivity::class.java))
            finish()
        }

        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        loadCachedSystemStatuses()
    }

    private fun loadCachedSystemStatuses() {
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)

        val lightState = prefs.getString("status_light", "OFF") ?: "OFF"
        val heatingState = prefs.getString("status_heating", "OFF") ?: "OFF"
        val ventState = prefs.getString("status_vent", "OFF") ?: "OFF"
        val securityState = prefs.getString("status_security", "DISARMED") ?: "DISARMED"

        updateSystemStatus(statusLight, lightState)
        updateSystemStatus(statusHeating, heatingState)
        updateSystemStatus(statusVent, ventState)

        statusSecurity.text = securityState
        if (securityState == "ARMED") {
            statusSecurity.setTextColor(Color.parseColor("#FF5252")) // Czerwony
        } else {
            statusSecurity.setTextColor(Color.parseColor("#808080")) // Szary
        }
    }

    private fun parseDashboardData(msg: String) {
        try {
            val cleanMsg = msg.trim()
            if (cleanMsg.startsWith("{")) {
                val json = JSONObject(cleanMsg)
                val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)

                // Wnętrze: Temperatura, Wilgotność, Ciśnienie
                if (json.has("indoor")) {
                    val indoor = json.getJSONObject("indoor")
                    if (indoor.has("temperature")) tvDashboardTemp.text = "${indoor.optString("temperature", "--")} °C"
                    if (indoor.has("humidity")) tvDashboardHum.text = "${indoor.optString("humidity", "--")} %"
                    if (indoor.has("pressure")) tvDashboardPress.text = "${indoor.optString("pressure", "--")} hPa"
                }

                // Zewnątrz: Temperatura, Wilgotność
                if (json.has("outdoor")) {
                    val outdoor = json.getJSONObject("outdoor")
                    if (outdoor.has("temperature")) tvDashboardTempOut.text = "${outdoor.optString("temperature", "--")} °C"
                    if (outdoor.has("humidity")) tvDashboardHumOut.text = "${outdoor.optString("humidity", "--")} %"
                }

                // Światło (tylko zewnątrz)
                if (json.has("lux")) {
                    val light = json.optString("lux", "--")
                    tvDashboardLight.text = "$light lx"
                }

                // Statusy Systemów
                if (json.has("status_light")) {
                    val state = json.getString("status_light")
                    prefs.edit().putString("status_light", state).apply()
                    updateSystemStatus(statusLight, state)
                }
                if (json.has("status_heating")) {
                    val state = json.getString("status_heating")
                    prefs.edit().putString("status_heating", state).apply()
                    updateSystemStatus(statusHeating, state)
                }
                if (json.has("status_vent")) {
                    val state = json.getString("status_vent")
                    prefs.edit().putString("status_vent", state).apply()
                    updateSystemStatus(statusVent, state)
                }
                if (json.has("status_security")) {
                    val state = json.getString("status_security")
                    prefs.edit().putString("status_security", state).apply()
                    statusSecurity.text = state
                    if (state == "ARMED") statusSecurity.setTextColor(Color.parseColor("#FF5252")) else statusSecurity.setTextColor(Color.parseColor("#808080"))
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun updateSystemStatus(textView: TextView, status: String) {
        textView.text = status
        if (status == "ON" || status == "RUNNING") {
            textView.setTextColor(Color.parseColor("#4CAF50")) // Zielony
        } else {
            textView.setTextColor(Color.parseColor("#808080")) // Szary
        }
    }
}