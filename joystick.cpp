#include "EventListener.hpp"
#include  <libevdev-1.0/libevdev/libevdev.h>
#include <iostream>
#include <dirent.h>
#include <cstring>
#include  <errno.h>
#include  <unistd.h>
#include  <fcntl.h>


class  Controller: public EventListener {
  public:
    Controller();
    ~Controller();
    void update();
    void read(struct input_event& ev);
    virtual int getAxis() const{return axis;};
    virtual int getQuit() const{return quit;};
  private:
    int jfd = -1;
    struct libevdev*  context;
    int axis = 4;
    int quit = 0;

};

Controller::Controller() {
  DIR *dir;
  struct dirent *ent;
  std::string dest = "/dev/input/by-path/";
  char *c = NULL;
  if ((dir = opendir("/dev/input/by-path")) == NULL){
    std::cout<<"Can't list files in /dev/input/by-path"<<std::endl;
    return;
  }
  while ((ent = readdir (dir)) != NULL) {
    c = strstr(ent->d_name,"event-joystick");
    if(c != NULL){
      dest += ent->d_name;
      break;
    }
  }
  if (c == NULL){
    return;
  }
  closedir (dir);
  int  rc = 1;
  jfd = open( dest.c_str(), O_RDONLY | O_NONBLOCK);
  if (jfd < 0){
    std::cout<<"Failed to open Joystick : "<<strerror(-jfd)<<std::endl;
  }else{
    rc = libevdev_new_from_fd(jfd, &context);
    if (rc < 0) {
      std::cerr<< "Module stingray-wheel : Failed to init libevdev : "<<strerror(-rc)<<std::endl;
    }
    std::cout<<"Opened Joystick"<<std::endl;
  }
}

Controller::~Controller() {
  if(jfd >0){
    libevdev_free(context);
    close(jfd);
  }
}

void Controller::read(struct input_event& ev){
    int absValue;
    switch (ev.type) {
      case 1://EV_KEY (Button action)
        if((ev.code == 304 ||ev.code==292) && ev.value == 1 ){
          quit = 1;
        }
        break;
      case 3 : //EV_ABS (Joystick)
        if(ev.code == 3){
          absValue = ev.value/1000;
          axis = absValue;
        }
        break;
    };
}
void Controller::update(){
  int rc = LIBEVDEV_READ_STATUS_SUCCESS;
  if(jfd < 0) return; //return early if we don't have a connection
  while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS){
    struct input_event ev;
    rc = libevdev_next_event(context, LIBEVDEV_READ_FLAG_NORMAL/*|LIBEVDEV_READ_FLAG_BLOCKING*/, &ev);
    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
      while (rc == LIBEVDEV_READ_STATUS_SYNC) {
        rc = libevdev_next_event(context, LIBEVDEV_READ_FLAG_SYNC, &ev);
        read(ev);
      }
    }else if (rc == LIBEVDEV_READ_STATUS_SUCCESS){
      read(ev);
    }else if(rc != -EAGAIN){
      std::cout<<"Evdev error : "<<strerror(-rc)<<std::endl;
    }
  };

}



extern "C"{
  // the class factories
  extern EventListener* create() {
   return new Controller;
  }
  extern void destroy(EventListener* p) {
   delete p;
  }
  extern void update(EventListener* p){
    ((Controller*)p)->update();
  }
}
