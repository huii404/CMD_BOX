# -*- coding: utf-8 -*-
"""
CMD BOX - Assistant Skills & Fun Effects
Tập hợp các kỹ năng tiện ích hệ thống và hiệu ứng vui nhộn (Matrix, bói toán, xúc xắc, roast).
"""

import sys
import os
import re
import time
import random
import subprocess
from styles import C, typewriter_effect
from dataset import IT_FORTUNES, ROAST_QUOTES, FLATTER_QUOTES


class AssistantSkills:
    # ==========================================================================
    # CÁC SKILLS TIỆN ÍCH HỆ THỐNG
    # ==========================================================================
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

    # ==========================================================================
    # CÁC SKILLS VUI NHỘN & HIỆU ỨNG ĐẶC BIỆT
    # ==========================================================================
    @staticmethod
    def matrix_rain(lines: int = 35):
        """Hiệu ứng rơi ma trận số nhị phân kiểu Hacker Cyberpunk."""
        print(f"\n{C.GREEN}[*] ĐANG KÍCH HOẠT MA TRẬN CYBERPUNK... (Nhấn Ctrl+C để dừng){C.RESET}\n")
        chars = "0101010101ABCDEF9876543210#$&*@%~<>[]{}"
        width = 65
        try:
            for _ in range(lines):
                row = "".join(random.choice(chars) if random.random() > 0.6 else " " for _ in range(width))
                # Ngẫu nhiên tạo chữ màu sáng hơn
                colored_row = "".join(f"{C.WHITE}{c}" if random.random() > 0.85 else f"{C.GREEN}{c}" for c in row)
                print(f" {colored_row}{C.RESET}")
                time.sleep(0.04)
        except KeyboardInterrupt:
            pass
        print(f"\n{C.CYAN}✨ Ma trận đã hoàn tất! Trở về thực tại.{C.RESET}\n")

    @staticmethod
    def tell_fortune():
        """Bói toán IT / Coder vui nhộn."""
        fortune = random.choice(IT_FORTUNES)
        print(f"\n{C.MAGENTA}🔮 ═══ QUẺ BÓI CÔNG NGHỆ HÔM NAY ═══ 🔮{C.RESET}")
        time.sleep(0.3)
        typewriter_effect(f"   {fortune}", delay=0.02, color=C.YELLOW)
        print(f"{C.MAGENTA}═══════════════════════════════════════{C.RESET}\n")

    @staticmethod
    def roll_dice():
        """Đổ xúc xắc ngẫu nhiên kèm hình vẽ xúc xắc."""
        val = random.randint(1, 6)
        dice_art = {
            1: "┌───────┐\n│       │\n│   ●   │\n│       │\n└───────┘",
            2: "┌───────┐\n│ ●     │\n│       │\n│     ● │\n└───────┘",
            3: "┌───────┐\n│ ●     │\n│   ●   │\n│     ● │\n└───────┘",
            4: "┌───────┐\n│ ●   ● │\n│       │\n│ ●   ● │\n└───────┘",
            5: "┌───────┐\n│ ●   ● │\n│   ●   │\n│ ●   ● │\n└───────┘",
            6: "┌───────┐\n│ ●   ● │\n│ ●   ● │\n│ ●   ● │\n└───────┘"
        }
        print(f"\n{C.CYAN}🎲 ĐANG ĐỔ XÚC XẮC...{C.RESET}")
        time.sleep(0.4)
        print(f"{C.YELLOW}{dice_art[val]}{C.RESET}")
        print(f" 👉 Kết quả: {C.GREEN}{C.BOLD}{val} điểm{C.RESET}!\n")

    @staticmethod
    def flip_coin():
        """Tung đồng xu sấp hay ngửa."""
        outcome = random.choice(["SẤP (Mặt số)", "NGỬA (Mặt hình)"])
        print(f"\n{C.CYAN}🪙 ĐANG TUNG ĐỒNG XU...{C.RESET}")
        time.sleep(0.4)
        print(f" 👉 Kết quả: {C.YELLOW}{C.BOLD}{outcome}{C.RESET}!\n")

    @staticmethod
    def roast_computer():
        """Chém gió / Cà khịa máy tính vui nhộn."""
        quote = random.choice(ROAST_QUOTES)
        print(f"\n{C.RED}🌶️ [CHÉM GIÓ MÁY TÍNH]:{C.RESET}")
        typewriter_effect(f"   {quote}", delay=0.02, color=C.YELLOW)
        print()

    @staticmethod
    def flatter_master():
        """Nịnh chủ nhân / Khen ngợi tạo động lực."""
        quote = random.choice(FLATTER_QUOTES)
        print(f"\n{C.MAGENTA}💖 [GỬI BẠN MỘT LỜI KHEN]:{C.RESET}")
        typewriter_effect(f"   {quote}", delay=0.02, color=C.GREEN)
        print()
