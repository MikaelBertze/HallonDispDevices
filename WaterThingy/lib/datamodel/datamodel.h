#ifndef _DATAMODEL_H
#define _DATAMODEL_H


typedef struct {
  int capture_img_x;
  int capture_img_y;
  int capture_img_size;
  
  int filter_start;
  int filter_length;

} settings_t;

typedef struct {
  int angle;
  int acc_angle;
  int angle_diff;
  int timediff_ms;
  long measurement_time_ms;
  double acc_consumption;
  double consumption;
  bool handled;
  bool enabled;
  int times[10];
} angle_report_t;

typedef struct {
  settings_t settings;
  angle_report_t state;
} data_model_t;

#endif