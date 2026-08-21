# -*- coding: utf-8 -*-
"""
CMD BOX - Cyber Virtual Assistant (Trợ lý ảo AI Thông minh - Hybrid AI)
- Tích hợp Gemini 1.5 Flash API Online (nếu có key) để trò chuyện và trả lời tự nhiên 100%.
- Tự động fallback về Vector N-gram & Small Talk Engine Offline khi không có key hoặc mất mạng.
- Hỗ trợ chào hỏi, giao tiếp thông minh, chẩn đoán lỗi máy tính và điều hướng CMD BOX.
"""

import sys
import os
import re
import math
import json
import time
import random
import subprocess
import unicodedata
import urllib.request
from typing import Optional, List, Dict, Any, Tuple

# Cấu hình UTF-8 & Tiêu đề console Windows
if sys.platform == "win32":
    try:
        os.system("chcp 65001 >nul")
        os.system("title CMD BOX - Trợ lý ảo AI")
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stdin.reconfigure(encoding='utf-8')
    except Exception:
        pass


# ANSI Color & Style Codes
class C:
    RESET       = "\033[0m"
    BOLD        = "\033[1m"
    DIM         = "\033[2m"
    CYAN        = "\033[96m"
    BLUE        = "\033[94m"
    GREEN       = "\033[92m"
    YELLOW      = "\033[93m"
    RED         = "\033[91m"
    MAGENTA     = "\033[95m"
    WHITE       = "\033[97m"
    GRAY        = "\033[90m"


def remove_accents(input_str: str) -> str:
    """Loại bỏ dấu tiếng Việt chuẩn NFKD."""
    nfkd = unicodedata.normalize('NFKD', input_str)
    return u"".join([c for c in nfkd if not unicodedata.combining(c)]).replace('đ', 'd').replace('Đ', 'D').lower()


# ==============================================================================
# QUẢN LÝ CẤU HÌNH API KEY (ONLINE CLOUD AI)
# ==============================================================================
CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json")

def load_api_key() -> Optional[str]:
    if os.getenv("GEMINI_API_KEY"):
        return os.getenv("GEMINI_API_KEY")
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                data = json.load(f)
                return data.get("gemini_api_key")
        except Exception:
            pass
    return None

def save_api_key(key: str) -> bool:
    try:
        data = {}
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                    data = json.load(f)
            except Exception:
                pass
        data["gemini_api_key"] = key.strip()
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        return True
    except Exception:
        return False


def call_gemini_api(api_key: str, user_prompt: str) -> Optional[str]:
    """Gọi trực tiếp Google Gemini 1.5 Flash API miễn phí."""
    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={api_key}"
    
    system_instruction = (
        "Bạn là Trợ lý ảo AI của ứng dụng Windows CMD BOX.\n"
        "Hãy trả lời tự nhiên, thân thiện và thông minh bằng tiếng Việt.\n"
        "Nếu người dùng chào hỏi hoặc hỏi xã giao, hãy chào lại vui vẻ và giới thiệu ngắn gọn.\n"
        "Nếu người dùng hỏi về vấn đề máy tính hoặc cần dùng tính năng, hãy giải thích ngắn và chỉ dẫn đúng Menu CMD BOX:\n"
        "- [Menu 1 -> 1] Dọn rác chuyên sâu PRO (dọn rác, file temp, cache, đầy ổ C)\n"
        "- [Menu 1 -> 2] Tắt ứng dụng khởi động (tăng tốc boot máy)\n"
        "- [Menu 1 -> 3] Tối ưu dịch vụ Windows\n"
        "- [Menu 1 -> 4] Chỉnh giao diện & Taskbar Win 11\n"
        "- [Menu 1 -> 5] Sửa lỗi Windows Update\n"
        "- [Menu 1 -> 6] Tối ưu hóa tổng thể hệ thống (Bật Ultimate Performance, Registry)\n"
        "- [Menu 2 -> 1] Xem thông tin mạng chi tiết (IP LAN, Public IP, DNS)\n"
        "- [Menu 2 -> 2] Sửa lỗi & Khôi phục mạng toàn diện (8 bước, reset Winsock, DNS, port 10013)\n"
        "- [Menu 2 -> 3] Kích hoạt Lá chắn bảo mật toàn diện (Defender, Firewall, chặn cổng 445/135)\n"
        "- [Menu 2 -> 4] Kiểm tra trạng thái bảo mật\n"
        "- [Menu 2 -> 5] Xem danh sách mật khẩu Wi-Fi đã lưu\n"
        "- [Menu 2 -> 6] Quét & Bảo vệ tập tin Hosts\n"
        "- [Menu 3 -> 1] Auto Click chuột theo tọa độ\n"
        "- [Menu 3 -> 2] Spam Text / Gửi tin nhắn tự động\n"
        "- [Menu 3 -> 3] Auto Paste dữ liệu nhiều dòng\n"
        "- [Menu 3 -> 4] Tải & Cài đặt phần mềm tự động (Chrome, Zalo, VS Code, Git...)\n"
        "- [Menu 3 -> 5] Gỡ bỏ ứng dụng rác mặc định (Bloatware)\n"
        "- [Menu 3 -> 6] Kiểm tra độ chai Pin Laptop (Battery Report)\n"
        "- [Menu 4 -> 1] Nén dung lượng Video / Ảnh (GPU NVENC/Intel/AMD)\n"
        "- [Menu 4 -> 2] Làm nét Video / Ảnh (Upscale, khử nhiễu)\n"
        "- [Menu 4 -> 3] Mp4 sang Mp3 (Trích xuất audio 320kbps)\n"
        "- [Menu 4 -> 4] Thay đổi tốc độ Video (0.5x - 2.0x)\n"
        "- [Menu 4 -> 5] Đổi đuôi định dạng Media (MP4, MKV, PNG, JPG, WEBP...)\n"
        "- [Menu 4 -> 6] Chuẩn hóa tên file trong thư mục\n"
        "- [Menu 4 -> 7] Ẩn file trong file (Steganography)"
    )

    payload = {
        "contents": [
            {
                "parts": [
                    {"text": system_instruction + "\n\n[Người dùng hỏi]: " + user_prompt}
                ]
            }
        ],
        "generationConfig": {
            "temperature": 0.4,
            "maxOutputTokens": 500
        }
    }

    try:
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=5) as response:
            result = json.loads(response.read().decode("utf-8"))
            return result["candidates"][0]["content"]["parts"][0]["text"].strip()
    except Exception:
        return None


