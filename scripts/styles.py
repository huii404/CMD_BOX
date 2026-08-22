# -*- coding: utf-8 -*-
"""
CMD BOX - Cyber UI Styles & Animations
"""

import sys
import os
import time
import random

# Đảm bảo UTF-8 trên Windows console
if sys.platform == "win32":
    try:
        os.system("chcp 65001 >nul")
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        sys.stdin.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass

# ANSI Color & Style Codes
class C:
    RESET       = "\033[0m"
    BOLD        = "\033[1m"
    DIM         = "\033[2m"
    ITALIC      = "\033[3m"
    UNDERLINE   = "\033[4m"
    
    # Colors
    CYAN        = "\033[96m"
    BLUE        = "\033[94m"
    GREEN       = "\033[92m"
    YELLOW      = "\033[93m"
    RED         = "\033[91m"
    MAGENTA     = "\033[95m"
    WHITE       = "\033[97m"
    GRAY        = "\033[90m"
    
    # Backgrounds
    BG_CYAN     = "\033[46m"
    BG_BLUE     = "\033[44m"
    BG_MAGENTA  = "\033[45m"


def print_cyber_banner(has_online_key: bool = False):
    os.system("cls")
    ai_status = f"{C.GREEN}● ONLINE AI (Gemini){C.RESET}" if has_online_key else f"{C.YELLOW}● HYBRID NLP (Offline){C.RESET}"
    banner = f"""{C.CYAN}
   ╔═══════════════════════════════════════════════════════════════╗
   ║   ⚡ {C.WHITE}{C.BOLD}CMD BOX CYBER ASSISTANT{C.RESET}{C.CYAN}  │  {ai_status}{C.CYAN}            ║
   ╚═══════════════════════════════════════════════════════════════╝{C.RESET}
 {C.GRAY}💡 Lệnh nhanh: {C.WHITE}'scan'{C.GRAY}, {C.WHITE}'ping'{C.GRAY}, {C.WHITE}'matrix'{C.GRAY}, {C.WHITE}'boi'{C.GRAY}, {C.WHITE}'pass'{C.GRAY}, {C.WHITE}'setkey <KEY>'{C.GRAY} │ {C.WHITE}'0'{C.GRAY} để thoát.{C.RESET}
"""
    print(banner)


def typewriter_effect(text: str, delay: float = 0.015, color: str = C.WHITE):
    """Hiệu ứng chữ gõ máy tính sinh động."""
    for char in text:
        sys.stdout.write(f"{color}{char}{C.RESET}")
        sys.stdout.flush()
        time.sleep(delay)
    print()


def cyber_divider():
    print(f"{C.GRAY}─────────────────────────────────────────────────────────────{C.RESET}")
