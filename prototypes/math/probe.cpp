#include "ink_math.h"
#include "md4c.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <malloc.h>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <vector>
struct Math { std::string source; bool display; };
struct Context { std::vector<Math> formulas; bool inside=false; };
static int block(MD_BLOCKTYPE,void*,void*) { return 0; }
static int enter(MD_SPANTYPE type,void*,void *user) {
    auto& c=*static_cast<Context*>(user);
    if(type==MD_SPAN_LATEXMATH || type==MD_SPAN_LATEXMATH_DISPLAY) {
        c.inside=true; c.formulas.push_back({{},type==MD_SPAN_LATEXMATH_DISPLAY});
    }
    return 0;
}
static int leave(MD_SPANTYPE type,void*,void *user) {
    if(type==MD_SPAN_LATEXMATH || type==MD_SPAN_LATEXMATH_DISPLAY)
        static_cast<Context*>(user)->inside=false;
    return 0;
}
static int text(MD_TEXTTYPE type,const MD_CHAR *s,MD_SIZE n,void *user) {
    auto& c=*static_cast<Context*>(user);
    if(c.inside && type==MD_TEXT_LATEXMATH) c.formulas.back().source.append(s,n);
    return 0;
}
static uint8_t bitmap[INK_MATH_BYTES];
static void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
static void tests() {
    ink_math_result r;
    std::vector<std::string> rejected={"\\inkpyUnknownCommand{x}","\\frac{1}{x",
        std::string(2049,'x'),std::string(17,'{')+"x"+std::string(17,'}'),
        "\\newcommand{\\a}{x}\\a","\\begin{array}x\\end{array}","\\input{secret}","é"};
    std::string commands; for(int i=0;i<129;++i) commands+="\\alpha ";
    rejected.push_back(commands);
    rejected.push_back(std::string(65,'&'));
    rejected.push_back("\\");
    for(const auto& s:rejected) {
        check(ink_math_render(s.c_str(),1,24,bitmap,&r)!=0,"rejection test failed");
        check(std::all_of(std::begin(bitmap),std::end(bitmap),[](uint8_t b){return b==0;}),
              "failure left partial pixels");
    }
    check(ink_math_render("x_1",0,24,bitmap,&r)==0 && r.baseline>8,"recovery after errors failed");
    check(ink_math_render("x",0,41,bitmap,&r)!=0,"size budget not enforced");
    for(int pixels:{12,24,40}) {
        check(ink_math_render("-",0,pixels,bitmap,&r)==0,"minus rendering failed");
        check(std::any_of(std::begin(bitmap),std::end(bitmap),[](uint8_t b){return b!=0;}),
              "minus vanished (font hinting regression)");
    }
    for(const char *source:{"\\lim_{n\\to\\infty}", "\\min_{x}"}) {
        ink_math_result in, display;
        check(ink_math_render(source,0,24,bitmap,&in)==0,"inline operator failed");
        check(ink_math_render(source,1,24,bitmap,&display)==0,"display operator failed");
        check(display.height>in.height && display.width<in.width,
              "operator limits must move below in display and stay beside inline");
    }
    std::cerr<<"preflight/recovery/glyph/operator tests passed (18 cases)\n";
}
int main(int argc,char **argv) {
    try {
        check(argc==4,"usage: math-probe RESOURCE_ROOT CORPUS OUTPUT_DIR");
        check(std::filesystem::file_size(argv[2])<=65536,"host corpus exceeds 64 KiB");
        std::ifstream input(argv[2]); check(bool(input),"cannot read corpus");
        std::string document((std::istreambuf_iterator<char>(input)),{});
        Context c;
        MD_PARSER parser{}; parser.flags=MD_FLAG_TABLES|MD_FLAG_STRIKETHROUGH|MD_FLAG_LATEXMATHSPANS;
        parser.enter_block=block; parser.leave_block=block;
        parser.enter_span=enter; parser.leave_span=leave; parser.text=text;
        check(md_parse(document.data(),document.size(),&parser,&c)==0,"MD4C failed");
        check(c.formulas.size()==29,"corpus extraction count changed");
        check(std::count_if(c.formulas.begin(),c.formulas.end(),[](const Math& m){return m.display;})==9,
              "inline/display extraction changed");
        for(const auto& f:c.formulas)
            check(f.source.find("not_math")==std::string::npos && f.source.find("unfinished")==std::string::npos,
                  "literal dollar/code treated as math");
        std::filesystem::create_directories(argv[3]);
        size_t before=mallinfo2().uordblks;
        char error[160]; check(ink_math_init(argv[1],error,sizeof(error))==0,error);
        size_t initialized=mallinfo2().uordblks, sampled_peak=initialized;
        std::cout<<"id\tstyle\tstatus\twidth\theight\tbaseline\tmicroseconds\terror\n";
        size_t successes=0;
        for(size_t i=0;i<c.formulas.size();++i) {
            auto& f=c.formulas[i]; ink_math_result r;
            auto start=std::chrono::steady_clock::now();
            int status=ink_math_render(f.source.c_str(),f.display,24,bitmap,&r);
            auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
            check((status==0)==(i<26),"corpus render/fallback expectation failed");
            sampled_peak=std::max(sampled_peak,mallinfo2().uordblks);
            std::string stem=std::string(argv[3])+"/"+std::to_string(i+1);
            std::ofstream(stem+".tex")<<f.source;
            if(!status) {
                ++successes;
                std::ofstream out(stem+".pbm",std::ios::binary);
                // Full page width prevents losing right overhang; trim only blank rows.
                int last=0;
                for(int y=0;y<INK_MATH_HEIGHT;++y)
                    for(int x=0;x<INK_MATH_WIDTH/8;++x) if(bitmap[y*60+x]) last=y;
                int rows=std::min(int(INK_MATH_HEIGHT),std::max(r.height,last+9));
                out<<"P4\n480 "<<rows<<"\n";
                out.write(reinterpret_cast<const char*>(bitmap),60*rows);
            }
            std::cout<<i+1<<'\t'<<(f.display?"display":"inline")<<'\t'
                <<(status?"fallback":"rendered")<<'\t'<<r.width<<'\t'<<r.height<<'\t'
                <<r.baseline<<'\t'<<us<<'\t'<<r.error<<'\n';
        }
        tests();
        ink_math_shutdown();
        struct rusage usage{}; getrusage(RUSAGE_SELF,&usage);
        std::cerr<<"formulas="<<c.formulas.size()<<" rendered="<<successes
            <<" fallback="<<c.formulas.size()-successes<<"\n"
            <<"glibc_heap_before_init="<<before<<" after_init="<<initialized
            <<" max_post_formula_sample="<<sampled_peak<<" after_release="<<mallinfo2().uordblks
            <<" peak_process_rss_kib="<<usage.ru_maxrss<<"\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}

