
#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>

using namespace std;

enum typelog {
  DEBUG,
  INFO,
  WARN,
  ERROR
};

struct log_cfg {
  bool headers = false;
  typelog level = INFO;
//  typelog level = DEBUG;
};

class LOG {
 public:
  log_cfg cfg;
  LOG() {}
  LOG(typelog type) {
    msglevel = type;
    if (cfg.headers) {
      operator<<("[" + getLabel(type) + "]");
    }
  }
  ~LOG() {
    if (opened) {
      cout << endl;
    }
    opened = false;
  }
  template<class T>
  LOG &operator<<(const T &msg) {
    if (msglevel >= cfg.level) {
      cout << msg;
      opened = true;
    }
    return *this;
  }
 private:
  bool opened = false;
  typelog msglevel = DEBUG;
  inline string getLabel(typelog type) {
    string label;
    switch (type) {
      case DEBUG: label = "DEBUG";
        break;
      case INFO: label = "INFO ";
        break;
      case WARN: label = "WARN ";
        break;
      case ERROR: label = "ERROR";
        break;
    }
    return label;
  }
};

#endif  /* LOG_HPP */
