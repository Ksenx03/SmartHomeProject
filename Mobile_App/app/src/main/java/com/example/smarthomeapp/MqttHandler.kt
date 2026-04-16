package com.example.smarthomeapp

import android.content.Context
import android.util.Log
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

class MqttHandler(context: Context) {
    private val serverUri = "tcp://172.20.10.3:1883"
    private val clientId = "AndroidClient_" + System.currentTimeMillis()
    private val client = MqttAsyncClient(serverUri, clientId, MemoryPersistence())

    fun connect(onConnected: () -> Unit, onMessage: (String) -> Unit) {
        val options = MqttConnectOptions().apply {
            isAutomaticReconnect = true
            isCleanSession = true
        }

        client.setCallback(object : MqttCallback {
            override fun messageArrived(topic: String?, message: MqttMessage?) {
                onMessage(message.toString())
            }
            override fun connectionLost(cause: Throwable?) {}
            override fun deliveryComplete(token: IMqttDeliveryToken?) {}
        })

        try {
            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    // Убираем отсюда жесткую подписку на access!
                    // Мы будем подписываться в самих Активити
                    onConnected()
                }
                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {}
            })
        } catch (e: Exception) { e.printStackTrace() }
    }

    // НОВАЯ ФУНКЦИЯ: теперь мы можем слушать что угодно
    fun subscribe(topic: String) {
        if (client.isConnected) {
            client.subscribe(topic, 0)
        }
    }

    fun publish(topic: String, payload: String) {
        if (client.isConnected) {
            client.publish(topic, MqttMessage(payload.toByteArray()))
        }
    }
}