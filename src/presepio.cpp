#include "presepio.h"

// for t=0 and t=TIMELINE_DURATION percent values must be equal
int analog_timeline[ANALOGOUT_COUNT][TIME_PERCENT_ITEMS] = {{0, 0, 5, 10, 10, 0, 15, 20, 20, 0, 25, 30, 30, 0, 35, 40, 40, 0, -1, 0},
                                                            {0, 40, 5, 0, 10, 30, 15, 0, 20, 20, 25, 0, 30, 10, 35, 0, 40, 40, -1, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0},
                                                            {0, 10, 5, 20, 10, 30, 15, 40, 20, 50, 25, 60, 30, 70, 35, 80, 40, 90, 50, 0}};

Presepio::Presepio() : pc(USBTX, USBRX),
                       timeline(),
                       relay_board(),
                       triac_board()
{
}

void Presepio::init()
{
  curr_time = 0;
  tick_received = false;
  tick_count = 0;
  ticker.attach(callback(this, &Presepio::tick), 1.0 / TICKS_PER_SECOND);

  pc.baud(115200);
  pc.printf("===== Presepe =====\n");
  pc.printf("   version: 0.1a   \n");
  pc.printf("===================\n");

  timeline.create(TIMELINE_ENTRIES);
  timeline.add(0, 1, 100, 1500);
  timeline.add(0, 9, 100,    0);
  timeline.add(1500, 1, 0, 1500);
  timeline.add(1500, 9, 0,    0);
  timeline.add(3000, TimelineEntry::OutputForTimelineEnd, 0, 0);
}

void Presepio::playTimeline()
{
  if (tick_received)
  {
    tick_received = false;
  
    //Calculate the current time in milliseconds
    int currTime = 1000 * tick_count / TICKS_PER_SECOND;

    const TimelineEntry *currEntry = timeline.getCurrent();
    //TODO loop for multiple simultaneous (or near) entries
    if (currEntry->time <= currTime)
    {
      //pc.printf("event! out[%i]=%i\r\n", currEntry->output, currEntry->value);
      //Apply
      if (currEntry->isTimelineEnd()) {
        timeline.moveFirst();
        tick_count = 0; //Reset time when timeline ended
      } else {
        uint8_t output = currEntry->output;
        if (output >= 1 && output <= 8) {
          triac_board.setOutput(output - 1, currEntry->value);
        } else if (output >= 9 && output <= 40) {
          relay_board.setOutput(output - 9, currEntry->value);
        }
        timeline.moveNext();
      }
    }

    triac_board.updateOutputs();
    relay_board.updateOutputs();
  }
}

//Change leds light based on timeline
void Presepio::dimming()
{
  // next timeline index entry to interpolate
  static int dim_nextEntry_idx[ANALOGOUT_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1};

  if (tick_received)
  {
    tick_received = false;

    if (tick_count == TICKS_PER_TENTHOFASECOND)
    {
      tick_count = 0;
      curr_time += 1;
      //restart timeline
      if (curr_time == TIMELINE_DURATION)
      {
        curr_time = 0;
        int out;
        for (out = 0; out < ANALOGOUT_COUNT; out++)
          dim_nextEntry_idx[out] = 1;
      }
    }

    //Update percent for each out
    int out;
    for (out = 0; out < ANALOGOUT_COUNT; out++)
    {
      // interpolation between entries
      int nextEntry_idx = dim_nextEntry_idx[out];
      int prevEntry_idx = dim_nextEntry_idx[out] - 1;

      int prevT = analog_timeline[out][prevEntry_idx * 2];
      int prevP = analog_timeline[out][prevEntry_idx * 2 + 1];

      int nextT = analog_timeline[out][nextEntry_idx * 2];
      int nextP = analog_timeline[out][nextEntry_idx * 2 + 1];

      if (curr_time == nextT)
        dim_nextEntry_idx[out] = dim_nextEntry_idx[out] + 1;
      if (dim_nextEntry_idx[out] == TIMELINE_ENTRIES)
        dim_nextEntry_idx[out] = 1;

      triac_board.setOutput(out, ((nextP - prevP) * (curr_time - prevT) / (nextT - prevT)) + prevP);
    }

    triac_board.updateOutputs();
  }
}

void Presepio::tick()
{
  tick_received = true;
  tick_count += 1;
}