// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "Avatar.h"

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

namespace m5avatar {

unsigned int seed = 0;

#ifndef rand_r
#define init_rand() srand(seed)
#define _rand() rand()
#else
#define init_rand() ;
#define _rand() rand_r(&seed)
#endif

#ifdef SDL_h_
#define TaskResult() return 0
#define TaskDelay(ms) lgfx::delay(ms)
long random(long howbig) { return std::rand() % howbig; }

// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#elif defined (ARDUINO_ARCH_SPRESENSE)
#define TaskResult() pthread_exit(NULL)
#define TaskDelay(ms) usleep((ms) * 1000)
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int suspended = 0; // shared variable

#else
#define TaskResult() vTaskDelete(NULL)
#define TaskDelay(ms) vTaskDelay(ms / portTICK_PERIOD_MS)
#endif

// TODO(meganetaaan): make read-only
DriveContext::DriveContext(Avatar *avatar) : avatar{avatar} {}

Avatar *DriveContext::getAvatar() { return avatar; }

TaskHandle_t drawTaskHandle;
TaskHandle_t facialTaskHandle;

TaskResult_t drawLoop(void *args) {
  DriveContext *ctx = reinterpret_cast<DriveContext *>(args);
  Avatar *avatar = ctx->getAvatar();
  // update drawings in the display
  while (avatar->isDrawing()) {

// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#if defined (ARDUINO_ARCH_SPRESENSE)
        pthread_mutex_lock(&lock);
        while (suspended) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);
#endif

    if (avatar->isDrawing()) {
      avatar->draw();
    }
    TaskDelay(10);
  }
  TaskResult();
}

TaskResult_t facialLoop(void *args) {
  int count = 0;
  DriveContext *ctx = reinterpret_cast<DriveContext *>(args);
  Avatar *avatar = ctx->getAvatar();
  uint32_t saccade_interval = 1000;
  uint32_t blink_interval = 1000;
  unsigned long last_saccade_millis = 0;
  unsigned long last_blink_millis = 0;
  bool eye_open = true;
  float vertical = 0.0f;
  float horizontal = 0.0f;
  float breath = 0.0f;
  init_rand();
  // update facial internal state
  while (avatar->isDrawing()) {
    if ((lgfx::millis() - last_saccade_millis) > saccade_interval) {
      vertical = _rand() / (RAND_MAX / 2.0) - 1;
      horizontal = _rand() / (RAND_MAX / 2.0) - 1;
      avatar->setRightGaze(vertical, horizontal);
      avatar->setLeftGaze(vertical, horizontal);
      saccade_interval = 500 + 100 * random(20);
      last_saccade_millis = lgfx::millis();
    }

    if (avatar->getIsAutoBlink()) {
      if ((lgfx::millis() - last_blink_millis) > blink_interval) {
        if (eye_open) {
          avatar->setEyeOpenRatio(1.0f);
          blink_interval = 2500 + 100 * random(20);
        } else {
          avatar->setEyeOpenRatio(0.0f);
          blink_interval = 300 + 10 * random(20);
        }
        eye_open = !eye_open;
        last_blink_millis = lgfx::millis();
      }
    }

    count = (count + 1) % 100;
    breath = sin(count * 2 * PI / 100.0);
    avatar->setBreath(breath);
    TaskDelay(33);  // approx. 30fps
  }
  TaskResult();
}

Avatar::Avatar() : Avatar(new Face()) {}

Avatar::Avatar(Face *face)
    : face{face},
      _isDrawing{false},
      expression{Expression::Neutral},
      breath{0},
      leftEyeOpenRatio_{1.0f},
      leftGazeH_{1.0f},
      leftGazeV_{1.0f},
      rightEyeOpenRatio_{1.0f},
      rightGazeH_{1.0f},
      rightGazeV_{1.0f},
      isAutoBlink_{true},
      mouthOpenRatio{0},
      rotation{0},
      scale{1},
      palette{ColorPalette()},
      speechText{""},
      colorDepth{1},
      batteryIconStatus{BatteryIconStatus::invisible} {}

Avatar::~Avatar() { delete face; }

void Avatar::setFace(Face *face) { this->face = face; }

Face *Avatar::getFace() const { return face; }

