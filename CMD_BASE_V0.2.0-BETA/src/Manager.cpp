#include "../include/Manager.h"
#include <iostream>
#include <vector>

using namespace std;

Manager::Manager(SystemCore &s) : sc(s) {}

bool Manager::ServiceControlAPI(string serviceName, DWORD startupType, bool stopService) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceA(scm, serviceName.c_str(), SERVICE_CHANGE_CONFIG | SERVICE_STOP | SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }

    bool configSuccess = ChangeServiceConfigA(svc, SERVICE_NO_CHANGE, startupType, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (stopService) {
        SERVICE_STATUS status;
        ControlService(svc, SERVICE_CONTROL_STOP, &status);
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return configSuccess;
}

void Manager::turnOffServicesMenu() {
    struct SvcInfo { string name; string desc; };
    vector<SvcInfo> targetSvcs = {
        // Windows Update & Bảo trì (Bổ sung mới)
        {"wuauserv", "Windows Update (Cập nhật hệ thống - Ngăn tự động update)"},
        {"UsoSvc", "Update Orchestrator Service (Điều phối cập nhật Windows)"},
        {"WaaSMedicSvc", "Windows Update Medic Service (Tự động sửa lỗi/bật lại Update)"},
        {"WerSvc", "Windows Error Reporting Service (Báo cáo lỗi về Microsoft)"},
        // Widgets, Notifications & Web Services
        {"WpnService", "Windows Push Notifications System (Hệ thống thông báo/Widgets)"},
        {"WpnUserService", "Windows Push Notifications User Service (Tắt WebView2 ngầm)"},
        {"WpcSvc", "Parental Controls (Quản lý trẻ em / Tính năng gia đình)"},
        // Xbox & Gaming Services (Dành cho máy không chơi game Xbox)
        {"XblAuthManager", "Xbox Live Auth Manager (Xác thực tài khoản Xbox)"},
        {"XblGameSave", "Xbox Live Game Save (Đồng bộ lưu lưu dữ liệu game)"},
        {"XboxNetApiSvc", "Xbox Live Networking Service (Mạng Xbox)"},
        // Telemetry & Thu thập dữ liệu (Theo dõi người dùng)
        {"DiagTrack", "Connected User Experiences and Telemetry (Thu thập dữ liệu)"},
        {"dmwappushservice", "WAP Push Message Routing Service (Định tuyến tin nhắn trắc lượng)"},
        // Trình duyệt & Cập nhật bên thứ ba
        {"EdgeUpdate", "Microsoft Edge Update Service (Cập nhật trình duyệt ngầm)"},
        // Các dịch vụ phần cứng / Tính năng khác không cần thiết (Bổ sung mới)
        {"wisvc", "Windows Insider Service (Dịch vụ chương trình dùng thử Insider)"},
        {"Spooler", "Print Spooler (Dịch vụ in ấn - Nên tắt nếu không dùng máy in)"},
        {"BthServ", "Bluetooth Support Service (Nên tắt nếu máy PC không có Bluetooth)"},
        {"MapsBroker", "Downloaded Maps Manager (Quản lý bản đồ ngoại tuyến)"},
        {"RemoteRegistry", "Remote Registry (Cho phép chỉnh sửa Registry từ xa - Nguy cơ bảo mật)"},
        {"SysMain", "Superfetch / SysMain (Tối ưu ổ đĩa - Nên tắt hoàn toàn nếu dùng SSD)"},
        {"WalletService", "Wallet Service (Ví điện tử và thanh toán của Windows)"}
    };

    cout << "====================================================================\n";
    cout << "       DANH SÁCH CÁC DỊCH VỤ SẼ ĐƯỢC TÓI ƯU HÓA HỆ THỐNG:\n";
    for (const auto &s : targetSvcs) {
        cout << "   - " << s.desc << " [" << s.name << "]\n";
    }
    cout << "====================================================================\n";

    cout << "[?] Bạn muốn cấu hình các dịch vụ này theo cách nào?\n";
    cout << "[1] Tối ưu thụ động (Manual - Chỉ chạy khi hệ thống yêu cầu)\n";
    cout << "[2] Vô hiệu hóa     (Disabled - Tắt hoàn toàn, tiết kiệm RAM/CPU)\n";
    cout << "[0] Hủy bỏ tiến trình\n";
    int choice = sc.readInt("Chọn phương thức: ");
    if (choice == 0) return;

    DWORD startType = (choice == 1) ? SERVICE_DEMAND_START : SERVICE_DISABLED;
    string modeName = (choice == 1) ? "MANUAL" : "DISABLED";

    cout << "\n[*] Đang thực thi cấu hình dịch vụ...\n";
    int successCount = 0;
    
    for (const auto &s : targetSvcs) {
        if (ServiceControlAPI(s.name, startType, true)) {
            cout << "[OK] -> " << modeName << ": " << s.name << "\n";
            successCount++;
        } else {
            cout << "[!] Thất bại: " << s.name << " (Có thể dịch vụ không tồn tại hoặc cần chạy quyền Administrator)\n";
        }
    }
    
    cout << "\n[SUCCESS] Hoàn tất tiến trình! Đã tối ưu thành công " << successCount << "/" << targetSvcs.size() << " dịch vụ.\n";
}