# ==============================================================================
# XỬ LÝ CHÀO HỎI & XÃ GIAO (OFFLINE SMALL TALK ENGINE)
# ==============================================================================
GREETINGS_RESPONSES = [
    "Chào bạn! Tôi là Cyber Assistant của CMD BOX. Bạn đang cần tối ưu máy tính, xử lý media hay cần tôi hỗ trợ việc gì?",
    "Xin chào! Rất vui được hỗ trợ bạn. Hãy cho tôi biết tình trạng máy tính hoặc chức năng bạn muốn tìm kiếm nhé!",
    "Hello! Trợ lý ảo CMD BOX luôn sẵn sàng. Bạn có thể hỏi về dọn rác, sửa mạng, nén video, pin laptop hoặc gõ 'scan' để kiểm tra máy."
]

def handle_small_talk(query_norm: str) -> Optional[str]:
    q = query_norm.strip()
    
    # 1. Hỏi tên người dùng / Thông tin cá nhân trên máy
    if any(k in q for k in ["toi ten gi", "ten toi la gi", "toi la ai", "biet toi ten gi", "ten nguoi dung", "user name", "username", "ten may", "ten may tinh", "may tinh ten gi", "thiet bi ten gi"]):
        user = os.getenv("USERNAME", "User")
        pc = os.getenv("COMPUTERNAME", "PC")
        return f"Bạn đang đăng nhập với tài khoản người dùng là {C.YELLOW}{C.BOLD}{user}{C.RESET} trên máy tính {C.CYAN}{pc}{C.RESET}!"

    # 2. Chào hỏi
    if any(q == k or q.startswith(k + " ") for k in ["chao", "xin chao", "hello", "hi", "helo", "alo", "chao ban", "chao bot", "chao tro ly", "hi bot", "chao buoi sang", "chao buoi toi", "2"]):
        return random.choice(GREETINGS_RESPONSES)
    
    # 3. Hỏi danh tính bot
    if any(k in q for k in ["ban la ai", "ai day", "ai do", "ten ban la gi", "ban ten gi", "gioi thieu ban than", "who are you", "tro ly gi"]):
        return (
            "Tôi là Cyber Assistant — Trợ lý ảo AI của phần mềm CMD BOX!\n"
            "Tôi có thể giúp bạn tự động chẩn đoán lỗi Windows, hướng dẫn tối ưu máy tính, nén video/ảnh, sửa mạng và nhiều tiện ích khác."
        )

    # 4. Hỏi thời gian / Ngày giờ
    if any(k in q for k in ["may gio", "gio may", "ngay may", "hom nay ngay may", "thoi gian", "time"]):
        now_str = time.strftime("%H:%M:%S - Ngày %d/%m/%Y")
        return f"Bây giờ là: {C.CYAN}{C.BOLD}{now_str}{C.RESET}"

    # 5. Hỏi thăm / Xã giao
    if any(k in q for k in ["khoe khong", "ban khoe khong", "the nao roi", "co khoe khong"]):
        return "Tôi là AI nên luôn luôn tràn đầy 100% năng lượng và sẵn sàng phục vụ bạn nè! Hôm nay máy tính của bạn hoạt động ổn chứ?"

    # 6. Khen ngợi
    if any(k in q for k in ["gioi qua", "thong minh", "hay qua", "vip", "dinh", "pro"]):
        return "Cảm ơn bạn nhiều nhé! Tôi luôn cố gắng để hỗ trợ bạn tối ưu và sử dụng máy tính hiệu quả nhất. ✨"

    # 7. Cảm ơn / Tạm biệt
    if any(q == k or q.startswith(k + " ") for k in ["cam on", "thank", "thanks", "cam on ban", "ok cam on", "tks"]):
        return "Không có gì nè! Nếu cần hỗ trợ thêm bất cứ điều gì về máy tính, cứ nhắn cho tôi nhé. Chúc bạn một ngày tốt lành! 🚀"

    # 8. Trợ giúp / Hướng dẫn
    if any(k in q for k in ["help", "huong dan", "tro giup", "lam duoc gi", "chuc nang", "co gi"]):
        return (
            "💡 Tôi có thể giúp bạn:\n"
            " • Chẩn đoán & hướng dẫn sửa máy: gõ 'máy lag', 'hết pin', 'đầy ổ cứng c', 'sửa mạng', 'nén video'...\n"
            " • Lệnh nhanh hệ thống: gõ 'scan' (kiểm tra máy), 'ping' (đo mạng), 'pass' (tạo mật khẩu), 'bitrate'.\n"
            " • Mở công cụ Windows: gõ 'mo regedit', 'mo taskmgr', 'mo temp'...\n"
            " • Kích hoạt Online AI: gõ 'setkey <GEMINI_API_KEY>' để kết nối Cloud AI."
        )

    return None