void Avatar::addTask(TaskFunction_t f, const char *name,
                     const uint32_t stack_size, UBaseType_t priority,
                     TaskHandle_t *const task_handle,
                     const BaseType_t core_id) {
  DriveContext *ctx = new DriveContext(this);
#ifdef SDL_h_
  if (task_handle == NULL) {
    SDL_CreateThreadWithStackSize(f, name, stack_size, ctx);
  } else {
    *task_handle = SDL_CreateThreadWithStackSize(f, name, stack_size, ctx);
  }
#else
  // TODO(meganetaaan): set a task handler
// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#if defined (ARDUINO_ARCH_SPRESENSE)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stack_size);
  pthread_attr_setschedpolicy(&attr, SCHED_RR); // （SCHED_FIFO/SCHED_RR）
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
// TODO
//  struct sched_param param;
//  param.sched_priority = priority;
//  pthread_attr_setschedparam(&attr, &param);
  
  pthread_create(task_handle, &attr, f, ctx);
  
  pthread_attr_destroy(&attr);
  pthread_setname_np(*task_handle, name);
#else
  xTaskCreateUniversal(f,           /* Function to implement the task */
                       name,        /* Name of the task */
                       stack_size,  /* Stack size in words */
                       ctx,         /* Task input parameter */
                       priority,    /* Priority of the task */
                       task_handle, /* Task handle. */
                       core_id);    /* Core No*/
#endif
#endif
}

void Avatar::init(int colorDepth) {
  // for compatibility with older version
  start(colorDepth);
}

void Avatar::stop() { _isDrawing = false; }

void Avatar::suspend() {
#ifndef SDL_h_
// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#if defined (ARDUINO_ARCH_SPRESENSE)
  pthread_mutex_lock(&lock);
  suspended = 1;
  pthread_mutex_unlock(&lock);
#else
  vTaskSuspend(drawTaskHandle);
#endif
#endif
}

void Avatar::resume() {
#ifndef SDL_h_
// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#if defined (ARDUINO_ARCH_SPRESENSE)
  pthread_mutex_lock(&lock);
  suspended = 0;
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond);
#else
  vTaskResume(drawTaskHandle);
#endif
#endif
}

void Avatar::start(int colorDepth) {
  // if the task already started, don't create another task;
  if (_isDrawing) return;
  _isDrawing = true;

  this->colorDepth = colorDepth;
  DriveContext *ctx = new DriveContext(this);
#ifdef SDL_h_
  drawTaskHandle =
      SDL_CreateThreadWithStackSize(drawLoop, "drawLoop", 2048, ctx);
  SDL_CreateThreadWithStackSize(facialLoop, "facialLoop", 1024, ctx);
#else
  // TODO(meganetaaan): keep handle of these tasks
// SPRESENSE Arduino Framework doen't use FreeRTOS but NuttX RTOS
#if defined (ARDUINO_ARCH_SPRESENSE)
  pthread_attr_t attr;
  struct sched_param param;
  
  // drawLoop
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 2048);
  pthread_attr_setschedpolicy(&attr, SCHED_RR); // （SCHED_FIFO/SCHED_RR）
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
// TODO
//  param.sched_priority = 10;
//  pthread_attr_setschedparam(&attr, &param);
  
  pthread_create(&drawTaskHandle, &attr, drawLoop, ctx);
  
  pthread_attr_destroy(&attr);
  pthread_setname_np(drawTaskHandle, "drawLoop");

  // facialLoop
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 1024);
  pthread_attr_setschedpolicy(&attr, SCHED_RR); // （SCHED_FIFO/SCHED_RR）
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
// TODO
//  param.sched_priority = 20;
//  pthread_attr_setschedparam(&attr, &param);
  
  pthread_create(&facialTaskHandle, &attr, facialLoop, ctx);
  
  pthread_attr_destroy(&attr);
  pthread_setname_np(facialTaskHandle, "facialLoop");

#else
  xTaskCreateUniversal(drawLoop,        /* Function to implement the task */
                       "drawLoop",      /* Name of the task */
                       2048,            /* Stack size in words */
                       ctx,             /* Task input parameter */
                       1,               /* Priority of the task */
                       &drawTaskHandle, /* Task handle. */
                       APP_CPU_NUM);

  xTaskCreateUniversal(facialLoop,   /* Function to implement the task */
                       "facialLoop", /* Name of the task */
                       1024,         /* Stack size in words */
                       ctx,          /* Task input parameter */
                       2,            /* Priority of the task */
                       NULL,         /* Task handle. */
                       APP_CPU_NUM);
