#if 0
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <memory>
using namespace std;

void ignore_me(void *) { }

class Application {
public:
    Application() {}

    void callAll()
    {
        for(auto& callback : m_callbacks)
        {
            callback();
        }
    }

    void registerCallback(function<void()> callback) { m_callbacks.push_back(callback); }

private:
    vector<function<void()>> m_callbacks;
};

static Application THE_APPLICATION;

class Widget {
public:
    Widget(const string& name = "") : m_name(name), m_monitor{this, ignore_me} {}

    // make sure NOT to copy the monitor as well as it must die with the instance
    // brilliant code from	http://stackoverflow.com/a/27721978
    // and					http://stackoverflow.com/a/3279550
    Widget(const Widget& other) : m_name(other.m_name) {}
    Widget(Widget&& other) : Widget() { swap(*this, other); }
    Widget& operator=(Widget other) { swap(*this, other); return *this; }
    friend void swap(Widget& a, Widget& b) { using std::swap; swap(a.m_name, b.m_name); }

    ~Widget() {cout << "Deleted: " << m_name << endl; }

    const string& name() const {return m_name; }

    void registerCallback()
    {
        std::weak_ptr<void> monitor = this->m_monitor;
        Application& app = THE_APPLICATION;
        auto someFunc = [this, monitor]()->bool
        {
            if(monitor.expired()){
                std::cout << "I expired" << std::endl;
                return false;
            } else {
                cout << m_name << endl;
                return true;
            }
        };
        app.registerCallback(someFunc);
    }

private:
    string m_name;

    std::shared_ptr<void> m_monitor;
};

int main() {

    Application& app = THE_APPLICATION;
    Widget theCopy;
    {
        auto deadWidget = Widget("Message from the grave!");
        deadWidget.registerCallback();

        theCopy = deadWidget;
        cout << "unnamed?: " << theCopy.name() << endl;
        cout << "Message from the grave?: " << deadWidget.name() << endl;
    }
    theCopy.registerCallback();

    auto x = Widget("That's numberwang!");
    x.registerCallback();

    auto v = Widget("That's a bad miss!");
    v.registerCallback();

    app.callAll();
    return 0;
}

#endif
