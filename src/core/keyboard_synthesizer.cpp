#include "tools/core/keyboard_synthesizer.hpp"
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace tools::core {

KeyboardSynthesizer& KeyboardSynthesizer::instance() {
    static KeyboardSynthesizer s_instance;
    return s_instance;
}

KeyboardSynthesizer::KeyboardSynthesizer() {
    init_uinput();
}

KeyboardSynthesizer::~KeyboardSynthesizer() {
    cleanup_uinput();
}

bool KeyboardSynthesizer::is_available() const {
    return uinput_fd_ >= 0;
}

bool KeyboardSynthesizer::init_uinput() {
    if (uinput_fd_ >= 0) return true;

    uinput_fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd_ < 0) {
        std::cerr << "[KeyboardSynthesizer] Nao foi possivel abrir /dev/uinput: " << strerror(errno) << std::endl;
        return false;
    }

    if (ioctl(uinput_fd_, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(uinput_fd_, UI_SET_KEYBIT, KEY_LEFTCTRL) < 0 ||
        ioctl(uinput_fd_, UI_SET_KEYBIT, KEY_V) < 0 ||
        ioctl(uinput_fd_, UI_SET_EVBIT, EV_SYN) < 0) {
        std::cerr << "[KeyboardSynthesizer] Falha ao configurar ioctls de teclado\n";
        close(uinput_fd_);
        uinput_fd_ = -1;
        return false;
    }

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    std::strcpy(usetup.name, "Tools Virtual Keyboard");

    if (ioctl(uinput_fd_, UI_DEV_SETUP, &usetup) < 0 ||
        ioctl(uinput_fd_, UI_DEV_CREATE) < 0) {
        std::cerr << "[KeyboardSynthesizer] Falha no UI_DEV_CREATE: " << strerror(errno) << std::endl;
        close(uinput_fd_);
        uinput_fd_ = -1;
        return false;
    }

    return true;
}

void KeyboardSynthesizer::cleanup_uinput() {
    if (uinput_fd_ >= 0) {
        ioctl(uinput_fd_, UI_DEV_DESTROY);
        close(uinput_fd_);
        uinput_fd_ = -1;
    }
}

void KeyboardSynthesizer::emit(int type, int code, int val) {
    if (uinput_fd_ < 0) return;

    struct input_event ie;
    std::memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = val;
    [[maybe_unused]] ssize_t ret = write(uinput_fd_, &ie, sizeof(ie));
}

void KeyboardSynthesizer::simulate_paste() {
    if (uinput_fd_ < 0) {
        if (!init_uinput()) return;
    }

    // Pressiona Left Ctrl
    emit(EV_KEY, KEY_LEFTCTRL, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(15000); // 15ms

    // Pressiona V
    emit(EV_KEY, KEY_V, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(25000); // 25ms

    // Solta V
    emit(EV_KEY, KEY_V, 0);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(15000); // 15ms

    // Solta Left Ctrl
    emit(EV_KEY, KEY_LEFTCTRL, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

} // namespace tools::core
