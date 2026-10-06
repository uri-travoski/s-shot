#pragma once

#include <QObject>
#include <QString>
#include <memory>

enum class HotkeyAction {
    Fullscreen,
    Region,
    Scrolling,
    ColorPicker,
    Editor
};

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    struct Impl;

    static HotkeyManager& instance();
    ~HotkeyManager();

    bool init();
    void updateHotkeys();

signals:
    void hotkeyTriggered(HotkeyAction action);

private:
    HotkeyManager();

    std::unique_ptr<Impl> m_impl;
};
