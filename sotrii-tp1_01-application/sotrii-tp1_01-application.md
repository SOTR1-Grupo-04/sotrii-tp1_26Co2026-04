# Respuestas Gemini

El código implementa una arquitectura de **Sistema Activado por Eventos (ETS)** utilizando **FreeRTOS** sobre un microcontrolador de la familia STM32 (evidenciado por el uso del HAL de ST). El diseño principal gira en torno a desacoplar la lógica de la aplicación de las operaciones de hardware, utilizando una capa de abstracción basada en tareas y colas para manejar la comunicación por bus I2C de forma asíncrona y segura entre hilos (*thread-safe*).

---

## 1. Capa de Inicialización y Control del Sistema

### `app.c`

Es el punto de entrada principal para la inicialización del entorno OS y la aplicación.

* **Configuración inicial**: Pone en cero los contadores globales del sistema y emite por consola de registros (*logger*) información sobre la versión y el inicio de la aplicación.
* **Creación de tareas de usuario**: Instancia y lanza dos hilos de FreeRTOS con prioridad `tskIDLE_PRIORITY + 1`: `Task Sender` y `Task Receiver`, asignándoles el doble del tamaño de pila mínimo (`configMINIMAL_STACK_SIZE`).
* **Inicialización de periféricos y abstracciones**: Llama a `open_i2c(&hi2c1)` para arrancar la infraestructura del controlador I2C, inicializa las interrupciones en `app_it_init()` y configura el contador de ciclos de reloj (DWT - *Data Watchpoint and Trace*) para mediciones precisas de tiempo de ejecución con `cycle_counter_init()`.

### `app_it.c`

Gestiona las interrupciones externas de la aplicación.

* Contiene el *callback* de interrupción externa `HAL_GPIO_EXTI_Callback()`. Actualmente implementa la estructura lógica para detectar la pulsación de un botón físico (`BTN_A_PIN`), reservando el bloque condicional para futuras acciones controladas por eventos de hardware.

### `freertos.c`

Implementa las funciones de gancho (*hooks* o *callbacks*) de FreeRTOS para el monitoreo del sistema y manejo de errores:

* **`vApplicationIdleHook()`**: Se ejecuta en la tarea Idle cuando no hay otras tareas listas para correr. Incrementa el contador de inactividad `g_task_idle_cnt`, siendo el punto ideal para poner al procesador en modos de bajo consumo (*Sleep*).
* **`vApplicationTickHook()`**: Se ejecuta en cada interrupción del *tick* del sistema operativo (en contexto de ISR) y actualiza el contador global de *ticks* `g_app_tick_cnt`.
* **`vApplicationStackOverflowHook()`**: Actúa como trampa de seguridad si el sistema detecta que alguna tarea ha desbordado su espacio de pila reservado. Deshabilita interrupciones (`taskENTER_CRITICAL`) y cuelga el sistema con un `configASSERT(0)` para depuración, registrando el fallo en `g_app_stack_overflow_cnt`.

---

## 2. Capa de Aplicación (Tareas de Usuario)

### `task_sender.c`

Simula una tarea generadora de datos que se comunica con un módulo LCD externo.

* **Objetivo I2C**: Configura una dirección de dispositivo `0x27` y un byte de datos inicial `0x55`. Esto corresponde al clásico expansor de E/S de 8 bits **PCF8574**, comúnmente utilizado como adaptador serie I2C para pantallas LCD alfanuméricas.
* **Bucle infinito**: En cada iteración, incrementa su contador estadístico, invierte bit a bit el valor del dato (`dev_data = ~dev_data;`, alternando entre `0x55` y `0xAA`), envía la orden de escritura llamando a la interfaz abstracta `write_i2c()` y se bloquea pacíficamente durante 250 ms (`vTaskDelay`).

### `task_receiver.c`

Actualmente funciona como una tarea par o receptora de demostración.

* Implementa el bucle estructural básico de una tarea de FreeRTOS: incrementa un contador local `g_task_receiver_cnt`, emite un mensaje de registro y espera 250 ms. Está pensada como contraparte para consumir datos o sincronizarse en eventos futuros.

---

## 3. Capa de Interfaz y Controlador I2C Asíncrono

El aspecto más destacable del sistema es cómo abstrae el bus I2C. En lugar de que la tarea `task_sender` llame directamente a las funciones bloqueantes del HAL (lo que causaría problemas de concurrencia y latencia), se utiliza un patrón de **productor-consumidor** mediado por colas de FreeRTOS.

### `task_i2c_interface.c` / `task_i2c_interface.h`

Actúa como una API externa para el resto de la aplicación, aislando el hardware físico:

