/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"

/* Application & Tasks includes */
#include "task_monitor.h"

/********************** macros and definitions *******************************/
#define G_TASK_MONITOR_CNT_INI 0ul
#define TASK_MONITOR_DEL_MAX (pdMS_TO_TICKS(2000ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_monitor_wait_2000mS = "   ==> Task MONITOR - Wait:  2000mS";

/********************** external data declaration ****************************/
uint32_t g_task_monitor_cnt;
extern uint32_t g_task_xxxx_tx_runtime_us;
extern uint32_t g_task_xxxx_tx_wcet_us;

/********************** external functions definition ************************/
void task_monitor(void *parameters)
{
    UNUSED(parameters);

    g_task_monitor_cnt = G_TASK_MONITOR_CNT_INI;

    LOGGER_INFO(" ");
    LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

    for (;;)
    {
        g_task_monitor_cnt++;

        LOGGER_INFO("  I2C TX runtime [uS] = %lu - WCET [uS] = %lu",
                    g_task_xxxx_tx_runtime_us,
                    g_task_xxxx_tx_wcet_us);

        LOGGER_INFO(p_task_monitor_wait_2000mS);
        vTaskDelay(TASK_MONITOR_DEL_MAX);
    }
}
