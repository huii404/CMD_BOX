# -*- coding: utf-8 -*-
"""
CMD BOX - Cyber Virtual Assistant (Trợ lý ảo AI Thông minh - Hybrid AI)
File chính khởi chạy và điều phối toàn bộ hệ thống Assistant.
"""

import sys
import os
import time
import subprocess

# Cấu hình UTF-8 & Tiêu đề console Windows
if sys.platform == "win32":
    try:
        os.system("chcp 65001 >nul")
        os.system("title CMD BOX - Trợ lý ảo AI")
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stdin.reconfigure(encoding='utf-8')
    except Exception:
        pass

# Import các module đã tách biệt
from styles import C, print_cyber_banner, cyber_divider, typewriter_effect
from ai_engine import (
    remove_accents,
    load_api_key,
    save_api_key,
    call_gemini_api,
    handle_small_talk,
    VectorSemanticReasoner
)
from skills import AssistantSkills


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
            norm_input = remove_accents(low_input)

            # 1. Lệnh thoát
            if low_input in ["0", "exit", "quit", "thoat"]:
                print(f"\n{C.CYAN}[Trợ lý]: Tạm biệt! Đang đóng cửa sổ...{C.RESET}")
                time.sleep(0.5)
                break

            # 2. Cài đặt / Cập nhật API Key
            if low_input.startswith("setkey ") or low_input.startswith("api "):
                new_key = user_input.split(maxsplit=1)[1].strip()
                if save_api_key(new_key):
                    api_key = new_key
                    print_cyber_banner(True)
                    print(f"\n {C.GREEN}✅ Đã lưu Google Gemini API Key thành công! Trợ lý đã kích hoạt Online AI.{C.RESET}\n")
                else:
                    print(f"\n {C.RED}❌ Không thể lưu API Key.{C.RESET}\n")
                continue

            # Làm sạch màn hình và hiển thị truy vấn hiện tại
            print_cyber_banner(bool(api_key))
            print(f" {C.WHITE}💬 [BẠN HỎI]:{C.RESET} {C.YELLOW}{C.BOLD}{user_input}{C.RESET}")
            cyber_divider()

            # 3. Tra cứu Skills hệ thống
            if any(low_input == k for k in ["scan", "kiem tra", "ktra", "thong tin may", "status"]):
                AssistantSkills.quick_system_scan()
                cyber_divider()
                print()
                continue
            elif any(low_input == k for k in ["ping", "kiem tra ping", "speed", "test mang"]):
                AssistantSkills.ping_test()
                cyber_divider()
                print()
                continue
            elif any(low_input == k for k in ["pass", "tao pass", "tao mat khau", "password"]):
                AssistantSkills.generate_password()
                cyber_divider()
                print()
                continue
            elif any(low_input == k for k in ["bitrate", "tinh bitrate", "tinh dung luong"]):
                AssistantSkills.calculate_bitrate()
                cyber_divider()
                print()
                continue
            elif low_input.startswith("loi ") or low_input.startswith("0x") or low_input.startswith("error"):
                code = low_input.replace("loi", "").replace("error", "").strip()
                AssistantSkills.lookup_error(code)
                cyber_divider()
                print()
                continue
            elif low_input.startswith("mo ") or low_input.startswith("open "):
                target = low_input.replace("mo ", "").replace("open ", "").strip()
                if AssistantSkills.quick_launcher(target):
                    cyber_divider()
                    print()
                    continue

            # 4. Tra cứu Skills & Hiệu ứng vui nhộn (Easter Eggs)
            if any(norm_input == k for k in ["matrix", "hacker", "mua ma tran", "hacker man"]):
                AssistantSkills.matrix_rain()
                cyber_divider()
                print()
                continue
            elif any(k in norm_input for k in ["boi", "que", "boi toan", "tarot", "xem boi", "que hom nay"]):
                AssistantSkills.tell_fortune()
                cyber_divider()
                print()
                continue
            elif any(k in norm_input for k in ["roll", "xuc xac", "do xuc xac", "dice"]):
                AssistantSkills.roll_dice()
                cyber_divider()
                print()
                continue
            elif any(k in norm_input for k in ["coin", "dong xu", "tung dong xu", "sap ngua"]):
                AssistantSkills.flip_coin()
                cyber_divider()
                print()
                continue
            elif any(k in norm_input for k in ["roast", "ca khia", "che may", "chui may"]):
                AssistantSkills.roast_computer()
                cyber_divider()
                print()
                continue
            elif any(k in norm_input for k in ["khen toi", "khen di", "ninh", "khen toi di"]):
                AssistantSkills.flatter_master()
                cyber_divider()
                print()
                continue

            # 5. Xử lý Chào hỏi / Xã giao Offline (Small Talk)
            small_talk_reply = handle_small_talk(norm_input)
            if small_talk_reply:
                print(f" {C.CYAN}🤖 [TRỢ LÝ AI]:{C.RESET}\n {C.WHITE}{small_talk_reply}{C.RESET}")
                cyber_divider()
                print()
                continue

            # 6. Nếu có Gemini API Key -> Gọi Cloud AI để trả lời thông minh nhất
            ai_responded = False
            if api_key:
                print(f" {C.DIM}[Đang xử lý qua Gemini AI...]{C.RESET}", end="\r", flush=True)
                ai_text = call_gemini_api(api_key, user_input)
                if ai_text:
                    print(" " * 40, end="\r")  # Xóa dòng loading
                    print(f" {C.CYAN}🤖 [GEMINI AI]:{C.RESET}\n{C.WHITE}{ai_text}{C.RESET}")
                    ai_responded = True

            # 7. Nếu không có Key hoặc AI lỗi mạng -> Dùng Vector N-gram Reasoning Offline
            if not ai_responded:
                intent = reasoner.search(user_input)
                if intent:
                    print(f" {C.CYAN}🎯 VẤN ĐỀ:{C.RESET} {C.BOLD}{intent['title']}{C.RESET}")
                    print(f" {C.GRAY}🔍 Chẩn đoán:{C.RESET} {intent['diagnosis']}\n")
                    print(f" {C.GREEN}🛠️ HƯỚNG DẪN TRONG CMD BOX:{C.RESET}")
                    print(f"   👉 Menu: {C.CYAN}{C.BOLD}{intent['menu_tag']}{C.RESET}")
                    steps = intent.get("steps") or intent.get("solution", [])
                    for step in steps:
                        print(f"      {C.WHITE}• {step}{C.RESET}")

                    # Quick action Windows
                    if intent.get("quick_action"):
                        qa = intent["quick_action"]
                        print(f"\n {C.YELLOW}⚡ Lối tắt nhanh:{C.RESET} Mở {qa['name']}? (y/n): ", end="", flush=True)
                        choice = input().strip().lower()
                        if choice in ['y', 'yes', '1']:
                            try:
                                subprocess.Popen(qa["command"], shell=True)
                                print(f"   {C.GREEN}→ Đã mở {qa['name']}!{C.RESET}")
                            except Exception as e:
                                print(f"   {C.RED}→ Không thể mở: {e}{C.RESET}")

                else:
                    print(f" {C.RED}❌ Chưa nhận diện được ý định rõ ràng.{C.RESET}")
                    print(f" {C.GRAY}Gợi ý: Thử mô tả ngắn gọn như: 'hết pin', 'đầy ổ cứng c', 'máy lag', 'sửa mạng', 'nén video', 'matrix', 'boi' hoặc gõ 'help'.{C.RESET}")

            cyber_divider()
            print()

        except (KeyboardInterrupt, EOFError):
            print("\n\nĐang thoát trợ lý ảo...\n")
            break


if __name__ == "__main__":
    main()
