#pragma once

class UiContext {
public:
    UiContext();
    ~UiContext();

    void update();
};

class Ui {
    static UiContext* ui_context_;

public:
    static void bind(UiContext* ui_context);
    static void unbind();
    static bool ready();
};