# ==============================================================================
# KHO TRI THỨC VĂN BẢN VÀ MÔ TẢ ĐẦY ĐỦ TÍNH NĂNG CMD BOX
# ==============================================================================
FEATURES_DATABASE: Dict[str, Dict[str, Any]] = {
    "don_rac": {
        "title": "Dọn dẹp rác & Giải phóng bộ nhớ ổ đĩa (Disk Cleanup PRO)",
        "menu_tag": "[Menu 1 -> 1] Dọn rác chuyên sâu PRO",
        "doc": "don dep rac giai phong bo nho o dia o c o d o cung day bao do xoa file tam temp cache thung rac recycle cleanmgr disk cleanup dung luong giam bo nho day o c day o cung c o cung bi day o c bao do o c het dung luong full disk",
        "diagnosis": "Ổ đĩa (đặc biệt là ổ C:) đang bị đầy bởi các tập tin rác tạm thời (Temp), cache Windows Update, log crash và file thừa.",
        "solution": [
            "Mở CMD BOX: Chọn [1] Bảo trì & Tối ưu -> [1] Dọn rác chuyên sâu PRO.",
            "Hệ thống sẽ tự động quét và dọn sạch các vùng nhớ đệm, temp, dump file và log thừa."
        ],
        "quick_action": {"name": "Thùng rác (Recycle Bin)", "command": "start shell:RecycleBinFolder"}
    },
    "pin_laptop": {
        "title": "Kiểm tra độ chai & Sức khỏe Pin Laptop (Battery Report)",
        "menu_tag": "[Menu 3 -> 6] Kiểm tra độ chai Pin Laptop",
        "doc": "pin battery sac laptop chai pin het pin tut pin hao pin suc khoe do chai hu hong bao ve pin kem chai pin bao cao battery report sụt pin pin yeu pin tut nhanh pin tụt",
        "diagnosis": "Pin laptop có thể đang bị chai, dung lượng thực tế giảm so với dung lượng thiết kế (Design Capacity), hoặc có ứng dụng ngầm gây hao pin.",
        "solution": [
            "Mở CMD BOX: Chọn [3] Công cụ tiện ích -> [6] Kiểm tra độ chai Pin Laptop.",
            "CMD BOX sẽ tự động phân tích và xuất báo cáo chi tiết (HTML Battery Report) tình trạng chu kỳ sạc và độ chai pin."
        ],
        "quick_action": None
    },
    "may_lag": {
        "title": "Tối ưu hóa tốc độ & Khắc phục máy giật lag / khởi động chậm",
        "menu_tag": "[Menu 1 -> 2, 3, 6] Tối ưu hóa hệ thống",
        "doc": "may tinh cham lag do giat treo nang may khoi dong lau boot cham cpu ram cao full disk ultimate performance toi uu giat hinh nang may choi game giat fps thap",
        "diagnosis": "Nhiều ứng dụng chạy ngầm khởi động cùng Windows, các dịch vụ thừa hoặc chưa kích hoạt chế độ Ultimate Performance.",
        "solution": [
            "Tắt ứng dụng khởi động ngầm: Chọn [1] -> [2] Tắt ứng dụng khởi động.",
            "Tắt dịch vụ thừa: Chọn [1] -> [3] Tối ưu dịch vụ Windows.",
            "Tối ưu tổng thể: Chọn [1] -> [6] Tối ưu hóa tổng thể hệ thống (Registry + Power Plan)."
        ],
        "quick_action": {"name": "Task Manager", "command": "start taskmgr"}
    },
    "sua_mang": {
        "title": "Sửa lỗi kết nối & Khôi phục mạng toàn diện (Network Repair PRO)",
        "menu_tag": "[Menu 2 -> 2] Sửa lỗi & Khôi phục mạng toàn diện",
        "doc": "mang internet wifi wlan lan net ping cao dns winsock adapter router mat mang rot mang chap chon khong vao duoc web sua loi network socket 10013 lag mang",
        "diagnosis": "Winsock Catalog lỗi, cache DNS cũ, Winsock bị chiếm port hoặc xung đột với WSL2/Docker/Hyper-V.",
        "solution": [
            "Tự động sửa lỗi toàn diện: Chọn [2] Mạng & Bảo mật -> [2] Sửa lỗi & Khôi phục mạng toàn diện (8 bước chuyên sâu).",
            "Xem thông tin IP / DNS / Adapter: Chọn [2] -> [1] Xem thông tin mạng chi tiết."
        ],
        "quick_action": {"name": "Cài đặt Kết nối Mạng", "command": "ncpa.cpl"}
    },
    "wifi_pass": {
        "title": "Xem lại mật khẩu Wi-Fi đã lưu trên máy tính",
        "menu_tag": "[Menu 2 -> 5] Xem danh sách mật khẩu Wi-Fi đã lưu",
        "doc": "pass wifi mat khau wifi xem pass lay pass tim pass quen pass chia se wifi password wlan tim mat khau",
        "diagnosis": "Cần tra cứu mật khẩu của các mạng Wi-Fi mà laptop/PC đã từng kết nối trước đây.",
        "solution": [
            "Mở CMD BOX: Chọn [2] Mạng & Bảo mật -> [5] Xem danh sách mật khẩu Wi-Fi đã lưu."
        ],
        "quick_action": None
    },
    "bao_mat": {
        "title": "Kích hoạt lá chắn bảo mật & Khóa cổng mạng nguy hiểm",
        "menu_tag": "[Menu 2 -> 3] Kích hoạt Lá chắn bảo mật toàn diện",
        "doc": "virus bao mat chan cong tuong lua firewall defender chan web ma doc trojan hack bi hack khoa cong 445 135 hosts an toan quet virus",
        "diagnosis": "Cần nâng cấp bảo mật, cập nhật Defender, bật DoH và chặn các cổng mạng nguy hiểm (445, 135, 139).",
        "solution": [
            "Kích hoạt bảo mật: Chọn [2] Mạng & Bảo mật -> [3] Kích hoạt Lá chắn bảo mật toàn diện.",
            "Kiểm tra tập tin Hosts: Chọn [2] -> [6] Quét & Bảo vệ tập tin Hosts."
        ],
        "quick_action": {"name": "Windows Security", "command": "start windowsdefender:"}
    },
    "auto_click": {
        "title": "Công cụ Auto Click chuột tự động",
        "menu_tag": "[Menu 3 -> 1] Auto Click chuột",
        "doc": "auto click chuot tu dong nhap chuot click lien tuc spam click treo game bam chuot click chuot",
        "diagnosis": "Cần tự động hóa thao tác nhấp chuột theo tọa độ và delay định sẵn, hỗ trợ phím ngắt khẩn cấp ESC/F6.",
        "solution": [
            "Mở CMD BOX: Chọn [3] Công cụ tiện ích -> [1] Auto Click chuột."
        ],
        "quick_action": None
    },
    "spam_text": {
        "title": "Công cụ Spam Text / Gửi tin nhắn tự động",
        "menu_tag": "[Menu 3 -> 2] Spam Text",
        "doc": "spam text spam tin nhan gui tin nhan nhan tin lien tuc spam mess spam zalo nhap chu lien tuc",
        "diagnosis": "Cần gửi tin nhắn hoặc văn bản lặp lại chuẩn Tiếng Việt Unicode với phím dừng khẩn cấp ESC/F6.",
        "solution": [
            "Mở CMD BOX: Chọn [3] -> [2] Spam Text."
        ],
        "quick_action": None
    },
    "auto_paste": {
        "title": "Công cụ Auto Paste dữ liệu nhiều dòng tự động",
        "menu_tag": "[Menu 3 -> 3] Auto Paste dữ liệu nhiều dòng",
        "doc": "auto paste dan tu dong nhap danh sach paste lien tuc dien form dan nhieu dong paste data",
        "diagnosis": "Cần dán tự động danh sách nhiều dòng dữ liệu vào form hoặc ứng dụng.",
        "solution": [
            "Mở CMD BOX: Chọn [3] -> [3] Auto Paste dữ liệu nhiều dòng."
        ],
        "quick_action": None
    },
    "cai_app": {
        "title": "Trình tải & Cài đặt phần mềm tự động",
        "menu_tag": "[Menu 3 -> 4] Tải & Cài đặt phần mềm tự động",
        "doc": "cai phan mem tai app cai app tai chrome tai zalo tai vscode tai discord cai dat download software tai git",
        "diagnosis": "Cần tải và cài đặt nhanh các phần mềm phổ biến mà không cần tìm link thủ công.",
        "solution": [
            "Mở CMD BOX: Chọn [3] -> [4] Tải & Cài đặt phần mềm tự động."
        ],
        "quick_action": None
    },
    "bloatware": {
        "title": "Gỡ bỏ ứng dụng rác mặc định (Bloatware Windows)",
        "menu_tag": "[Menu 3 -> 5] Gỡ bỏ ứng dụng rác mặc định",
        "doc": "go bloatware go app rac xoa app mac dinh go ung dung win xoa bot phan mem uninstall app",
        "diagnosis": "Các ứng dụng mặc định không dùng đến trên Windows đang chiếm dung lượng và chạy ngầm.",
        "solution": [
            "Mở CMD BOX: Chọn [3] -> [5] Gỡ bỏ ứng dụng rác mặc định."
        ],
        "quick_action": {"name": "Cài đặt Ứng dụng (Apps)", "command": "start ms-settings:appsfeatures"}
    },
    "nen_media": {
        "title": "Nén dung lượng Video / Ảnh (Tăng tốc GPU NVENC/Intel/AMD)",
        "menu_tag": "[Menu 4 -> 1] Nén dung lượng Video / Ảnh",
        "doc": "nen video giam size video clip nang qua clip anh photo giam dung luong video nang anh nang video qua lon bop dung luong rut gon media gpu nvenc khong gui duoc qua zalo nang qua giam mb",
        "diagnosis": "Tệp video/ảnh có dung lượng quá lớn cần tối ưu kích thước mà vẫn giữ độ nét và metadata.",
        "solution": [
            "Mở CMD BOX: Chọn [4] Xử lý Media -> [1] Nén dung lượng Video / Ảnh."
        ],
        "quick_action": None
    },
    "lam_net": {
        "title": "Làm nét & Khử nhiễu Video / Ảnh (Enhancement)",
        "menu_tag": "[Menu 4 -> 2] Làm nét Video / Ảnh",
        "doc": "lam net lam ro upscale video mo anh mo chat luong kem mo qua khu nhieu noise net hon tang do net video ro hon",
        "diagnosis": "Video hoặc Ảnh bị mờ, nhiễu hạt, cần bộ lọc tăng chi tiết và độ tương phản.",
        "solution": [
            "Mở CMD BOX: Chọn [4] -> [2] Làm nét Video / Ảnh (Từ Tự nhiên đến Siêu nét PRO)."
        ],
        "quick_action": None
    },
    "mp4_to_mp3": {
        "title": "Chuyển đổi Video sang Audio (Mp4 -> Mp3)",
        "menu_tag": "[Menu 4 -> 3] Mp4 -> Mp3",
        "doc": "mp4 sang mp3 tach nhac lay audio chuyen video thanh nhac rut mp3 lay tieng tach am thanh trich xuat nhac clip",
        "diagnosis": "Cần trích xuất dải âm thanh chất lượng cao 320kbps từ video.",
        "solution": [
            "Mở CMD BOX: Chọn [4] -> [3] Mp4 -> Mp3."
        ],
        "quick_action": None
    },
    "toc_do_video": {
        "title": "Thay đổi tốc độ phát Video (Speed Control)",
        "menu_tag": "[Menu 4 -> 4] Thay đổi tốc độ Video",
        "doc": "toc do video tua nhanh slow motion giam toc do tang toc do video quay cham speed video",
        "diagnosis": "Cần làm chậm (Slow-motion 0.5x) hoặc tăng tốc (2.0x) video giữ nguyên cao độ âm thanh.",
        "solution": [
            "Mở CMD BOX: Chọn [4] -> [4] Thay đổi tốc độ Video."
        ],
        "quick_action": None
    },
    "doi_dinh_dang": {
        "title": "Đổi đuôi định dạng Media hàng loạt",
        "menu_tag": "[Menu 4 -> 5] Đổi đuôi định dạng Media",
        "doc": "doi duoi convert chuyen duoi mkv sang mp4 png sang jpg webp sang png doi format chuyen dinh dang",
        "diagnosis": "Cần chuyển đổi định dạng (MP4, MKV, AVI, MOV / JPG, PNG, WEBP, BMP...).",
        "solution": [
            "Mở CMD BOX: Chọn [4] -> [5] Đổi đuôi định dạng Media."
        ],
        "quick_action": None
    },
    "an_file": {
        "title": "Ẩn file trong file (Steganography)",
        "menu_tag": "[Menu 4 -> 7] Ẩn file trong file",
        "doc": "an file giau file steganography giau tap tin an file vao anh an file vao video giau du lieu mat ma",
        "diagnosis": "Cần ngụy trang tệp dữ liệu mật vào bên trong ảnh hoặc video.",
        "solution": [
            "Mở CMD BOX: Chọn [4] -> [7] Ẩn file trong file."
        ],
        "quick_action": None
    },
    "win_update": {
        "title": "Sửa lỗi Windows Update",
        "menu_tag": "[Menu 1 -> 5] Sửa lỗi Windows Update",
        "doc": "loi update windows update khong update duoc cap nhat win update loi update fail fix update",
        "diagnosis": "Cache cập nhật Windows bị lỗi hoặc service BITS/WUAUSERV bị treo.",
        "solution": [
            "Mở CMD BOX: Chọn [1] -> [5] Sửa lỗi Windows Update."
        ],
        "quick_action": {"name": "Windows Update Settings", "command": "start ms-settings:windowsupdate"}
    },
    "taskbar_win11": {
        "title": "Tùy biến Taskbar & Giao diện Win 11",
        "menu_tag": "[Menu 1 -> 4] Chỉnh giao diện & Taskbar Win 11",
        "doc": "taskbar can le taskbar win 11 menu chuot phai giao dien win 11 chinh taskbar classic menu",
        "diagnosis": "Cần khôi phục menu chuột phải cổ điển, căn lề trái Taskbar hoặc ẩn icon thừa.",
        "solution": [
            "Mở CMD BOX: Chọn [1] -> [4] Chỉnh giao diện & Taskbar Win 11."
        ],
        "quick_action": None
    }
}


