#include "version/ProfileLoader.hpp"
#include "version/AutoDetector.hpp"
#include "ipc/Bridge.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::string profilesDir = "profiles";
    std::string ipcSocket = "\\\\.\\pipe\\zucks_engine";
    for (int i = 1; i < argc - 1; ++i) {
        std::string a = argv[i];
        if (a == "--profiles") profilesDir = argv[++i];
        if (a == "--ipc")     ipcSocket   = argv[++i];
    }

    auto profiles = zucks::ProfileLoader::LoadDirectory(profilesDir);
    zucks::AutoDetector detector;
    detector.SetProfiles(profiles);
    std::cout << "[Zucks] Loaded " << profiles.size() << " profiles.\n";

    zucks::Bridge bridge;
    bridge.Start(ipcSocket, &detector);

    std::cout << "[Zucks] Engine running. IPC: " << ipcSocket << "\n";
    bridge.Wait();
    return 0;
}