#endif
#endif
}

void Avatar::draw() {
  Gaze rightGaze = Gaze(this->rightGazeV_, this->rightGazeV_);
  Gaze leftGaze = Gaze(this->leftGazeV_, this->leftGazeH_);
  DrawContext *ctx = new DrawContext(
      this->expression, this->breath, &this->palette, rightGaze,
      this->rightEyeOpenRatio_, leftGaze, this->leftEyeOpenRatio_,
      this->mouthOpenRatio, this->speechText, this->rotation, this->scale,
      this->colorDepth, this->batteryIconStatus, this->batteryLevel,
      this->speechFont);
  face->draw(ctx);
  delete ctx;
}

bool Avatar::isDrawing() { return _isDrawing; }

void Avatar::setExpression(Expression expression) {
  suspend();
  this->expression = expression;
  resume();
}

Expression Avatar::getExpression() { return this->expression; }

void Avatar::setBreath(float breath) { this->breath = breath; }

float Avatar::getBreath() { return this->breath; }

void Avatar::setRotation(float radian) { this->rotation = radian; }

void Avatar::setScale(float scale) { this->scale = scale; }

void Avatar::setPosition(int top, int left) {
  this->getFace()->getBoundingRect()->setPosition(top, left);
}

void Avatar::setColorPalette(ColorPalette cp) { palette = cp; }

ColorPalette Avatar::getColorPalette(void) const { return this->palette; }

void Avatar::setMouthOpenRatio(float ratio) { this->mouthOpenRatio = ratio; }

void Avatar::setEyeOpenRatio(float ratio) {
  setRightEyeOpenRatio(ratio);
  setLeftEyeOpenRatio(ratio);
}

void Avatar::setLeftEyeOpenRatio(float ratio) {
  this->leftEyeOpenRatio_ = ratio;
}

float Avatar::getLeftEyeOpenRatio() { return this->leftEyeOpenRatio_; }

void Avatar::setRightEyeOpenRatio(float ratio) {
  this->rightEyeOpenRatio_ = ratio;
}

float Avatar::getRightEyeOpenRatio() { return this->rightEyeOpenRatio_; }

void Avatar::setIsAutoBlink(bool b) { this->isAutoBlink_ = b; }

bool Avatar::getIsAutoBlink() { return this->isAutoBlink_; }

void Avatar::setRightGaze(float vertical, float horizontal) {
  this->rightGazeV_ = vertical;
  this->rightGazeH_ = horizontal;
}

void Avatar::getRightGaze(float *vertical, float *horizontal) {
  *vertical = this->rightGazeV_;
  *horizontal = this->rightGazeH_;
}

void Avatar::setLeftGaze(float vertical, float horizontal) {
  this->leftGazeV_ = vertical;
  this->leftGazeH_ = horizontal;
}

void Avatar::getLeftGaze(float *vertical, float *horizontal) {
  *vertical = this->leftGazeV_;
  *horizontal = this->leftGazeH_;
}

void Avatar::getGaze(float *vertical, float *horizontal){
  *vertical = 0.5f * this->leftGazeV_ + 0.5f * this->rightGazeV_;
  *horizontal = 0.5f * this->leftGazeH_ + 0.5f * this->rightGazeH_;
}

void Avatar::setSpeechText(const char *speechText) {
  this->speechText = String(speechText);
}

void Avatar::setSpeechFont(const lgfx::IFont *speechFont) {
  this->speechFont = speechFont;
}

void Avatar::setBatteryIcon(bool batteryIcon) {
  if (!batteryIcon) {
    batteryIconStatus = BatteryIconStatus::invisible;
  } else {
    batteryIconStatus = BatteryIconStatus::unknown;
  }
}

void Avatar::setBatteryStatus(bool isCharging, int32_t batteryLevel) {
  if (this->batteryIconStatus != BatteryIconStatus::invisible) {
    if (isCharging) {
      this->batteryIconStatus = BatteryIconStatus::charging;
    } else {
      this->batteryIconStatus = BatteryIconStatus::discharging;
    }
    this->batteryLevel = batteryLevel;
  }
}

}  // namespace m5avatar
