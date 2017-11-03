#include <assert.h>
#include <iostream>
#include <memory>
#include <thread>

// linux specific
#include <errno.h>
#include <linux/limits.h>
#include <sys/inotify.h>
#include <unistd.h>

/**
 * See man pages for `inotify`.
 */
constexpr size_t _filewatcher_buffer_size(size_t message_count)
{
    return (sizeof(struct inotify_event) + NAME_MAX + 1) * message_count;
}

class FileWatcher {
public: // enum
    enum Events : uint32_t {
        CREATED  = 1 << 0, ///< File/directory created in watched directory
        ACCESSED = 1 << 1, ///< File/directory accessed in watched directory
        MODIFIED = 1 << 2, ///< File/directory modified in watched directory
        DELETED  = 1 << 3, ///< File/directory deleted from watched directory
        ALL      = CREATED | ACCESSED | MODIFIED | DELETED,
    };

public: // methods
    FileWatcher() : m_notifier(s_CLOSED), m_watcher(s_CLOSED) {}

    ~FileWatcher() { stop_watching(); }

    void stop_watching()
    {
        if (m_notifier == s_CLOSED) {
            assert(m_watcher == s_CLOSED);
            return;
        }
        if (m_watcher != s_CLOSED) {
            inotify_rm_watch(m_notifier, m_watcher);
            m_watcher = s_CLOSED;
        }
        close(m_notifier);
        m_notifier = s_CLOSED;
    }

    void start_watching(const std::string& directory, const Events events)
    {
        if (m_notifier != s_CLOSED) {
            throw std::runtime_error("Already watching another folder"); // TODO: multiple watchers
        }

        // initialize the inotify instance
        m_notifier = inotify_init();
        if (m_notifier == s_CLOSED) {
            switch (errno) {
            case EMFILE:
                throw std::runtime_error("The per-process limit on the number of open file descriptors has been "
                                         "reached.");
            case ENFILE:
                throw std::runtime_error("The system-wide limit on the total number of open files has been "
                                         "reached.");
            case ENOMEM:
                throw std::runtime_error("Insufficient kernel memory is available.");
            default:
                throw std::runtime_error("Unexpected error code");
            }
        }

        uint32_t flags = 0;
        if (events | Events::CREATED) {
            flags |= IN_CREATE;
        }
        if (events | Events::ACCESSED) {
            flags |= IN_OPEN | IN_ACCESS | IN_CLOSE;
        }
        if (events | Events::MODIFIED) {
            flags |= IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO;
        }
        if (events | Events::DELETED) {
            flags |= IN_DELETE | IN_DELETE_SELF;
        }
        assert(m_watcher == s_CLOSED);
        m_watcher = inotify_add_watch(m_notifier, directory.c_str(), IN_ALL_EVENTS);
        if (m_watcher == s_CLOSED) {
            switch (errno) {
            case EACCES:
                throw std::runtime_error("Read access to the given file is not permitted");
            case EBADF:
                throw std::runtime_error("The given file descriptor is not valid");
            case EFAULT:
                throw std::runtime_error("Pathname points outside of the process's accessible address space");
            case EINVAL:
                throw std::runtime_error("The given event mask contains no valid events, or `m_notifier` is not an "
                                         "inotify file descriptor");
            case ENAMETOOLONG:
                throw std::runtime_error("Pathname is too long");
            case ENOENT:
                throw std::runtime_error("A directory component in pathname does not exist or is a dangling "
                                         "symbolic link");
            case ENOMEM:
                throw std::runtime_error("Insufficient kernel memory was available");
            case ENOSPC:
                throw std::runtime_error("The user limit on the total number of inotify watches was reached or the "
                                         "kernel failed to allocate a needed resource");
            default:
                throw std::runtime_error("Unexpected error code");
            }
        }

        std::cout << "Started watching directory: " << directory << std::endl;

        static const size_t buffer_size = _filewatcher_buffer_size(16);
        char buffer[buffer_size]; // see
        while (m_notifier != s_CLOSED) {
            long length = read(m_notifier, buffer, buffer_size);
            if (length < 0) {
                throw std::runtime_error("Error reading `inotify` buffer");
            }

            for (long i = 0; i < length;) {
                struct inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[i]);
                if (event->len) {
                    //                    if (event->mask & IN_CREATE) {
                    //                        if (event->mask & IN_ISDIR) {
                    //                            printf("New directory %s created.\n", event->name);
                    //                        }
                    //                        else {
                    //                            printf("New file %s created.\n", event->name);
                    //                        }
                    //                    }
                    //                    else if (event->mask & IN_DELETE) {
                    //                        if (event->mask & IN_ISDIR) {
                    //                            printf("Directory %s deleted.\n", event->name);
                    //                        }
                    //                        else {
                    //                            printf("File %s deleted.\n", event->name);
                    //                        }
                    //                    }
                    std::cout << "Got event " << event->mask << " for file/dir " << event->name << std::endl;
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }

        std::cout << "Finished watching directory: " << directory << std::endl;
    }

private: // fields
    int m_notifier;

    int m_watcher;

private: // static fields
    static const int s_CLOSED;
};

const int FileWatcher::s_CLOSED = -1;

// int main(int, char**)
//{
//    FileWatcher file_watcher;
//    std::thread watcher(&FileWatcher::start_watching, &file_watcher, "/home/clemens/code/scratch/test/",
//    FileWatcher::ALL); std::cin.get(); // wait for an `enter` file_watcher.stop_watching(); watcher.join(); return 0;
//}