# ==============================================================================
# BỘ MÁY TF-IDF & BI-GRAM VECTOR REASONER
# ==============================================================================
class VectorSemanticReasoner:
    def __init__(self):
        self.db = FEATURES_DATABASE

    def search(self, query: str) -> Optional[Dict[str, Any]]:
        q_norm = remove_accents(query).strip()
        tokens = [w for w in re.findall(r'\w+', q_norm) if len(w) > 1 or w == 'c']
        bigrams = [' '.join(tokens[i:i+2]) for i in range(len(tokens)-1)]
        trigrams = [' '.join(tokens[i:i+3]) for i in range(len(tokens)-2)]

        best_match = None
        max_score = 0.0

        for key, item in self.db.items():
            doc = remove_accents(item["doc"])
            doc_words = set(doc.split())

            score = sum(1.2 for t in tokens if t in doc_words)
            score += sum(3.0 for bg in bigrams if bg in doc)
            score += sum(5.0 for tg in trigrams if tg in doc)

            if score > max_score:
                max_score = score
                best_match = item

        if max_score >= 1.5:
            return best_match
        return None


# ==============================================================================
# HỆ THỐNG SKILLS TÍCH HỢP
# ==============================================================================
class AssistantSkills:
    @staticmethod
    def quick_system_scan():
        print(f"\n{C.CYAN}┌────────────────────── 📊 CHẨN ĐOÁN HỆ THỐNG ──────────────────────┐{C.RESET}")
        
        # Ổ đĩa C:
        try:
            import ctypes
            free_bytes = ctypes.c_ulonglong(0)
            total_bytes = ctypes.c_ulonglong(0)
            ctypes.windll.kernel32.GetDiskFreeSpaceExW(
                ctypes.c_wchar_p("C:\\"), None, ctypes.byref(total_bytes), ctypes.byref(free_bytes)
            )
            free_gb = free_bytes.value / (1024**3)
            tot_gb = total_bytes.value / (1024**3)
            used_pct = ((tot_gb - free_gb) / tot_gb) * 100
            
            bar_len = 18
            filled = int(bar_len * (used_pct / 100))
            bar = "█" * filled + "░" * (bar_len - filled)
            
            color = C.RED if free_gb < 15 else (C.YELLOW if free_gb < 30 else C.GREEN)
            print(f"  {C.WHITE}Ổ đĩa C:{C.RESET} [{color}{bar}{C.RESET}] Trống: {color}{free_gb:.1f} GB{C.RESET}/{tot_gb:.1f} GB ({used_pct:.0f}%)")
        except Exception:
            print(f"  {C.WHITE}Ổ đĩa C:{C.RESET} Không thể đọc dung lượng")

        # Hostname & User
        user = os.getenv("USERNAME", "Unknown")
        pc = os.getenv("COMPUTERNAME", "Unknown")
        print(f"  {C.WHITE}Thiết bị:{C.RESET} {C.CYAN}{user}@{pc}{C.RESET}")

        # Windows Version
        try:
            ver = sys.getwindowsversion()
            build = ver.build
            win_name = "Windows 11" if build >= 22000 else "Windows 10"
            print(f"  {C.WHITE}Hệ điều hành:{C.RESET} {win_name} (Build {build})")
        except Exception:
            pass

        # Kiểm tra mạng nhanh
        print(f"  {C.WHITE}Kết nối Internet:{C.RESET} ", end="", flush=True)
        res = subprocess.run("ping -n 1 8.8.8.8 >nul 2>&1", shell=True)
        if res.returncode == 0:
            print(f"{C.GREEN}● Ổn định{C.RESET}")
        else:
            print(f"{C.RED}● Mất kết nối{C.RESET}")

        print(f"{C.CYAN}└───────────────────────────────────────────────────────────────────┘{C.RESET}")

    @staticmethod
    def ping_test():
        print(f"\n{C.CYAN}[*] Đang đo độ trễ mạng (Ping Test)...{C.RESET}")
        targets = [("Google DNS", "8.8.8.8"), ("Cloudflare", "1.1.1.1")]
        for name, ip in targets:
            p = subprocess.run(f"ping -n 2 {ip}", capture_output=True, text=True, shell=True)
            match = re.search(r"Average = (\d+)ms|Trung bình = (\d+)ms", p.stdout)
            if match:
                avg = match.group(1) or match.group(2)
                color = C.GREEN if int(avg) < 40 else (C.YELLOW if int(avg) < 90 else C.RED)
                print(f"  {C.WHITE}Ping tới {name:<12} ({ip}):{C.RESET} {color}{avg} ms{C.RESET}")
            else:
                print(f"  {C.WHITE}Ping tới {name:<12} ({ip}):{C.RESET} {C.RED}Thất bại / Timeout{C.RESET}")

    @staticmethod
    def generate_password(length: int = 16) -> str:
        chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-="
        pwd = "".join(random.SystemRandom().choice(chars) for _ in range(length))
        print(f"\n{C.GREEN}🔑 MẬT KHẨU BẢO MẬT CAO:{C.RESET}")
        print(f"   {C.YELLOW}{C.BOLD}{pwd}{C.RESET} ({length} ký tự, gồm số + chữ hoa/thường + ký tự đặc biệt)\n")
        return pwd

    @staticmethod
    def calculate_bitrate():
        print(f"\n{C.CYAN}🧮 TÍNH TOÁN DUNG LƯỢNG & BITRATE VIDEO{C.RESET}")
        try:
            dur_str = input("  Nhập thời lượng video (phút): ").strip()
            target_mb_str = input("  Nhập dung lượng mong muốn (MB): ").strip()
            dur = float(dur_str)
            target_mb = float(target_mb_str)
            
            total_sec = dur * 60
            total_kbps = (target_mb * 8192) / total_sec
            audio_kbps = 128
            video_kbps = max(100, int(total_kbps - audio_kbps))
            
            print(f"\n  {C.GREEN}→ Gợi ý thông số thiết lập:{C.RESET}")
            print(f"    - Bitrate Video : {C.CYAN}{video_kbps} kbps{C.RESET} (-b:v {video_kbps}k)")
            print(f"    - Bitrate Audio : {C.CYAN}{audio_kbps} kbps{C.RESET} (-b:a {audio_kbps}k)")
            print(f"    - Dung lượng ước tính: ~{target_mb:.1f} MB\n")
        except Exception:
            print(f"  {C.RED}Thông số không hợp lệ!{C.RESET}")

    @staticmethod
    def lookup_error(code: str):
        ERRORS = {
            "0x80070005": "Lỗi Access Denied (Từ chối truy cập do thiếu quyền Admin). Khắc phục: Chạy CMD BOX bằng Run as Administrator.",
            "0x80240020": "Lỗi Windows Update do cache tải về bị hỏng. Khắc phục: Dùng [1] -> [5] Sửa lỗi Windows Update.",
            "0x80070422": "Lỗi dịch vụ Windows bị Disabled (Tắt). Khắc phục: Dùng [1] -> [3] Tối ưu/Bật lại dịch vụ.",
            "10013": "Lỗi WSAEACCES (Socket 10013: Dải cổng mạng bị chiếm dụng bởi Hyper-V/WSL2/Docker). Khắc phục: Dùng [2] -> [2] Sửa lỗi mạng PRO.",
            "0x800f081f": "Lỗi thiếu gói .NET Framework 3.5 / DISM Source File. Khắc phục: Chạy DISM restore health.",
            "0x80070002": "Lỗi The system cannot find the file specified (Không tìm thấy tệp tin được chỉ định)."
        }
        code_clean = code.lower().replace("0x", "").strip()
        matched = False
        for k, v in ERRORS.items():
            if code_clean in k.lower():
                print(f"\n{C.YELLOW}🔍 TRA CỨU MÃ LỖI {k}:{C.RESET}")
                print(f"  {v}\n")
                matched = True
                break
        if not matched:
            print(f"\n{C.GRAY}Không tìm thấy mã lỗi '{code}' trong cơ sở dữ liệu nội bộ.{C.RESET}\n")

    @staticmethod
    def quick_launcher(cmd: str) -> bool:
        SHORTCUTS = {
            "regedit": ("Registry Editor", "start regedit"),
            "gpedit": ("Group Policy Editor", "start gpedit.msc"),
            "taskmgr": ("Task Manager", "start taskmgr"),
            "cmd": ("Command Prompt Admin", "start cmd"),
            "powershell": ("PowerShell", "start powershell"),
            "temp": ("Thư mục Temp", "start %temp%"),
            "appdata": ("Thư mục AppData", "start %appdata%"),
            "godmode": ("God Mode Control Panel", "start shell:::{ED7BA470-8E54-465E-825C-99712043E01C}"),
            "devmgmt": ("Device Manager", "start devmgmt.msc"),
            "services": ("Windows Services", "start services.msc")
        }
        for key, (name, run_cmd) in SHORTCUTS.items():
            if key in cmd:
                try:
                    subprocess.Popen(run_cmd, shell=True)
                    print(f"  {C.GREEN}→ Đã mở {name}!{C.RESET}")
                    return True
                except Exception as e:
                    print(f"  {C.RED}Lỗi khi mở: {e}{C.RESET}")
                    return True
        return False


