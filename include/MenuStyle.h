#pragma once
#include <iostream>
#include <string>

namespace MenuStyle {
inline constexpr const char* RESET = "\x1b[0m";
inline constexpr const char* BOLD = "\x1b[1m";
inline constexpr const char* TEXT = "\x1b[38;2;232;235;247m";
inline constexpr const char* MUTED = "\x1b[38;2;151;157;184m";
struct Accent { const char* text; };
inline constexpr Accent ORCHID{"\x1b[38;2;195;165;255m"};
inline constexpr Accent MINT{"\x1b[38;2;114;232;194m"};
inline constexpr Accent SKY{"\x1b[38;2;126;203;255m"};
inline constexpr Accent AMBER{"\x1b[38;2;255;208;133m"};
inline constexpr Accent ROSE{"\x1b[38;2;255;151;177m"};
inline constexpr Accent FRAME[] = {ORCHID, SKY, MINT, AMBER, ROSE};
inline constexpr size_t WIDTH = 68;
inline size_t row = 0;

inline size_t columns(const std::string& text) {
    size_t width=0;
    for(unsigned char c:text)if((c&0xc0)!=0x80)++width;
    return width;
}
inline void edge(size_t count,size_t offset=0) {
    for(size_t i=0;i<count;++i)std::cout<<FRAME[((i+offset)*5/(WIDTH+2))%5].text<<"─";
}
inline void left() {
    std::cout<<FRAME[(row/2)%5].text<<"  │"<<RESET;
}
inline void right(size_t used) {
    if(used<WIDTH)std::cout<<std::string(WIDTH-used,' ');
    std::cout<<FRAME[4-(row/2)%5].text<<"│"<<RESET<<'\n';++row;
}
inline void blank() { left();right(0); }
inline void header(const char* title,Accent accent=ORCHID) {
    row=0;
    std::cout<<'\n'<<ORCHID.text<<"  ╭";
    edge(2);
    std::cout<<' '<<BOLD<<accent.text<<title<<RESET<<' ';
    auto used=columns(title)+4;
    if(used<WIDTH)edge(WIDTH-used,used);
    std::cout<<ROSE.text<<"╮"<<RESET<<'\n';blank();
}
inline void item(int number,const char* label,Accent accent=ORCHID,const char* note="") {
    auto key="  ["+std::to_string(number)+"]  ";
    left();
    std::cout<<TEXT<<key<<accent.text<<label<<MUTED<<note<<RESET;
    right(columns(key)+columns(label)+columns(note));
}
inline void info(const char* label,const std::string& value,Accent accent=ORCHID) {
    auto prefix=std::string("  ")+label+" : ";
    left();std::cout<<MUTED<<prefix<<accent.text<<value<<RESET;
    right(columns(prefix)+columns(value));
}
inline void section(const char* title) {
    blank();left();std::cout<<MUTED<<"  "<<title<<RESET;right(columns(title)+2);
}
inline void footer(const char* back="Quay lại menu chính",Accent accent=ORCHID) {
    blank();item(0,back,ROSE);blank();
    std::cout<<ORCHID.text<<"  ╰";edge(WIDTH);
    std::cout<<ROSE.text<<"╯"<<RESET<<"\n\n"<<BOLD<<accent.text<<"  ❯ "<<TEXT<<"Chọn mục: "<<RESET;
}
}
