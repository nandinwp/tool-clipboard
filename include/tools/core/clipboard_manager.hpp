#pragma once

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <functional>

namespace tools::core {

struct ClipboardItem {
    std::string text;
    std::string timestamp;
    bool pinned{false};
};

class ClipboardManager {
public:
    using UpdateCallback = std::function<void()>;

    ClipboardManager();
    ~ClipboardManager();

    // Inicia o monitoramento da área de transferência
    void start_monitoring(GdkDisplay *display);

    // Lê ativamente a área de transferência atual (útil ao ganhar foco no Wayland)
    void check_current_clipboard();

    // Adiciona um novo item manualmente
    bool add_item(const std::string &text);

    // Alterna o estado de fixado (Pin) do item
    void toggle_pin(size_t index);
    bool is_pinned(size_t index) const;

    // Copia um item do histórico de volta para a área de transferência do sistema
    bool copy_to_clipboard(size_t index);

    // Remove um item específico
    void remove_item(size_t index);

    // Limpa itens não-fixados do histórico
    void clear_history();

    // Retorna a lista dos últimos 10 itens
    const std::vector<ClipboardItem>& get_items() const { return items_; }

    // Retorna e define a quantidade máxima de itens
    int get_max_items() const { return max_items_; }
    void set_max_items(int max_count);

    // Registra callback para quando a lista for atualizada
    void set_update_callback(UpdateCallback cb) { update_callback_ = std::move(cb); }

    // Salva e carrega histórico do disco
    void load_history();
    void save_history() const;

private:
    static void on_clipboard_changed(GdkClipboard *clipboard, gpointer user_data);

    GdkClipboard *clipboard_{nullptr};
    std::vector<ClipboardItem> items_;
    int max_items_{10};
    UpdateCallback update_callback_;
    bool suppress_next_change_{false};
    std::string config_file_path_;
};

} // namespace tools::core
