# -*- coding: utf-8 -*-
"""
CMD BOX - Knowledge Base & Training Dataset
Chứa toàn bộ bộ từ khóa nhận diện, hướng dẫn sử dụng và dữ liệu giao tiếp vui nhộn.
"""

from typing import Dict, Any, List

# ==============================================================================
# KHO TRI THỨC VÀ TỪ KHÓA TÍNH NĂNG CMD BOX
# ==============================================================================
FEATURES_DATABASE: Dict[str, Dict[str, Any]] = {
    "don_rac": {
        "title": "Dọn dẹp rác & Giải phóng ổ đĩa (Disk Cleanup PRO)",
        "menu_tag": "[Menu 1 -> 1] Dọn rác chuyên sâu PRO",
        "keywords": [
            "don rac", "xoa rac", "rac may tinh", "day o c", "full o c", "day o dia", "o c bi day",
            "o c bao do", "het dung luong o c", "o cung full", "file temp", "xoa temp", "thung rac",
            "recycle bin", "giai phong bo nho", "clean disk", "disk cleanup", "cleanmgr", "cache",
            "xoa file tam", "giam dung luong o c", "nang bo nho", "don dep", "clear cache"
        ],
        "diagnosis": "Ổ đĩa C: bị đầy bởi tệp tin tạm (Temp), bộ nhớ đệm cache và file rác hệ thống.",
        "steps": [
            "Chọn [1] Bảo trì & Tối ưu trên Menu chính.",
            "Chọn [1] Dọn rác chuyên sâu PRO để tự động dọn sạch."
        ],
        "quick_action": {"name": "Thùng rác (Recycle Bin)", "command": "start shell:RecycleBinFolder"}
    },
    "may_lag": {
        "title": "Tối ưu hóa tốc độ, khắc phục máy giật lag & khởi động chậm",
        "menu_tag": "[Menu 1 -> 2, 3, 6] Tối ưu hóa hệ thống",
        "keywords": [
            "may lag", "may cham", "may do", "treo may", "giat lag", "khoi dong lau", "boot cham",
            "cpu cao", "ram cao", "full disk", "tang toc may", "tang toc do", "toi uu may", "boost fps",
            "choi game lag", "nang may", "tang toc win", "ultimate performance", "tat app chay ngam",
            "startup", "khoi dong cung win", "dich vu win", "services", "muot ma", "speed up"
        ],
        "diagnosis": "Nhiều ứng dụng chạy ngầm khi khởi động hoặc các dịch vụ Windows chưa tối ưu.",
        "steps": [
            "Tắt app chạy ngầm: Chọn [1] -> [2] Tắt ứng dụng khởi động.",
            "Tối ưu dịch vụ: Chọn [1] -> [3] Tối ưu dịch vụ Windows.",
            "Bật hiệu năng cao: Chọn [1] -> [6] Tối ưu hóa tổng thể hệ thống."
        ],
        "quick_action": {"name": "Task Manager", "command": "start taskmgr"}
    },
    "win_update": {
        "title": "Sửa lỗi Windows Update không cập nhật được",
        "menu_tag": "[Menu 1 -> 5] Sửa lỗi Windows Update",
        "keywords": [
            "win update", "windows update", "loi update", "khong update duoc", "cap nhat win",
            "update fail", "fix update", "loi cap nhat", "dung update", "bits", "wuauserv",
            "0x80240020", "0x80070422", "update window bi loi"
        ],
        "diagnosis": "Cache tải bản cập nhật Windows bị lỗi hoặc tiến trình Windows Update bị treo.",
        "steps": [
            "Chọn [1] Bảo trì & Tối ưu.",
            "Chọn [5] Sửa lỗi Windows Update để reset toàn bộ dịch vụ update."
        ],
        "quick_action": {"name": "Windows Update Settings", "command": "start ms-settings:windowsupdate"}
    },
    "taskbar_win11": {
        "title": "Tùy biến Taskbar & Giao diện Windows 11",
        "menu_tag": "[Menu 1 -> 4] Chỉnh giao diện & Taskbar Win 11",
        "keywords": [
            "taskbar", "thanh taskbar", "win 11", "can le taskbar", "taskbar can trai", "menu chuot phai",
            "chuot phai win 10", "chuot phai classic", "context menu", "chinh giao dien", "taskbar giua"
        ],
        "diagnosis": "Cần khôi phục menu chuột phải cổ điển Win 10 hoặc căn lề trái thanh Taskbar Win 11.",
        "steps": [
            "Chọn [1] Bảo trì & Tối ưu.",
            "Chọn [4] Chỉnh giao diện & Taskbar Win 11."
        ],
        "quick_action": None
    },
    "sua_mang": {
        "title": "Sửa lỗi kết nối & Khôi phục mạng toàn diện (Network Repair)",
        "menu_tag": "[Menu 2 -> 2] Sửa lỗi & Khôi phục mạng toàn diện",
        "keywords": [
            "mat mang", "sua mang", "loi mang", "khong vao duoc web", "rot mang", "wifi cham", "wifi lag",
            "ping cao", "dns", "winsock", "reset mang", "socket 10013", "wsaeacces", "khong ket noi duoc",
            "cham mang", "mang chap chon", "fix internet", "repair network", "flushdns", "ipconfig"
        ],
        "diagnosis": "Xung đột cổng socket mạng, lỗi Winsock Catalog hoặc cache DNS bị lỗi.",
        "steps": [
            "Chọn [2] Mạng & Bảo mật.",
            "Chọn [2] Sửa lỗi & Khôi phục mạng toàn diện (8 bước chuyên sâu)."
        ],
        "quick_action": {"name": "Cài đặt Kết nối Mạng", "command": "ncpa.cpl"}
    },
    "wifi_pass": {
        "title": "Xem lại mật khẩu Wi-Fi đã lưu trên máy tính",
        "menu_tag": "[Menu 2 -> 5] Xem danh sách mật khẩu Wi-Fi đã lưu",
        "keywords": [
            "pass wifi", "mat khau wifi", "xem pass wifi", "lay pass wifi", "tim pass wifi",
            "quen pass wifi", "chia se wifi", "show wifi password", "wlan password", "coi pass wifi"
        ],
        "diagnosis": "Cần trích xuất lại danh sách mật khẩu Wi-Fi đã từng kết nối trước đây.",
        "steps": [
            "Chọn [2] Mạng & Bảo mật.",
            "Chọn [5] Xem danh sách mật khẩu Wi-Fi đã lưu."
        ],
        "quick_action": None
    },
    "bao_mat": {
        "title": "Kích hoạt Lá chắn bảo mật toàn diện & Tường lửa",
        "menu_tag": "[Menu 2 -> 3] Kích hoạt Lá chắn bảo mật toàn diện",
        "keywords": [
            "bao mat", "virus", "quet virus", "diet virus", "tuong lua", "firewall", "defender",
            "chan cong", "khoa cong 445", "khoa cong 135", "chan hacker", "trojan", "ma doc",
            "hosts", "file hosts", "chan web", "an toan may tinh", "security"
        ],
        "diagnosis": "Cần quét bảo mật, bật tường lửa Defender và chặn các cổng mạng nguy hiểm.",
        "steps": [
            "Bảo mật hệ thống: Chọn [2] Mạng & Bảo mật -> [3] Kích hoạt Lá chắn bảo mật.",
            "Kiểm tra file Hosts: Chọn [2] -> [6] Quét & Bảo vệ tập tin Hosts."
        ],
        "quick_action": {"name": "Windows Security", "command": "start windowsdefender:"}
    },
    "auto_click": {
        "title": "Công cụ Auto Click chuột tự động theo tọa độ",
        "menu_tag": "[Menu 3 -> 1] Auto Click chuột",
        "keywords": [
            "auto click", "click tu dong", "nhap chuot tu dong", "bam chuot", "clicker", "spam click",
            "treo game", "auto nhap chuot", "click nhanh", "click chuot lien tuc", "auto mouse"
        ],
        "diagnosis": "Cần tự động click chuột liên tục hoặc click theo tọa độ có sẵn phím ngắt khẩn cấp.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [1] Auto Click chuột (Nhấn ESC hoặc F6 để dừng khẩn cấp)."
        ],
        "quick_action": None
    },
    "spam_text": {
        "title": "Công cụ Spam Text / Gửi tin nhắn tự động hàng loạt",
        "menu_tag": "[Menu 3 -> 2] Spam Text",
        "keywords": [
            "spam text", "spam tin nhan", "gui tin tu dong", "spam mess", "spam zalo", "nhan tin tu dong",
            "gui tin lien tuc", "auto chat", "spam tin", "gui text hang loat"
        ],
        "diagnosis": "Cần tự động gửi tin nhắn lặp lại có dấu tiếng Việt với phím ngắt ESC/F6.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [2] Spam Text."
        ],
        "quick_action": None
    },
    "auto_paste": {
        "title": "Công cụ Auto Paste dữ liệu nhiều dòng tự động",
        "menu_tag": "[Menu 3 -> 3] Auto Paste dữ liệu nhiều dòng",
        "keywords": [
            "auto paste", "dan tu dong", "paste nhieu dong", "dien form tu dong", "paste lien tuc",
            "nhap danh sach", "auto paste data", "tu dong dan text"
        ],
        "diagnosis": "Cần dán tự động danh sách nhiều dòng vào phần mềm hoặc bảng biểu.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [3] Auto Paste dữ liệu nhiều dòng."
        ],
        "quick_action": None
    },
    "cai_app": {
        "title": "Trình tải & Cài đặt phần mềm tự động (Auto Installer)",
        "menu_tag": "[Menu 3 -> 4] Tải & Cài đặt phần mềm tự động",
        "keywords": [
            "cai phan mem", "tai app", "cai app", "tai phan mem", "tai chrome", "tai zalo", "tai vscode",
            "tai discord", "tai git", "tai unikey", "cai dat", "download app", "winget", "install software"
        ],
        "diagnosis": "Cần tải và cài đặt nhanh các ứng dụng phổ biến chỉ với 1 click.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [4] Tải & Cài đặt phần mềm tự động."
        ],
        "quick_action": None
    },
    "bloatware": {
        "title": "Gỡ bỏ ứng dụng rác mặc định (Bloatware Windows)",
        "menu_tag": "[Menu 3 -> 5] Gỡ bỏ ứng dụng rác mặc định",
        "keywords": [
            "go bloatware", "go app rac", "xoa app mac dinh", "go ung dung mac dinh", "xoa bloatware",
            "uninstall bloatware", "xoa app win", "go phan mem thua", "go bot ung dung"
        ],
        "diagnosis": "Các ứng dụng mặc định không cần thiết trên Windows đang chiếm dung lượng và chạy ngầm.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [5] Gỡ bỏ ứng dụng rác mặc định."
        ],
        "quick_action": {"name": "Cài đặt Apps Windows", "command": "start ms-settings:appsfeatures"}
    },
    "pin_laptop": {
        "title": "Kiểm tra độ chai & Sức khỏe Pin Laptop (Battery Report)",
        "menu_tag": "[Menu 3 -> 6] Kiểm tra độ chai Pin Laptop",
        "keywords": [
            "pin", "chai pin", "kiem tra pin", "do chai pin", "pin laptop", "het pin", "tut pin",
            "hao pin", "pin sut nhanh", "battery report", "suc khoe pin", "xem pin", "check pin"
        ],
        "diagnosis": "Cần kiểm tra dung lượng pin thực tế, số chu kỳ sạc và độ chai của pin laptop.",
        "steps": [
            "Chọn [3] Công cụ tiện ích.",
            "Chọn [6] Kiểm tra độ chai Pin Laptop (tự động xuất file báo cáo HTML)."
        ],
        "quick_action": None
    },
    "nen_media": {
        "title": "Nén dung lượng Video / Ảnh (Tăng tốc GPU NVENC/Intel/AMD)",
        "menu_tag": "[Menu 4 -> 1] Nén dung lượng Video / Ảnh",
        "keywords": [
            "nen video", "giam size video", "video nang qua", "giam dung luong video", "nen anh",
            "giam size anh", "anh nang qua", "video qua lon", "bop dung luong", "giam mb", "compress video",
            "video nang khong gui duoc zalo", "compress image", "nen clip", "ha dung luong"
        ],
        "diagnosis": "Tệp video hoặc hình ảnh có dung lượng lớn, cần nén nhẹ mà vẫn giữ độ sắc nét.",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [1] Nén dung lượng Video / Ảnh."
        ],
        "quick_action": None
    },
    "lam_net": {
        "title": "Làm nét & Khử nhiễu Video / Ảnh (Enhancement)",
        "menu_tag": "[Menu 4 -> 2] Làm nét Video / Ảnh",
        "keywords": [
            "lam net", "lam ro", "upscale", "video mo", "anh mo", "mo qua", "khu nhieu", "tang do net",
            "lam net anh", "lam net video", "anh bi vo", "video bi vo", "net hon", "ro hon"
        ],
        "diagnosis": "Video hoặc hình ảnh bị mờ, nhiễu hạt, cần bộ lọc tăng chi tiết và độ tương phản.",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [2] Làm nét Video / Ảnh (chọn mức từ Tự nhiên đến Siêu nét PRO)."
        ],
        "quick_action": None
    },
    "mp4_to_mp3": {
        "title": "Chuyển đổi Video sang Audio (MP4 -> MP3)",
        "menu_tag": "[Menu 4 -> 3] MP4 -> MP3",
        "keywords": [
            "mp4 sang mp3", "tach nhac", "lay audio", "chuyen video thanh nhac", "rut mp3", "lay tieng",
            "tach am thanh", "trich xuat nhac", "mp4 to mp3", "xuat audio 320kbps", "lay nhac tu video"
        ],
        "diagnosis": "Cần trích xuất dải âm thanh chất lượng cao (320kbps) từ video.",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [3] MP4 -> MP3."
        ],
        "quick_action": None
    },
    "toc_do_video": {
        "title": "Thay đổi tốc độ phát Video (Tua nhanh / Slow-motion)",
        "menu_tag": "[Menu 4 -> 4] Thay đổi tốc độ Video",
        "keywords": [
            "toc do video", "tua nhanh", "slow motion", "quay cham", "giam toc do video", "tang toc do video",
            "chinh toc do", "video speed", "tua video", "chay nhanh", "chay cham"
        ],
        "diagnosis": "Cần chỉnh tốc độ video từ 0.5x đến 2.0x mà vẫn giữ nguyên cao độ âm thanh.",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [4] Thay đổi tốc độ Video."
        ],
        "quick_action": None
    },
    "doi_dinh_dang": {
        "title": "Đổi đuôi & Chuyển đổi định dạng Media hàng loạt",
        "menu_tag": "[Menu 4 -> 5] Đổi đuôi định dạng Media",
        "keywords": [
            "doi duoi", "convert", "chuyen duoi", "mkv sang mp4", "png sang jpg", "webp sang png",
            "doi format", "chuyen dinh dang", "doi duoi video", "doi duoi anh", "convert file"
        ],
        "diagnosis": "Cần chuyển đổi qua lại giữa các định dạng media (MP4, MKV, PNG, JPG, WEBP...).",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [5] Đổi đuôi định dạng Media."
        ],
        "quick_action": None
    },
    "an_file": {
        "title": "Ẩn file trong file (Steganography - Giấu tệp tin mật)",
        "menu_tag": "[Menu 4 -> 7] Ẩn file trong file",
        "keywords": [
            "an file", "giau file", "steganography", "giau file vao anh", "giau file vao video",
            "an tap tin", "giau du lieu", "ma hoa file vao anh", "hide file"
        ],
        "diagnosis": "Cần ngụy trang tập tin nhạy cảm vào bên trong một file ảnh hoặc video.",
        "steps": [
            "Chọn [4] Xử lý Media.",
            "Chọn [7] Ẩn file trong file."
        ],
        "quick_action": None
    }
}


