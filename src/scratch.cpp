#if 0


/** MAIN
 */
int main(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc         = argc;
    app_info.argv         = argv;
    app_info.enable_vsync = false;
    Application& app      = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon          = "notf.png";
    window_info.size          = {800, 600};
    window_info.clear_color   = Color("#262a32");
    window_info.is_resizeable = true;
    WindowPtr window          = Window::create(window_info);

    ControllerPtr controller = std::make_shared<WindowController>(window);
    window->get_layout()->set_controller(controller);

    return app.exec();
}

#endif
