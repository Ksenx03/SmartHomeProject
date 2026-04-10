package com.example.smarthomeapp

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView

class DashboardActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_dashboard)

        // UI Components
        val themeBtn: ImageView = findViewById(R.id.ivThemeToggle)
        val ivLogout: ImageView = findViewById(R.id.ivLogout)
        val ivCloud: ImageView = findViewById(R.id.ivConnectionStatus)

        // Navigation Cards
        val btnHeating: CardView = findViewById(R.id.btnHeating)
        val btnVentilation: CardView = findViewById(R.id.btnVentilation)
        val btnWater: CardView = findViewById(R.id.btnWater)
        val btnAccess: CardView = findViewById(R.id.btnAccess)
        val btnLighting: CardView = findViewById(R.id.btnLighting)
        val btnBlinds: CardView = findViewById(R.id.btnBlinds)

        // Find the specific bulb icon inside the Lighting CardView
        // Replace R.id.ivLightingIcon with the actual ID from your activity_dashboard.xml
        val ivLightingIcon: ImageView = findViewById(R.id.ivLighting)
        ivLightingIcon.setColorFilter(Color.WHITE)

        // Initialize MQTT connection
        mqttHandler = MqttHandler(this)
        mqttHandler.connect {
            // Connection success: Update cloud status to Green 🟢
            runOnUiThread {
                ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
            }
        }

        // Theme Toggle Logic
        val currentMode = AppCompatDelegate.getDefaultNightMode()
        if (currentMode == AppCompatDelegate.MODE_NIGHT_YES) {
            themeBtn.setImageResource(R.drawable.ic_theme_dark)
        } else {
            themeBtn.setImageResource(R.drawable.ic_theme_light)
        }

        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }

        // Logout Logic
        ivLogout.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
            startActivity(intent)
            finish()
        }

        // Navigation Setup
        btnHeating.setOnClickListener { startActivity(Intent(this, HeatingActivity::class.java)) }
        btnVentilation.setOnClickListener { startActivity(Intent(this, VentilationActivity::class.java)) }
        btnWater.setOnClickListener { startActivity(Intent(this, WaterSensorActivity::class.java)) }
        btnAccess.setOnClickListener { startActivity(Intent(this, AccessActivity::class.java)) }
        btnLighting.setOnClickListener { startActivity(Intent(this, LightingActivity::class.java)) }
        btnBlinds.setOnClickListener { startActivity(Intent(this, BlindsActivity::class.java)) }
    }

    /**
     * Set lighting icon color based on current theme:
     * Light theme -> Semi-transparent Green
     * Dark theme -> Semi-transparent White
     */
    private fun applyLightingIconTint(icon: ImageView) {
        val currentMode = AppCompatDelegate.getDefaultNightMode()
        if (currentMode == AppCompatDelegate.MODE_NIGHT_YES) {
            // Dark theme: White with ~60% alpha (#99FFFFFF)
            icon.setColorFilter(Color.parseColor("#99FFFFFF"))
        } else {
            // Light theme: Green with ~60% alpha (#994CAF50)
            icon.setColorFilter(Color.parseColor("#994CAF50"))
        }
    }
}