package com.example.smarthomeapp

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.NotificationCompat
import java.text.SimpleDateFormat
import java.util.*

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

        // ИСПРАВЛЕНО: Теперь при успешном подключении принудительно вызывается подписка
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe("makieta/access/status")
            },
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
        val timeStamp = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        var logEntry = ""

        when (cleanMsg) {
            "ARMED" -> {
                isArmed = true
                isAlarmActive = false
                logEntry = "[$timeStamp] System ARMED"
            }
            "DISARMED" -> {
                isArmed = false
                isAlarmActive = false
                logEntry = "[$timeStamp] System DISARMED"
            }
            "ALARM" -> {
                isAlarmActive = true
                logEntry = "[$timeStamp] !!! ALARM TRIGGERED !!!"
                showNotification("ALERT!", "Wykryto nieautoryzowany ruch!")
            }
            "DENIED" -> {
                isAlarmActive = true
                logEntry = "[$timeStamp] !!! ACCESS DENIED !!!"
                showNotification("Odmowa dostępu", "Użyto nieznanej karty RFID!")
            }
            "62CB0951" -> {
                logEntry = "[$timeStamp] Access Granted: Kseniya"
                openLockVisual()
            }
        }

        // Сохраняем состояние в историю логов, если пришло важное событие
        if (logEntry.isNotEmpty()) {
            saveLogToHistory(logEntry)
        }
        saveSecurityState()
        updateUIState()
    }

    private fun openLockVisual() {
        ivShield.setImageResource(R.drawable.ic_access_open)
        ivShield.setColorFilter(heatingGreen)

        ivShield.postDelayed({
            ivShield.setImageResource(R.drawable.ic_access_lock)
            updateUIState()
        }, 5000)
    }

    private fun updateUIState() {
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
            if (logLine.trim().isNotEmpty()) {
                val tv = TextView(this).apply {
                    text = logLine
                    textSize = 14f
                    setPadding(0, 8, 0, 8)
                    setTextColor(if (logLine.contains("!!!")) Color.RED else heatingGreen)
                }
                logsContainer.addView(tv)
            }
        }
    }

    private fun saveLogToHistory(entry: String) {
        val prefs = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE)
        val currentHistory = prefs.getString("logHistory", "") ?: ""
        val newHistory = if (currentHistory.isEmpty()) entry else "$entry\n$currentHistory"
        prefs.edit().putString("logHistory", newHistory).apply()
    }

    private fun loadSecurityState() {
        val prefs = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE)
        isArmed = prefs.getBoolean("isArmed", false)
        isAlarmActive = prefs.getBoolean("isAlarmActive", false)
    }

    private fun saveSecurityState() {
        getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE).edit()
            .putBoolean("isArmed", isArmed)
            .putBoolean("isAlarmActive", isAlarmActive)
            .apply()
    }

    private fun showNotification(title: String, message: String) {
        val channelId = "SecurityChannel_v3"
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                channelId,
                "Makieta Security Alerts",
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "Alerts regarding security events like unauthorized movement"
                enableLights(true)
                lightColor = Color.RED
                enableVibration(true)
                lockscreenVisibility = NotificationCompat.VISIBILITY_PUBLIC
            }
            notificationManager.createNotificationChannel(channel)
        }

        val fullScreenIntent = android.content.Intent(this, AccessActivity::class.java)
        val fullScreenPendingIntent = android.app.PendingIntent.getActivity(
            this,
            0,
            fullScreenIntent,
            android.app.PendingIntent.FLAG_IMMUTABLE or android.app.PendingIntent.FLAG_UPDATE_CURRENT
        )

        val notification = NotificationCompat.Builder(this, channelId)
            .setSmallIcon(android.R.drawable.ic_lock_idle_lock)
            .setContentTitle(title)
            .setContentText(message)
            .setPriority(NotificationCompat.PRIORITY_MAX)
            .setCategory(NotificationCompat.CATEGORY_ALARM) // Указываем категорию тревоги/будильника
            .setDefaults(NotificationCompat.DEFAULT_ALL)
            .setFullScreenIntent(fullScreenPendingIntent, true) // Принудительный Heads-Up
            .setAutoCancel(true)
            .build()

        notificationManager.notify(System.currentTimeMillis().toInt(), notification)
    }
}