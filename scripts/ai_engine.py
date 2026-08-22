# -*- coding: utf-8 -*-
"""
CMD BOX - AI Engine & Hybrid NLP Reasoner
Xử lý kết nối Cloud AI (Gemini 3.6 Flash) và Bộ máy suy luận từ khóa Vector N-Gram Offline.
"""

import os
import re
import json
import random
import time
import urllib.request
import unicodedata
from typing import Optional, Dict, Any, List
from styles import C
from dataset import FEATURES_DATABASE, GREETINGS_RESPONSES


CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json")


def remove_accents(input_str: str) -> str:
    """Loại bỏ dấu tiếng Việt chuẩn NFKD để matching chính xác."""
    nfkd = unicodedata.normalize('NFKD', input_str)
    return u"".join([c for c in nfkd if not unicodedata.combining(c)]).replace('đ', 'd').replace('Đ', 'D').lower()


def load_api_key() -> Optional[str]:
    """Tải API key từ biến môi trường hoặc file config.json."""
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
    """Lưu API Key vào config.json."""
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
    """Gọi trực tiếp Google Gemini API với cơ chế dự phòng nhiều model."""
    models_to_try = ["gemini-3.6-flash", "gemini-3.7-flash", "gemini-flash-latest"]
    
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
            "temperature": 0.7,
            "maxOutputTokens": 2048
        }
    }

    for model_name in models_to_try:
        url = f"https://generativelanguage.googleapis.com/v1beta/models/{model_name}:generateContent?key={api_key}"
        try:
            req = urllib.request.Request(
                url,
                data=json.dumps(payload).encode("utf-8"),
                headers={"Content-Type": "application/json"}
            )
            with urllib.request.urlopen(req, timeout=10) as response:
                result = json.loads(response.read().decode("utf-8"))
                return result["candidates"][0]["content"]["parts"][0]["text"].strip()
        except Exception:
            continue
    return None


class VectorSemanticReasoner:
    """Bộ máy phân tích ngữ nghĩa và tìm kiếm tính năng thông minh Offline."""
    def __init__(self):
        self.db = FEATURES_DATABASE

    def search(self, query: str) -> Optional[Dict[str, Any]]:
        q_norm = remove_accents(query).strip()
        tokens = [w for w in re.findall(r'\w+', q_norm) if len(w) > 1 or w == 'c']
        bigrams = [' '.join(tokens[i:i+2]) for i in range(len(tokens)-1)]

        best_match = None
        max_score = 0.0

        for key, item in self.db.items():
            keywords = [remove_accents(k) for k in item["keywords"]]
            score = 0.0

            # 1. Khớp cụm từ khóa hoàn chỉnh (+15 điểm)
            for kw in keywords:
                if kw in q_norm:
                    score += 15.0 + len(kw.split()) * 3.0

            # 2. Khớp Bi-gram (+4 điểm)
            for bg in bigrams:
                for kw in keywords:
                    if bg in kw:
                        score += 4.0

            # 3. Khớp Tokens từ đơn (+1.5 điểm)
            kw_tokens = set(" ".join(keywords).split())
            for t in tokens:
                if t in kw_tokens:
                    score += 1.5

            if score > max_score:
                max_score = score
                best_match = item

        if max_score >= 3.0:
            return best_match
        return None


def handle_small_talk(query_norm: str) -> Optional[str]:
    """Xử lý giao tiếp xã giao và thông tin cơ bản khi không dùng Cloud AI."""
    q = query_norm.strip()
    
    # 1. Hỏi tên người dùng / Thông tin máy
    if any(k in q for k in ["toi ten gi", "ten toi la gi", "toi la ai", "biet toi ten gi", "ten nguoi dung", "user name", "username", "ten may", "ten may tinh", "may tinh ten gi", "thiet bi ten gi"]):
        user = os.getenv("USERNAME", "User")
        pc = os.getenv("COMPUTERNAME", "PC")
        return f"Bạn đang đăng nhập với tài khoản {C.YELLOW}{C.BOLD}{user}{C.RESET} trên máy tính {C.CYAN}{pc}{C.RESET}!"

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

    # 6. Cảm ơn / Tạm biệt
    if any(q == k or q.startswith(k + " ") for k in ["cam on", "thank", "thanks", "cam on ban", "ok cam on", "tks"]):
        return "Không có gì nè! Nếu cần hỗ trợ thêm bất cứ điều gì về máy tính, cứ nhắn cho tôi nhé. Chúc bạn một ngày tốt lành! 🚀"

    # 7. Trợ giúp / Hướng dẫn
    if any(k in q for k in ["help", "huong dan", "tro giup", "lam duoc gi", "chuc nang", "co gi"]):
        return (
            "💡 Tôi có thể giúp bạn:\n"
            " • Chẩn đoán & hướng dẫn: 'đầy ổ c', 'máy lag', 'sửa mạng', 'nén video', 'làm nét'...\n"
            " • Lệnh vui nhộn: 'matrix' (hiệu ứng hacker), 'boi' (quẻ bói IT), 'roll' (đổ xúc xắc), 'roast' (chém gió máy).\n"
            " • Lệnh nhanh hệ thống: 'scan' (kiểm tra máy), 'ping' (đo mạng), 'pass' (tạo mật khẩu).\n"
            " • Mở công cụ Windows: 'mo regedit', 'mo taskmgr', 'mo temp'..."
        )

    return None