* **`open_i2c()`**: Inicializa la estructura interna del driver (`task_i2c_dta`). Crea dos colas de mensajes (`queue_tx` para transmisión con capacidad para 5 tramas, y `queue_rx` para recepción) y las registra para depuración. Finalmente, levanta dos tareas dedicadas en background: `Task I2C Tx` y `Task I2C Rx`.
* **`write_i2c()`**: Es la función que utilizan las tareas de usuario. Encapsula la dirección del dispositivo físico y el dato a enviar en una estructura `task_i2c_tx_dta_t`, y la empuja a la cola de transmisión (`queue_tx`) mediante `xQueueSend()`. Si la cola está llena, la tarea invocadora se bloqueará (`portMAX_DELAY`) hasta que haya espacio.
* **`release_i2c()`**: Permite el apagado limpio del driver, eliminando las tareas y desasignando las colas de FreeRTOS.

### `task_i2c.c`

Contiene la implementación real de los hilos de fondo encargados del hardware físico:

* **`task_i2c_tx` (El Consumidor)**: En su bucle principal, se queda durmiendo e esperando mensajes en `queue_tx` mediante `xQueueReceive()`. Tan pronto como `task_sender` introduce un dato, esta tarea se despierta, mide el tiempo de inicio con el contador DWT y ejecuta la transferencia física al bus invocando a `HAL_I2C_Master_Transmit()`. Al terminar, mide el tiempo que tomó la transacción (`g_task_xxxx_tx_runtime_us`) y espera 250 ms.
* **`task_i2c_rx`**: Tarea paralela configurada para futuras lecturas del bus. En su versión actual, actúa como una tarea *heartbeat* o de monitoreo: resetea el contador de ciclos, cambia el estado de un LED físico (`HAL_GPIO_TogglePin(LED_A_PORT, LED_A_PIN)`), calcula el tiempo de ejecución y duerme 250 ms.

---

## Flujo de Datos del Sistema

El flujo completo de una operación de escritura muestra la potencia de esta arquitectura modular:

$$\text{task\_sender} \xrightarrow{\quad\text{write\_i2c()}\quad} \text{Queue (Tx)} \xrightarrow{\quad\text{task\_i2c\_tx}\quad} \text{HAL\_I2C\_Master\_Transmit()} \longrightarrow \text{Hardware I2C (PCF8574 LCD)}$$

Este diseño garantiza que las operaciones de hardware lentas o susceptibles a bloqueo no detengan el flujo del resto de la aplicación, delegando el acceso físico al periférico a un único punto centralizado y protegido frente a condiciones de carrera.

---

# Medición WCET

## Observaciones de la Implementación y Prueba

Durante la implementación del paso correspondiente al driver I2C sobre FreeRTOS, se observó lo siguiente:

* Se utilizó el periférico **I2C1** del STM32F446RE, configurado sobre los pines `PB8` (`SCL`) y `PB9` (`SDA`).
* La prueba funcional del driver se realizó con un **LCD 16x2 con backpack I2C PCF8574** en la dirección `0x27`.
* La tarea `task_sender` dejó de enviar el patrón alternado `0x55/0xAA` y pasó a ejecutar una secuencia de inicialización del LCD en modo de 4 bits.
* Una vez inicializado el display, `task_sender` escribe el texto `"hola mundo"` carácter por carácter usando únicamente la función de interfaz `write_i2c()`.
* Para que la secuencia del LCD funcionara correctamente, fue necesario eliminar el retardo de `250 ms` dentro de `task_i2c_tx()`, ya que ese retardo entre bytes rompía la inicialización y la escritura del display.
* Se agregó una tarea periódica `task_monitor`, cuya única función es informar por logger el tiempo de ejecución medido en la transmisión I2C y el peor caso observado.

## Observaciones sobre la medición de WCET

Para medir el WCET observado se utilizó el contador de ciclos **DWT**. La medición se realiza dentro de `task_i2c_tx()` reseteando el contador antes de llamar a `HAL_I2C_Master_Transmit()` y leyendo el tiempo en microsegundos al finalizar la transmisión.

Las variables usadas para la observación son:

* `g_task_xxxx_tx_runtime_us`: tiempo de ejecución de la última transmisión I2C.
* `g_task_xxxx_tx_wcet_us`: máximo tiempo de ejecución observado hasta el momento.

Durante la depuración se observó que:

* el valor de `g_task_xxxx_tx_runtime_us` cambia en cada transmisión según la operación efectivamente ejecutada sobre el bus I2C,
* el valor de `g_task_xxxx_tx_wcet_us` solo se actualiza cuando aparece una transmisión que supera el máximo previamente registrado,
* el WCET observado corresponde al tiempo de ejecución del backend de transmisión (`task_i2c_tx`) y no al tiempo de retorno de la función de interfaz `write_i2c()`,
* si se introduce un retardo artificial dentro de `task_i2c_tx()`, la medición deja de representar correctamente el costo real de la transmisión I2C,
* para visualizar la evolución de las mediciones sin saturar el sistema, se agregó una tarea periódica `task_monitor` que informa `runtime` y `WCET` cada `2000 ms`.
* la máxima medición del `WCET` que se obtuvo fue de 247246 microsegundos, como se puede observar en la siguiente línea de logueo:

```[info]   I2C TX runtime [uS] = 197 - WCET [uS] = 247246```
