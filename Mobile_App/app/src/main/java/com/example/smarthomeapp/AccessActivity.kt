package com.example.smarthomeapp

import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity

class AccessActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private var isArmed = false
    private var isAlarmActive = false

    private lateinit var ivShield: ImageView
    private lateinit var tvSecurityText: TextView
    private lateinit var btnToggle: Button
    private lateinit var logsContainer: LinearLayout

    private val heatingGreen = Color.parseColor("#00695C")
    private val buttonBgInactive = Color.parseColor("#4D00695C")
    private val alarmRed = Color.parseColor("#B71C1C")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_access)

        ivShield = findViewById(R.id.ivSecurityStatus)
        tvSecurityText = findViewById(R.id.tvSecurityText)
        btnToggle = findViewById(R.id.btnToggleSecurity)
        logsContainer = findViewById(R.id.logsContainer)

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }
        val btnUnlock = findViewById<Button>(R.id.btnUnlock)

        loadSecurityState()
        displayHistory()
        updateUIState()

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = { },
            onMessage = { msg ->
                runOnUiThread {
                    handleMqtt(msg)
                    displayHistory()
                }
            }
        )

        btnUnlock.setOnClickListener {
            mqttHandler.publish("makieta/access/ustaw", "unlock")
            openLockVisual() // Запускаем анимацию открытия
        }

        btnToggle.setOnClickListener {
            val cmd = if (isAlarmActive || isArmed) "DISARM" else "ARM"
            mqttHandler.publish("makieta/access/ustaw", cmd)
        }

        logsContainer.setOnLongClickListener {
            getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE).edit()
                .putString("logHistory", "").apply()
            displayHistory()
            true
        }
    }

    private fun handleMqtt(msg: String) {
        val cleanMsg = msg.trim()
        when (cleanMsg) {
            "ARMED" -> { isArmed = true; isAlarmActive = false }
            "DISARMED" -> { isArmed = false; isAlarmActive = false }
            "ALARM", "DENIED" -> { isAlarmActive = true }
            "62CB0951" -> { openLockVisual() } // Если приложили твою карту
        }
        updateUIState()
    }

    private fun openLockVisual() {
        // Меняем картинку на открытый замок
        ivShield.setImageResource(R.drawable.ic_access_open)
        ivShield.setColorFilter(heatingGreen)

        // Через 5 секунд возвращаем закрытый замок
        ivShield.postDelayed({
            ivShield.setImageResource(R.drawable.ic_access_lock)
            updateUIState() // Возвращаем актуальный цвет (красный или зеленый)
        }, 5000)
    }

    private fun updateUIState() {
        // Если замок сейчас в процессе "открытия" (визуально), не перекрашиваем его сразу
        when {
            isAlarmActive -> {
                ivShield.setColorFilter(Color.RED)
                tvSecurityText.text = "ALARM ACTIVE!"
                tvSecurityText.setTextColor(Color.RED)
                btnToggle.text = "STOP"
                btnToggle.backgroundTintList = ColorStateList.valueOf(Color.RED)
                btnToggle.setTextColor(Color.WHITE)
            }
            isArmed -> {
                ivShield.setColorFilter(alarmRed)
                tvSecurityText.text = "ARMED"
                tvSecurityText.setTextColor(alarmRed)
                btnToggle.text = "DISARM"
                btnToggle.backgroundTintList = ColorStateList.valueOf(alarmRed)
                btnToggle.setTextColor(Color.WHITE)
            }
            else -> {
                ivShield.setColorFilter(heatingGreen)
                tvSecurityText.text = "SECURE"
                tvSecurityText.setTextColor(heatingGreen)
                btnToggle.text = "ARM"
                btnToggle.backgroundTintList = ColorStateList.valueOf(buttonBgInactive)
                btnToggle.setTextColor(heatingGreen)
            }
        }
    }

    private fun displayHistory() {
        logsContainer.removeAllViews()
        val history = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE)
            .getString("logHistory", "") ?: ""

        if (history.isEmpty()) return

        history.split("\n").forEach { logLine ->
            val tv = TextView(this).apply {
                text = logLine
                textSize = 14f
                setPadding(0, 8, 0, 8)
                setTextColor(if (logLine.contains("!!!")) Color.RED else heatingGreen)
            }
            logsContainer.addView(tv)
        }
    }

    private fun loadSecurityState() {
        val prefs = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE)
        isArmed = prefs.getBoolean("isArmed", false)
        isAlarmActive = prefs.getBoolean("isAlarmActive", false)
    }
}