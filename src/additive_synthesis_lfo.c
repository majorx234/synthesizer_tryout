#include <stdio.h>
#include <raylib.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define SAMPLE_DURATION (1.0 / SAMPLE_RATE)
#define STREAM_BUFFER_SIZE 2048
#define NUM_OSCILLATORS 1

typedef struct {
  double phase;
  double freq;
  double omega_freq_modulated;
  double amplitude;
  double time_step;
  double old_time_step;
} Oscillator;

void updateOsc(Oscillator* osc, double lfo_freq_modulation)
{
  // doing frequenz modulation
  // sin((w + (lfo * w)) * t)
  double omega = 2.0 * PI * osc->freq;
  osc->omega_freq_modulated = (omega + lfo_freq_modulation * omega);
  // zeit hochzählen
  osc->time_step += 1.0/SAMPLE_RATE;
}

// STREAM_BUFFER_SIZE anzahl der samples im
// wieviel zeit pro abtastwert
//samplerate = 44100/s
// STREAM_BUFFER_SIZE/44100 => wieviel zeit streambuffersize

void zeroSignal(float* signal)
{
   for (size_t t = 0; t < STREAM_BUFFER_SIZE; ++t)
    {
      signal[t] = 0.0;
    }
}

double sineWaveOsc(Oscillator* osc)
{
  // sin(2*pi*t*f + phase)
  // normalisierte frequenz
  double omega = osc->omega_freq_modulated;
  double sine_stuff = (osc->time_step * omega + osc->phase/omega);
  if(sine_stuff >= 2*PI) {
    osc->time_step = 0.0;
  }
  return sinf(sine_stuff) * osc->amplitude;
}

void accumulateSignal(float* signal, Oscillator* osc, double* signal_lfo)
{
  for (size_t t = 0; t < STREAM_BUFFER_SIZE; ++t)
  {
    updateOsc(osc, signal_lfo[t]);
    //updateOsc(osc, 0.0);
    signal[t] += (float)sineWaveOsc(osc);
  }
}

void calc_lfo_signal(double* signal_lfo, Oscillator* lfo)
{
  for (size_t t = 0; t < STREAM_BUFFER_SIZE; ++t)
  {
    updateOsc(lfo, 0.0);
    signal_lfo[t] = sineWaveOsc(lfo);
  }
}

int main(void)
{
  const int screen_width = 1024;
  const int screen_height = 768;
  InitWindow(screen_width, screen_height, "synthesizer_tryout");
  SetTargetFPS(60);
  InitAudioDevice();

  unsigned int sample_rate = SAMPLE_RATE;
  SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
  AudioStream synth_stream = LoadAudioStream(sample_rate, sizeof(float) * 8, 1);

  SetAudioStreamVolume(synth_stream, 0.05f);
  PlayAudioStream(synth_stream);

  Oscillator osc[NUM_OSCILLATORS] = {0};
  Oscillator lfo = {.phase = 0.0};
  lfo.freq = 1.0;
  lfo.amplitude = 0.5;
  float signal[STREAM_BUFFER_SIZE];
  double signal_lfo[STREAM_BUFFER_SIZE];

  while(!WindowShouldClose())
  {
    Vector2 mouse_pos = GetMousePosition();
    double normalized_mouse_x = (mouse_pos.x /(double)screen_width);
    double base_freq = 25.0 + 400.0; //(normalized_mouse_x * 400.0);

    calc_lfo_signal(signal_lfo, &lfo);
    if (IsAudioStreamProcessed(synth_stream))
    {
      zeroSignal(signal);
      for (size_t i = 0; i < NUM_OSCILLATORS; i++)
      {
        osc[i].freq = base_freq * 2.0 * (i+1);
        osc[i].amplitude = 1.0 / (2.0*i+1);

        accumulateSignal(signal, &osc[i], signal_lfo);
        //printf("osc signal %f\n", signal[i]);
        //printf("lfosc signal %f\n", lfo.freq);
      }
      UpdateAudioStream(synth_stream, signal, STREAM_BUFFER_SIZE);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (size_t i = 0; i < screen_width; ++i)
    {
      DrawPixel(i, screen_height/4 + (int)(signal[i] * 100), BLUE);
    }
    for (size_t i = 0; i < screen_width; ++i)
    {
      DrawPixel(i, 3*screen_height/4 + (int)(signal_lfo[i] * 100), BLUE);
    }

    EndDrawing();
  }
  UnloadAudioStream(synth_stream);
  CloseAudioDevice();
  CloseWindow();
}
