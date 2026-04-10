package com.example.smarthomeapp

import android.content.Context
import android.util.Log
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

class MqttHandler(context: Context) {
    private val serverUri = "tcp://172.20.10.3:1883"
    private val clientId = "Xiaomi_Client_" + System.currentTimeMillis() // Уникальный ID, чтобы Малинка не выкидывала

    private val client = MqttAsyncClient(serverUri, clientId, MemoryPersistence())

    fun connect(onConnected: () -> Unit) {
        if (client.isConnected) {
            onConnected()
            return
        }

        val options = MqttConnectOptions()
        options.isAutomaticReconnect = true
        options.isCleanSession = true // Очищаем старые сессии для скорости
        options.connectionTimeout = 10

        try {
            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    Log.d("MQTT_TAG", "Подключено!")
                    onConnected()
                }
                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    Log.e("MQTT_TAG", "Ошибка: ${exception?.message}")
                }
            })
        } catch (e: Exception) { e.printStackTrace() }
    }

    fun publish(topic: String, payload: String) {
        try {
            if (client.isConnected) {
                val message = MqttMessage(payload.toByteArray())
                message.qos = 0 // САМОЕ ВАЖНОЕ: 0 — это мгновенная отправка без задержек
                message.isRetained = false
                client.publish(topic, message)
                Log.d("MQTT_TAG", "Пуля улетела: $payload")
            }
        } catch (e: Exception) { Log.e("MQTT_TAG", "Ошибка отправки: ${e.message}") }
    }
}