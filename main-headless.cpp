#include <iostream>
#include <qemu-hypervisor.hpp>

int main(int argc, char *argv[])
{
    QemuContext ctx;
    bool verbose = false;
    std::string instance = QEMU_DEFAULT_INSTANCE;
    std::string tapname = QEMU_DEFAULT_INTERFACE;
    std::string machine = QEMU_DEFAULT_MACHINE;
    QEMU_DISPLAY display = QEMU_DISPLAY::NONE;

    for (int i = 1; i < argc; ++i)
    { // Remember argv[0] is the path to the program, we want from argv[1] onwards

        if (std::string(argv[i]).find("-h") != std::string::npos)
        {
            std::cout << "Usage(): " << argv[0] << " (-h) (-v) -instance {default=" << QEMU_DEFAULT_INSTANCE << "} -interface {default=" << QEMU_DEFAULT_INTERFACE << "}  -machine {default=" << QEMU_DEFAULT_MACHINE << "} -drive hdd (n+1)" << std::endl;
            exit(-1);
        }

        if (std::string(argv[i]).find("-v") != std::string::npos)
        {
            verbose = true;
        }

        if (std::string(argv[i]).find("-instance") != std::string::npos && (i + 1 < argc))
        {
            instance = argv[i + 1];
        }

        if (std::string(argv[i]).find("-interface") != std::string::npos && (i + 1 < argc))
        {
            tapname = argv[i + 1];
        }

        if (std::string(argv[i]).find("-machine") != std::string::npos && (i + 1 < argc))
        {
            machine = argv[i + 1];
        }

        if (std::string(argv[i]).find("-drive") != std::string::npos && (i + 1 < argc))
        {
            QEMU_drive(ctx, argv[i + 1]);
        }
        
    }

    QEMU_instance(ctx, instance);
    QEMU_display(ctx, display);
    QEMU_machine(ctx, machine);

    // if all is well, we could fork, here and setup a listener, for
    // the qemu-sockets, we specify - reading back and forth.

    QEMU_Launch(ctx, tapname);

    return EXIT_SUCCESS;
}
