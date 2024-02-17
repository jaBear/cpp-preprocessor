#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

string GetLine(istream& in) {
    string s;
    getline(in, s);
    return s;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}



// напишите эту функцию
bool PrepFunc(const path& in_file, ofstream& out_file, const vector<path>& include_directories) {
    static regex include_in_quotes(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex include_in_brackets(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    int line_number = 0;
    if(!filesystem::exists(in_file)) {
        return false;
    } else {
        ifstream stream;
        stream.open(in_file);
        string line;
        
        while (getline(stream, line)) {
            line_number++;
            smatch m;
            
            if (regex_match(line, m, include_in_quotes)) {
                path new_path = in_file.parent_path() / path(m[1]);
                fstream fs(new_path);
                
                if (fs.is_open()) {
                    bool result = PrepFunc(new_path, out_file, include_directories);
                    if (!result) {
                        return false;
                    } else {
                        continue;
                    }
                    
                } else {
                    bool res = false;
                    path new_path2;
                    for (auto d : include_directories) {
                        fstream fs1(d / path(m[1]));
                        
                        if (fs1.is_open()) {
                            new_path2 = d / path(m[1]);
                            res = true;
                        }
                    }
                        if (res) {
                            res = false;
                            bool result = PrepFunc(new_path2, out_file, include_directories);
                            if (!result) {
                                return false;
                            } else {
                                continue;
                            }
                        } else
                        cout << "unknown include file "s << path(m[1]).filename().string() << " at file "s << path(in_file).string() << " at line "s << line_number << endl;
                        return false;
                    }
                    //если я пытаюсь делать 1 функцию, то все перестает работать. Прошу оставить как есть
                } else if(regex_match(line, m, include_in_brackets)) {
                    bool res = false;
                    path new_path2;
                    for (const path& d : include_directories) {
                        fstream fs1(d / path(m[1]));
                        if (fs1.is_open()) {
                            new_path2 = d / path(m[1]);
                            res = true;
                        }
                    }
                    
                    if (res) {
                        res = false;
                        bool result = PrepFunc(new_path2, out_file, include_directories);
                        if (!result) {
                            return false;
                        } else {
                            continue;
                        }
                    } else
                        cout << "unknown include file "s << path(m[1]).filename().string() << " at file "s << path(in_file).string() << " at line "s << line_number << endl;
                    return false;
                    
                } else {
                    out_file << line << '\n';
                }
            };
    }

    return true;

};
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    filesystem::remove(out_file);
    ofstream out(out_file, ios::app);
    
    return PrepFunc(in_file, out, include_directories);
}


void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }


    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
