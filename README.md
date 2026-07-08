# CESE - Sistemas Operativos de Tiempo Real II
## Trabajo Práctico N°: 1 - Tareas de FreeRTOS
### 26Co2026-04

### Responsable de la entrega:

| N° SIU | Apellidos, Nombres       | Fecha     | Deadline  |
| :----- | :---------------------   | :------:  | :-------: |
| e2505  | Botalla, Tomás Enrique   |           |           |
| e2615  | Restovich, Joaquin       |           |           |
| e2606  | Lazcano, Luca Mauricio   |           |           |

## Notas

- En Linux, usando STM32CubeIDE, cambiar:
    - "Run" > "Debug Configurations" > "Debugger" > "OpenOCD Command" por "${stm32cubeide_openocd_path}/openocd"
    - "Run" > "Debug Configurations" > "Debugger" > "Configuration Script" > "Mode Setup" > por "Software system reset"
    - "Run" > "Debug Configurations" > "Startup" > `monitor arm semihosting enable`