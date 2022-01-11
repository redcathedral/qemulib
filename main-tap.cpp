#include <iostream>
#include <qemu-hypervisor.hpp>
#include <qemu-link.hpp>
#include <yaml-cpp/yaml.h>

template <typename... Args>
std::string m3_string_format(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0)
    {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template <class type>
type globals(YAML::Node &configuration, std::string section, std::string key, type defaults)
{
    YAML::Node global = configuration[section];
    return global[key].as<type>();
}

std::vector<std::tuple<std::string, std::string>> loadstores(YAML::Node &config)
{
    YAML::Node node = config["datastores"];
    std::vector<std::tuple<std::string, std::string>> datastores;
    if (node.Type() == YAML::NodeType::Sequence)
    {
        for (std::size_t i = 0; i < node.size(); i++)
        {
            if (node.Type() == YAML::NodeType::Sequence)
            {
                YAML::Node seq = node[i];
                for (std::size_t j = 0; j < seq.size(); j++)
                {
                    for (YAML::const_iterator it = seq.begin(); it != seq.end(); ++it)
                    {
                        datastores.push_back(std::make_tuple<std::string, std::string>(it->first.as<std::string>(), it->second.as<std::string>()));
                    }
                }
            }
        }
    }
    return datastores;
}

int main(int argc, char *argv[])
{
    std::vector<std::tuple<std::string, std::string>> drives{
        {"meta-", "ubuntu2004backingfile"},
        {"vip-", "ubuntu2004backingfile"},
        {"test-", "ubuntu2004backingfile"},
        {"ubuntu2004-master", "ubuntu2004backingfile"},
        {"ubuntu2004-", "ubuntu2004-master"},
        {"hippogrif-", "test-ubuntu2004-hippogrif"},

    };

    std::vector<std::tuple<std::string, std::string>> datastores{
        {"main", "/home/gandalf/vms"},
        {"iso", "/home/gandalf/Downloads/Applications"},
    };

    QemuContext ctx;
    int port = 4444, snapshot = 0, mandatory = 0, driveid = 1;
    std::string masterinterface = "enp2s0";
    QEMU_DISPLAY display = QEMU_DISPLAY::GTK;
    YAML::Node config = YAML::LoadFile("/home/gandalf/workspace/qemu/registry.yml");
    std::string instance = globals<std::string>(config, "globals", "default_instance", QEMU_DEFAULT_INSTANCE);
    std::string machine = globals<std::string>(config, "globals", "default_machine", QEMU_DEFAULT_MACHINE);
    std::string bridge = globals<std::string>(config, "globals", "default_bridge", QEMU_DEFAULT_BRIDGE); 
    std::string nspace = globals<std::string>(config, "globals", "default_netspace", QEMU_DEFAULT_NETSPACE);
    std::string disksize = globals<std::string>(config, "globals", "default_disk_size", "32g");
    std::string default_datastore = globals<std::string>(config, "globals", "default_datastore", "default");
    std::string default_isostore = globals<std::string>(config, "globals", "default_isostore", "iso");

    std::vector<std::tuple<std::string, std::string>> stores = loadstores(config);
    if (stores.size() > 0)
    {
        datastores = stores;
    }

    std::string usage = m3_string_format("usage(): %s (-help) (-headless) (-snapshot) -incoming {default=4444} "
                                         "-instance {default=%s} -namespace {default=%s} -machine {default=%s} "
                                         "-iso cdrom -tapmaster {default=%s} -drive hd+1 instance://instance-id { eg. instance://i-1234 }",
                                         argv[0], instance.c_str(), nspace.c_str(), machine.c_str(), masterinterface.c_str());

    // Remember argv[0] is the path to the program, we want from argv[1] onwards
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]).find("-help") != std::string::npos)
        {
            std::cout << usage << std::endl;
            exit(EXIT_FAILURE);
        }
        
        if (std::string(argv[i]).find("-headless") != std::string::npos)
        {
            display = QEMU_DISPLAY::VNC;
        }

        if (std::string(argv[i]).find("-instance") != std::string::npos && (i + 1 < argc))
        {
            instance = argv[i + 1];
        }

        if (std::string(argv[i]).find("-machine") != std::string::npos && (i + 1 < argc))
        {
            machine = argv[i + 1];
        }

        if (std::string(argv[i]).find("-tapmaster") != std::string::npos && (i + 1 < argc))
        {
            masterinterface = argv[i + 1];
        }

        if (std::string(argv[i]).find("-iso") != std::string::npos && (i + 1 < argc))
        {

            // This allows us, to use different datastores, following this idea
            // -drive main:test-something-2.
            std::string datastore = default_isostore;
            std::string drivename = std::string(argv[i + 1]);
            const std::string delimiter = ":";

            if (drivename.find(delimiter) != std::string::npos)
            {
                datastore = drivename.substr(0, drivename.find(delimiter)); // remove the drivename-part.
                drivename = drivename.substr(drivename.find(delimiter) + 1); // remove the datastore-part.
            }

            std::string drive = m3_string_format("%s/%s.iso", datastore.c_str(), drivename.c_str());
            QEMU_iso(ctx, drive);
        }

        if (std::string(argv[i]).find("-drive") != std::string::npos && (i + 1 < argc))
        {
            // This allows us, to use different datastores, following this idea
            // -drive main:test-something-2.
            std::string datastore = default_datastore;
            std::string drivename = std::string(argv[i + 1]);
            const std::string delimiter = ":";

            if (drivename.find(delimiter) != std::string::npos)
            {
                datastore = drivename.substr(0, drivename.find(delimiter)); // remove the drivename-part.
                drivename = drivename.substr(drivename.find(delimiter) + 1); // remove the datastore-part.
            }

            auto it = std::find_if(datastores.begin(), datastores.end(), [&datastore](const std::tuple<std::string, std::string> &ct)
                                   { return datastore.starts_with(std::get<0>(ct)); });

            std::string drive = m3_string_format("%s/%s.img", std::get<1>(*it).c_str(), drivename.c_str());
            if (!fileExists(drive))
            {
                QEMU_allocate_drive(drive, disksize);
            }

            if (-1 == QEMU_drive(ctx, drive, driveid++))
            {
                exit(EXIT_FAILURE);
            }
        }

        if (std::string(argv[i]).find("-incoming") != std::string::npos && (i + 1 < argc))
        {
            QEMU_Accept_Incoming(ctx, std::atoi(argv[i + 1]));
        }

        if (std::string(argv[i]).find("-snapshot") != std::string::npos)
        {
            QEMU_ephimeral(ctx);
        }

        if (std::string(argv[i]).find("-namespace") != std::string::npos && (i + 1 < argc))
        {
            // Check, that namespace eixsts
            nspace = m3_string_format("/var/run/netns/%s", argv[i + 1]);
            if (!fileExists(nspace.c_str()))
            {
                std::cerr << "Error: Namespace " << nspace << " does, not exist." << std::endl;
                std::cout << usage << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        if (std::string(argv[i]).find("://") != std::string::npos)
        {
            const std::string delimiter = "://";
            std::string instanceid = std::string(argv[i]).substr(std::string(argv[i]).find(delimiter) + 3);

            auto it = std::find_if(drives.begin(), drives.end(), [&instanceid](const std::tuple<std::string, std::string> &ct)
                                   { return instanceid.starts_with(std::get<0>(ct)); });

            std::string absdrive = m3_string_format("%s/%s.img", std::get<1>(datastores.front()).c_str(), instanceid.c_str());

            if (it != drives.end())
            {
                std::tuple<std::string, std::string> conf = *it;
                std::string backingdrive = std::get<1>(conf);

                if (!fileExists(absdrive))
                {
                    QEMU_allocate_backed_drive(absdrive, disksize, m3_string_format("%s/%s.img", std::get<1>(datastores.front()).c_str(), backingdrive.c_str()));
                }

                QEMU_drive(ctx, absdrive, 0);

                mandatory = 1;
            }
            else
            {
                if (!fileExists(absdrive))
                {
                    QEMU_allocate_drive(absdrive, disksize);
                }
                QEMU_drive(ctx, absdrive, 0);
                mandatory = 1;
            }

            std::size_t str_hash = std::hash<std::string>{}(instanceid);
            std::string hostname = generatePrefixedUniqueString("i", str_hash, 8);
            std::string instance = generatePrefixedUniqueString("i", str_hash, 32);

            QEMU_cloud_init_default(ctx, hostname, instance);
        }
    }

    if (mandatory == 0)
    {
        std::cerr << "Error: Missing mandatory fields" << std::endl;
        std::cout << usage << std::endl;
        exit(EXIT_FAILURE);
    }

    pid_t daemon = fork();
    if (daemon == 0)
    {
        QEMU_instance(ctx, instance);
        QEMU_display(ctx, display);
        QEMU_machine(ctx, machine);
        QEMU_notified_started(ctx);
        QEMU_set_namespace(nspace);
        std::string tapdevice = QEMU_allocate_macvtap(ctx, masterinterface);
        QEMU_set_default_namespace();

        pid_t child = fork();
        if (child == 0)
        {
            QEMU_launch(ctx, true); // where qemu-launch, does block, ie we can wait for it.
        }
        else
        {
            int status = 0;
            pid_t w = waitpid(child, &status, WUNTRACED | WCONTINUED);
            if (WIFEXITED(status))
            {
                QEMU_set_namespace(nspace);
                QEMU_delete_link(ctx, tapdevice);
                QEMU_set_default_namespace();
                QEMU_notified_exited(ctx);
            }

            return EXIT_SUCCESS;
        }
    }
}