# ==============================================================================
# UI CYBER BANNER
# ==============================================================================
def print_cyber_banner(has_online_key: bool = False):
    os.system("cls")
    ai_status = f"{C.GREEN}● ONLINE AI (Gemini){C.RESET}" if has_online_key else f"{C.YELLOW}● HYBRID NLP (Offline){C.RESET}"
    banner = f"""{C.CYAN}
   ╔═══════════════════════════════════════════════════════════╗
   ║   ⚡ {C.WHITE}{C.BOLD}CMD BOX CYBER ASSISTANT{C.RESET}{C.CYAN}  │  {ai_status}{C.CYAN}            ║
   ╚═══════════════════════════════════════════════════════════╝{C.RESET}
 {C.GRAY}💡 Lệnh nhanh: {C.WHITE}'scan'{C.GRAY}, {C.WHITE}'ping'{C.GRAY}, {C.WHITE}'pass'{C.GRAY}, {C.WHITE}'setkey <KEY>'{C.GRAY} │ {C.WHITE}'0'{C.GRAY} để thoát.{C.RESET}
"""
    print(banner)


# ==============================================================================
# CHƯƠNG TRÌNH CHÍNH
# ==============================================================================
def main():
    reasoner = VectorSemanticReasoner()
    api_key = load_api_key()
    print_cyber_banner(bool(api_key))

    while True:
        try:
            prompt = f"{C.CYAN}╭─({C.GREEN}AI{C.CYAN})─[{C.WHITE}Query{C.CYAN}]\n╰─➤ {C.RESET}"
            user_input = input(prompt).strip()

            if not user_input:
                continue

            low_input = user_input.lower().strip()

            # 1. Lệnh thoát
            if low_input in ["0", "exit", "quit", "thoat"]:
                print(f"\n{C.CYAN}[Trợ lý]: Tạm biệt! Đang đóng cửa sổ...{C.RESET}")
                time.sleep(0.5)
                break

            # 2. Cài đặt / cập nhật API Key
            if low_input.startswith("setkey ") or low_input.startswith("api "):
                new_key = user_input.split(maxsplit=1)[1].strip()
                if save_api_key(new_key):
                    api_key = new_key
                    print_cyber_banner(True)
                    print(f"\n {C.GREEN}✅ Đã lưu Google Gemini API Key thành công! Trợ lý đã kích hoạt Online AI.{C.RESET}\n")
                else:
                    print(f"\n {C.RED}❌ Không thể lưu API Key.{C.RESET}\n")
                continue

            # Làm sạch màn hình và hiển thị câu hỏi hiện tại
            print_cyber_banner(bool(api_key))
            print(f" {C.WHITE}💬 [BẠN HỎI]:{C.RESET} {C.YELLOW}{C.BOLD}{user_input}{C.RESET}")
            print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}")

            # 3. Tra cứu Skills đặc biệt
            if low_input in ["scan", "kiem tra", "ktra", "thong tin may", "status"]:
                AssistantSkills.quick_system_scan()
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue
            elif low_input in ["ping", "kiem tra ping", "speed", "test mang"]:
                AssistantSkills.ping_test()
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue
            elif low_input in ["pass", "tao pass", "tao mat khau", "password"]:
                AssistantSkills.generate_password()
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue
            elif low_input in ["bitrate", "tinh bitrate", "tinh dung luong"]:
                AssistantSkills.calculate_bitrate()
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue
            elif low_input.startswith("loi ") or low_input.startswith("0x") or low_input.startswith("error"):
                code = low_input.replace("loi", "").replace("error", "").strip()
                AssistantSkills.lookup_error(code)
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue
            elif low_input.startswith("mo ") or low_input.startswith("open "):
                target = low_input.replace("mo ", "").replace("open ", "").strip()
                if AssistantSkills.quick_launcher(target):
                    print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                    continue

            # 4. Xử lý Chào hỏi / Xã giao (Small Talk)
            small_talk_reply = handle_small_talk(remove_accents(user_input))
            if small_talk_reply:
                print(f" {C.CYAN}🤖 [TRỢ LÝ AI]:{C.RESET}\n {C.WHITE}{small_talk_reply}{C.RESET}")
                print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")
                continue

            # 5. Nếu có Gemini API Key -> Gọi Cloud AI để trả lời thông minh nhất
            ai_responded = False
            if api_key:
                print(f" {C.DIM}[Đang xử lý qua Gemini AI...]{C.RESET}", end="\r", flush=True)
                ai_text = call_gemini_api(api_key, user_input)
                if ai_text:
                    print(" " * 40, end="\r") # Xóa dòng loading
                    print(f" {C.CYAN}🤖 [GEMINI AI]:{C.RESET}\n{C.WHITE}{ai_text}{C.RESET}")
                    ai_responded = True

            # 6. Nếu không có Key hoặc AI lỗi mạng -> Dùng Vector N-gram Reasoning Offline
            if not ai_responded:
                intent = reasoner.search(user_input)
                if intent:
                    print(f" {C.CYAN}🎯 VẤN ĐỀ:{C.RESET} {C.BOLD}{intent['title']}{C.RESET}")
                    print(f" {C.GRAY}🔍 Chẩn đoán:{C.RESET} {intent['diagnosis']}\n")
                    print(f" {C.GREEN}🛠️ HƯỚNG DẪN TRONG CMD BOX:{C.RESET}")
                    print(f"   👉 {C.CYAN}{C.BOLD}{intent['menu_tag']}{C.RESET}")
                    for sol in intent["solution"]:
                        print(f"      {C.WHITE}• {sol}{C.RESET}")

                    # Quick action Windows
                    if intent.get("quick_action"):
                        qa = intent["quick_action"]
                        print(f"\n {C.YELLOW}⚡ Lối tắt Windows:{C.RESET} Mở {qa['name']}? (y/n): ", end="", flush=True)
                        choice = input().strip().lower()
                        if choice in ['y', 'yes', '1']:
                            try:
                                subprocess.Popen(qa["command"], shell=True)
                                print(f"   {C.GREEN}→ Đã mở {qa['name']}!{C.RESET}")
                            except Exception as e:
                                print(f"   {C.RED}→ Không thể mở: {e}{C.RESET}")

                else:
                    print(f" {C.RED}❌ Chưa nhận diện được ý định rõ ràng.{C.RESET}")
                    print(f" {C.GRAY}Gợi ý: Thử mô tả ngắn gọn như: 'hết pin', 'đầy ổ cứng c', 'máy lag', 'sửa mạng', 'nén video', 'làm nét' hoặc gõ 'help'.{C.RESET}")

            print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}\n")

        except (KeyboardInterrupt, EOFError):
            print("\n\nĐang thoát trợ lý ảo...\n")
            break


if __name__ == "__main__":
    main()
