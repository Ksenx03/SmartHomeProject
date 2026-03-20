// Модель устройства (ООП подход)
data class SmartDevice(
    val id: Int,
    val name: String,
    val type: DeviceType,
    var status: String = "OK",
    var isActive: Boolean = false
)

// Перечисление типов для логики SOLID
enum class DeviceType {
    HEATING,        // Отопление
    VENTILATION,    // Вентилятор
    WATER_SENSOR,   // Датчик воды
    ACCESS_CONTROL, // Доступ (чип)
    LIGHTING,       // Свет
    BLINDS          // Жалюзи
}