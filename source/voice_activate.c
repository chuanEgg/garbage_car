#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "stdlib.h"

// FreeRTOS headers & defines
#include <FreeRTOS.h>
#include <task.h>
#define TASK_STACK_SIZE 			(5 * 1024)
#define TCP_CLIENT_TASK_PRIORITY 	(1)
// task headers
#include "tcp_client.h"

// Cyberon headers
#include "cyberon_asr.h"

#define FRAME_SIZE                  (480u)
#define SAMPLE_RATE_HZ              (16000u)
#define DECIMATION_RATE             (96u)
#define AUDIO_SYS_CLOCK_HZ          (24576000u)
#define PDM_DATA                    (P10_5)
#define PDM_CLK                     (P10_4)
#define VOLUME_RATIO                (4*FRAME_SIZE)
#define THRESHOLD_HYSTERESIS        (25)
#define MAX_SILENCE_TIME			(30)
void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event);
void clock_init(void);
uint32_t get_volume(int16_t *audio_frame);
uint32_t silence_time = 0;

void asr_callback(const char *function, char *message, char *parameter);

volatile bool pdm_pcm_flag = false;
int16_t pdm_pcm_ping[FRAME_SIZE] = {0};
int16_t pdm_pcm_pong[FRAME_SIZE] = {0};
int16_t *pdm_pcm_buffer = &pdm_pcm_ping[0];
cyhal_pdm_pcm_t pdm_pcm;
cyhal_clock_t   audio_clock;
cyhal_clock_t   pll_clock;

const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate     = SAMPLE_RATE_HZ,
    .decimation_rate = DECIMATION_RATE,
    .mode            = CYHAL_PDM_PCM_MODE_LEFT,
    .word_length     = 16,  /* bits */
    .left_gain       = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
    .right_gain      = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
};
uint32_t noise_threshold = THRESHOLD_HYSTERESIS;

void voice_activate_task(void *args)
{
    uint64_t uid;

    clock_init();

    cyhal_pdm_pcm_init(&pdm_pcm, PDM_DATA, PDM_CLK, &audio_clock, &pdm_pcm_cfg);
    cyhal_pdm_pcm_register_callback(&pdm_pcm, pdm_pcm_isr_handler, NULL);
    cyhal_pdm_pcm_enable_event(&pdm_pcm, CYHAL_PDM_PCM_ASYNC_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);
    cyhal_pdm_pcm_start(&pdm_pcm);
    cyhal_pdm_pcm_read_async(&pdm_pcm, pdm_pcm_buffer, FRAME_SIZE);
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    printf("\x1b[2J\x1b[;H");
    printf("===== Cyberon DSpotter Demo =====\r\n");

    uid = Cy_SysLib_GetUniqueId();
    printf("uniqueIdHi: 0x%08lX, uniqueIdLo: 0x%08lX\r\n", (uint32_t)(uid >> 32), (uint32_t)(uid << 32 >> 32));

    if(!cyberon_asr_init(asr_callback))
    {
    	while(1);
    }

    printf("\r\nAwaiting voice input trigger command (\"Hello CyberVoice\"):\r\n");
    xTaskCreate(tcp_client_task, "Network task", TASK_STACK_SIZE, NULL, TCP_CLIENT_TASK_PRIORITY, NULL);

    while(1)
    {
        if(pdm_pcm_flag)
        {
            pdm_pcm_flag = 0;
            cyberon_asr_process(pdm_pcm_buffer, FRAME_SIZE);
        }
    }
}

void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event)
{
    static bool ping_pong = false;

    (void) arg;
    (void) event;

    if(ping_pong)
    {
        cyhal_pdm_pcm_read_async(&pdm_pcm, pdm_pcm_ping, FRAME_SIZE);
        pdm_pcm_buffer = &pdm_pcm_pong[0];
    }
    else
    {
        cyhal_pdm_pcm_read_async(&pdm_pcm, pdm_pcm_pong, FRAME_SIZE);
        pdm_pcm_buffer = &pdm_pcm_ping[0];
    }

    ping_pong = !ping_pong;
    pdm_pcm_flag = true;
}

void clock_init(void)
{
	cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[0]);
    cyhal_clock_set_frequency(&pll_clock, AUDIO_SYS_CLOCK_HZ, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);

    cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);

    cyhal_clock_set_source(&audio_clock, &pll_clock);
    cyhal_clock_set_enabled(&audio_clock, true, true);
}

uint32_t get_volume(int16_t *audio_frame)
{
	uint32_t volume = 0;
	for (uint32_t index = 0; index < FRAME_SIZE; index++)
	{
		volume += abs(audio_frame[index]);
	}
	return volume;
}
void asr_callback(const char *function, char *message, char *parameter)
{
	printf("[%s]%s(%s)\r\n", function, message, parameter);
	/* TODO: record stuff after triggered and process it*/
	if (strcmp(parameter, "--") == 0)
	{
		printf("end of line\n\n");
		cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
		pdm_pcm_flag = false;
		while(true)
		{
			if (pdm_pcm_flag)
			{
				pdm_pcm_flag = 0;
				uint32_t volume = get_volume(pdm_pcm_buffer);
//				for (uint32_t index = 0; index < (volume/VOLUME_RATIO); index++)
//				{
//					printf("-");
//				}
				printf("%ld\n\r", volume/VOLUME_RATIO);
//				printf("\n\r");
				if ((volume/VOLUME_RATIO) <= noise_threshold)
				{
					printf("\t silence\n");
					silence_time += 1;
					if (silence_time >= MAX_SILENCE_TIME)
					{
						silence_time = 0;
						cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
						printf("exiting loop\n\n");
						break;
					}
				}
				else
				{
					xQueueSendToBack(data_q, &volume, 0u);
					silence_time = 0;
				}

				/* Setup to read the next frame */
				cyhal_pdm_pcm_read_async(&pdm_pcm, pdm_pcm_buffer, FRAME_SIZE);
			}
			cyhal_syspm_sleep();

		}
	}
//	printf("\n\r");

}
