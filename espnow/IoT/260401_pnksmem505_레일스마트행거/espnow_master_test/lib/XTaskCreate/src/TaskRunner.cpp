#include "TaskRunner.h"

TaskRunner::TaskRunner(const Config &config)
    : _config(config),
      _handle(nullptr),
      _loop(nullptr),
      _loopWithContext(nullptr),
      _context(nullptr),
      _useContext(false) {}

void TaskRunner::begin() {
  if (_handle != nullptr) {
    return;
  }
#if CONFIG_FREERTOS_UNICORE
  xTaskCreate(TaskRunner::entry, _config.name, _config.stackSize, this, _config.priority,
              &_handle);
#else
  if (_config.core >= 0) {
    xTaskCreatePinnedToCore(TaskRunner::entry, _config.name, _config.stackSize, this,
                            _config.priority, &_handle, _config.core);
  } else {
    xTaskCreate(TaskRunner::entry, _config.name, _config.stackSize, this, _config.priority,
                &_handle);
  }
#endif
}

void TaskRunner::begin(const Begin &beginConfig) {
  if (beginConfig.loopWithContext) {
    _loopWithContext = beginConfig.loopWithContext;
    _context = beginConfig.context;
    _useContext = true;
  } else if (beginConfig.loop) {
    _loop = beginConfig.loop;
    _useContext = false;
  }
  begin();
}

void TaskRunner::entry(void *param) {
  TaskRunner *self = static_cast<TaskRunner *>(param);
  for (;;) {
    if (self->_useContext) {
      if (self->_loopWithContext) {
        self->_loopWithContext(self->_context);
      }
    } else if (self->_loop) {
      self->_loop();
    }
    delay(1);
  }
}
