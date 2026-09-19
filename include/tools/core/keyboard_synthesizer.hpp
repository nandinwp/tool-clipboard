#pragma once

namespace tools::core {

class KeyboardSynthesizer {
public:
    static KeyboardSynthesizer& instance();

    // Retorna se o dispositivo de entrada virtual está disponível
    bool is_available() const;

    // Simula a combinação de teclas Ctrl + V para colar na janela ativa
    void simulate_paste();

private:
    KeyboardSynthesizer();
    ~KeyboardSynthesizer();

    bool init_uinput();
    void cleanup_uinput();
    void emit(int type, int code, int val);

    int uinput_fd_{-1};
};

} // namespace tools::core
