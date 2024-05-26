#include <SPIFFS.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <config.h>
#include <I2SOutput.h>
#include <DACOutput.h>
#include "SDCard.h"
#include "esp_system.h"
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_STDIO
#include "minimp3.h"
#include "esp_log.h"

extern "C"
{
  void app_main();
}

const int BUFFER_SIZE = 1024;

void play_task(void *param)
{
  // create the output - see config.h for settings
  Output *output = new I2SOutput(I2S_NUM_0, i2s_speaker_pins);

  gpio_set_direction(GPIO_BUTTON, GPIO_MODE_INPUT);
  
  // setup for the mp3 decoded
  short *pcm = (short *)malloc(sizeof(short) * MINIMP3_MAX_SAMPLES_PER_FRAME);
  uint8_t *input_buf = (uint8_t *)malloc(BUFFER_SIZE);
  if (!pcm)
  {
    ESP_LOGE("main", "Failed to allocate pcm memory");
  }
  if (!input_buf)
  {
    ESP_LOGE("main", "Failed to allocate input_buf memory");
  }
  while (true)
  {
    // mp3 decoder state
    mp3dec_t mp3d = {};
    mp3dec_init(&mp3d);
    mp3dec_frame_info_t info = {};
    // keep track of how much data we have buffered, need to read and decoded
    int to_read = BUFFER_SIZE;
    int buffered = 0;
    int decoded = 0;
    bool is_output_started = false;

    FILE *fp = fopen("/sdcard/test.mp3", "rb");
    if (!fp)
    {
      ESP_LOGE("main", "Failed to open file");
      continue;
    }
    while (1)
    { 
        if (gpio_get_level(GPIO_BUTTON) == 1) {
            esp_restart();
        }

      // read in the data that is needed to top up the buffer
      size_t n = fread(input_buf + buffered, 1, to_read, fp);
      // feed the watchdog
      vTaskDelay(pdMS_TO_TICKS(1));
      // ESP_LOGI("main", "Read %d bytes\n", n);
      buffered += n;
      if (buffered == 0)
      {
        // we've reached the end of the file and processed all the buffered data
        output->stop();
        is_output_started = false;
        break;
      }
      // decode the next frame
      int samples = mp3dec_decode_frame(&mp3d, input_buf, buffered, pcm, &info);
      // we've processed this may bytes from teh buffered data
      buffered -= info.frame_bytes;
      // shift the remaining data to the front of the buffer
      memmove(input_buf, input_buf + info.frame_bytes, buffered);
      // we need to top up the buffer from the file
      to_read = info.frame_bytes;
      if (samples > 0)
      {
        // if we haven't started the output yet we can do it now as we now know the sample rate and number of channels
        if (!is_output_started)
        {
          output->start(info.hz);
          is_output_started = true;
        }
        // if we've decoded a frame of mono samples convert it to stereo by duplicating the left channel
        // we can do this in place as our samples buffer has enough space
        if (info.channels == 1)
        {
          for (int i = samples - 1; i >= 0; i--)
          {
            pcm[i * 2] = pcm[i];
            pcm[i * 2 - 1] = pcm[i];
          }
        }
        // write the decoded samples to the I2S output
        output->write(pcm, samples);
        // keep track of how many samples we've decoded
        decoded += samples;
      }
      // ESP_LOGI("main", "decoded %d samples\n", decoded);
    }
    ESP_LOGI("main", "Finished\n");
    fclose(fp);
  }
}

void app_main()
{
  #ifdef USE_SPIFFS
  ESP_LOGI(TAG, "Mounting SPIFFS on /sdcard");
  new SPIFFS("/sdcard");
  #else
  new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  #endif

  xTaskCreatePinnedToCore(play_task, "task", 32768, NULL, 1, NULL, 1);
}