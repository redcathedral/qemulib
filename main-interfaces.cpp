
#include <qemu-interfaces.hpp>

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

int main(int argc, char *argv[])
{
    bool verbose = false;
    std::string topic = "";
    std::string reservation = "";
    std::string device = "";
    int force = 0;
    bool all = true;

    std::string usage = m3_string_format("usage(): %s (-help) (-device) (reservation://%s)",
                                         argv[0], reservation.c_str());

    for (int i = 1; i < argc; ++i)
    { // Remember argv[0] is the path to the program, we want from argv[1] onwards

        if (std::string(argv[i]).find("-help") != std::string::npos)
        {
            std::cout << usage << std::endl;
            exit(EXIT_FAILURE);
        }

        if (std::string(argv[i]).find("-device") != std::string::npos && (i + 1 < argc))
        {
            device = argv[i + 1];
        }

        if (std::string(argv[i]).find("-v") != std::string::npos)
        {
            verbose = true;
        }

        if (std::string(argv[i]).find("://") != std::string::npos)
        {
            const std::string delimiter = "://";
            reservation = std::string(argv[i]).substr(std::string(argv[i]).find(delimiter) + 3);
            all = false;
        }
    }

    std::vector<std::string> reservations = QEMU_get_reservations();
    std::for_each(reservations.begin(), reservations.end(), [force, all, &reservation, &device](std::string &res) {
        if (res == reservation || all == true) {
            std::cout << "reservation: " << res << std::endl;
            std::string error;
            json11::Json json = json11::Json::parse(QEMU_qga_qinterfaces(res), error);
            json11::Json::array node = json["return"].array_items();

            std::for_each(node.begin(), node.end(), [&device](const json11::Json &nd) { 
                json11::Json::array ips =  nd["ip-addresses"].array_items() ;

                if (nd["name"].string_value().starts_with(device)) {
                    std::cout << " device: " << nd["name"].string_value() << std::endl;
                    std::for_each(ips.begin(), ips.end(), [](const json11::Json &ip) { 
                        
                        if ( ip["ip-address-type"].string_value().starts_with("ipv4")) {
                            std::cout << "  ip: " << ip["ip-address"].string_value() << std::endl;
                        }
                    });
                } 
            });
        } 
    });
    return EXIT_SUCCESS;
}