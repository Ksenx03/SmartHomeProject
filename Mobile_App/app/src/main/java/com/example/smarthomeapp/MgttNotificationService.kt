package com.example.smarthomeapp

import android.app.*
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.media.AudioAttributes
import android.media.RingtoneManager
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import java.text.SimpleDateFormat
import java.util.*

class MqttNotificationService : Service() {

    private lateinit var mqttHandler: MqttHandler
    // Zmieniamy ID kanału, aby wymusić odświeżenie ustawień w systemie Android
    private val CHANNEL_ID = "SecurityChannel_v3"

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        createNotificationChannel()

        // Powiadomienie stałe (Foreground Service)
        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Makieta Security")
            .setContentText("System jest aktywny")
            .setSmallIcon(R.drawable.ic_access_lock)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()

        startForeground(1, notification)

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = { Log.d("SERVICE", "Połączono z MQTT") },
            onMessage = { msg ->
                val cleanMsg = msg.trim()
                saveSecurityState(cleanMsg)
                saveLogToHistory(cleanMsg)

                if (cleanMsg == "ALARM" || cleanMsg == "DENIED") {
                    sendPushNotification(cleanMsg)
                }
            }
        )

        return START_STICKY
    }

    private fun sendPushNotification(type: String) {
        val title = if (type == "ALARM") "🚨 WŁAMANIE!" else "⚠️ INTRIUZ!"
        val text = "Wykryto ruch! Sprawdź aplikację natychmiast."

        val intent = Intent(this, AccessActivity::class.java)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK

        // Unikalny requestCode dla każdego kliknięcia
        val pIntent = PendingIntent.getActivity(
            this,
            System.currentTimeMillis().toInt(),
            intent,
            PendingIntent.FLAG_IMMUTABLE
        )

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_access_lock)
            .setContentTitle(title)
            .setContentText(text)
            .setPriority(NotificationCompat.PRIORITY_MAX) // Maksymalny priorytet UI
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .setDefaults(Notification.DEFAULT_ALL)
            .setFullScreenIntent(pIntent, true) // To wymusza wyskakujące okno (Heads-up)
            .setAutoCancel(true)
            .setVibrate(longArrayOf(0, 500, 200, 500))
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)

        val manager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

        // Wysyłamy z UNIKALNYM ID (używamy milisekund)
        val notificationId = (System.currentTimeMillis() % 1000000).toInt()
        manager.notify(notificationId, builder.build())

        Log.d("SERVICE", "Wysłano alert o ID: $notificationId")
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val name = "Alerty bezpieczeństwa"
            val importance = NotificationManager.IMPORTANCE_HIGH
            val channel = NotificationChannel(CHANNEL_ID, name, importance).apply {
                description = "Krytyczne powiadomienia o włamaniu"
                enableLights(true)
                lightColor = Color.RED
                enableVibration(true)
                vibrationPattern = longArrayOf(0, 500, 200, 500)
                setBypassDnd(true) // Przebijanie trybu "Nie przeszkadzać"
                lockscreenVisibility = Notification.VISIBILITY_PUBLIC
            }

            val manager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            manager.createNotificationChannel(channel)
        }
    }

    // Metody zapisu (zostają bez zmian, bo działają dobrze)
    private fun saveSecurityState(status: String) {
        val prefs = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE).edit()
        when (status) {
            "ARMED" -> { prefs.putBoolean("isArmed", true); prefs.putBoolean("isAlarmActive", false) }
            "DISARMED" -> { prefs.putBoolean("isArmed", false); prefs.putBoolean("isAlarmActive", false) }
            "ALARM", "DENIED" -> { prefs.putBoolean("isAlarmActive", true) }
        }
        prefs.apply()
    }

    private fun saveLogToHistory(msg: String) {
        val prefs = getSharedPreferences("SecurityPrefs", Context.MODE_PRIVATE)
        val currentLogs = prefs.getString("logHistory", "") ?: ""
        val time = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        val logEntry = when(msg) {
            "ARMED" -> "[$time] System: UZBROJONY"
            "DISARMED" -> "[$time] System: ROZBROJONY"
            "ALARM" -> "[$time] !!! WŁAMANIE !!!"
            "DENIED" -> "[$time] !!! NIEZNANA KARTA !!!"
            "62CB0951" -> "[$time] Karta: OK"
            else -> "[$time] Dane: $msg"
        }
        val logList = currentLogs.split("\n").toMutableList()
        logList.add(0, logEntry)
        val limitedLogs = logList.take(30).joinToString("\n")
        prefs.edit().putString("logHistory", limitedLogs).apply()
    }

    override fun onBind(intent: Intent?): IBinder? = null
}