# ==============================================================================
# DỮ LIỆU CHÀO HỎI & XÃ GIAO
# ==============================================================================
GREETINGS_RESPONSES: List[str] = [
    "Chào bạn! Tôi là Cyber Assistant của CMD BOX. Bạn đang cần tối ưu máy tính, xử lý media hay cần tôi hỗ trợ việc gì?",
    "Xin chào! Rất vui được hỗ trợ bạn. Hãy cho tôi biết tình trạng máy tính hoặc chức năng bạn muốn tìm kiếm nhé!",
    "Hello! Trợ lý ảo CMD BOX luôn sẵn sàng. Bạn có thể hỏi về dọn rác, sửa mạng, nén video, pin laptop hoặc gõ 'scan' để kiểm tra máy."
]


# ==============================================================================
# EASTER EGGS & FUN DATA (QUẺ BÓI IT, ROAST, KHEN NGỢI)
# ==============================================================================
IT_FORTUNES: List[str] = [
    "✨ [ĐẠI CÁT]: Hôm nay code chạy mượt mà, build 1 phát ăn ngay không một warning!",
    "🌟 [CÁT]: Sếp khen bạn làm việc chăm chỉ, khả năng sắp được tăng lương hoặc nhận thưởng nóng.",
    "⚠️ [TIỂU HUNG]: Đừng bao giờ đụng vào code chạy trên Production vào chiều thứ 6!",
    "🔮 [QUẺ ĐỘC]: Bug hôm nay là một tính năng chưa được tài liệu hóa (Feature, not a Bug)!",
    "⚡ [BÁC HỌC]: Khi code không chạy, hãy khởi động lại máy. 90% lỗi tự biến mất!",
    "💡 [TRÍ TUỆ]: Cà phê + CMD BOX = Năng suất x3 lần người thường!",
    "🍀 [MAY MẮN]: Thần may mắn đang ở cạnh bạn: Hôm nay không gặp lỗi 0x80070005!"
]

ROAST_QUOTES: List[str] = [
    "🔥 Máy tính của bạn đang gào thét: 'Cứu tôi với, tab Chrome đang ăn hết RAM rồi!'",
    "🔥 Ổ đĩa C của bạn đang đỏ như mặt trời mùa hạ. Mau mở Menu 1 -> 1 dọn rác gấp đi nha!",
    "🔥 Máy bạn chạy chậm đến mức ốc sên lướt qua còn ngoái lại cười khẩy kìa! Bật Ultimate Performance lên đi!",
    "🔥 Quạt tản nhiệt của máy bạn đang chuẩn bị cất cánh như máy bay Boeing 747 đấy!"
]

FLATTER_QUOTES: List[str] = [
    "👑 Bạn là người dùng tuyệt vời và thông minh nhất mà tôi từng được phục vụ! Chúc bạn ngày mới đỉnh chóp!",
    "💎 Nhìn cách bạn gõ phím là biết ngay một cao thủ am hiểu công nghệ rồi!",
    "🚀 Năng lượng của bạn hôm nay quá đỉnh, chắc chắn sẽ giải quyết xong mọi công việc trong chớp mắt!"
]
