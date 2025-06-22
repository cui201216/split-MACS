#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <getopt.h>
#include <sstream>

using namespace std;

// 解析命令行参数
struct Args {
    string input_file;
    int split_count = 0;
};

Args parse_args(int argc, char* argv[]) {
    Args args;
    int opt;
    while ((opt = getopt(argc, argv, "i:x:")) != -1) {
        switch (opt) {
            case 'i':
                args.input_file = optarg;
                break;
            case 'x':
                args.split_count = stoi(optarg);
                break;
            default:
                cerr << "Usage: " << argv[0] << " -i input_file -x split_count\n";
                exit(1);
        }
    }
    if (args.input_file.empty() || args.split_count <= 0) {
        cerr << "Error: Missing or invalid arguments\n";
        exit(1);
    }
    return args;
}

// 存储一个site的信息
struct Site {
    int id;
    double pos;
    double freq;
    string haplotype;
};

// 读取输入文件，获取前两行和site数据
void read_file(const string& filename, string& command_line, string& seed_line, vector<Site>& sites, int& total_haplotypes) {
    ifstream in(filename);
    if (!in) {
        cerr << "Error: Cannot open input file " << filename << endl;
        exit(1);
    }

    string line;
    // 读取第一行（COMMAND）
    getline(in, command_line);
    if (command_line.find("COMMAND:") != 0) {
        cerr << "Error: First line must start with 'COMMAND:'\n";
        exit(1);
    }
    // 读取第二行（SEED）
    getline(in, seed_line);
    if (seed_line.find("SEED:") != 0) {
        cerr << "Error: Second line must start with 'SEED:'\n";
        exit(1);
    }

    // 读取site数据
    total_haplotypes = 0;
    while (getline(in, line)) {
        if (line.find("SITE:") == 0) {
            Site site;
            istringstream iss(line);
            string temp;
            iss >> temp >> site.id >> site.pos >> site.freq >> site.haplotype; // 单倍型序列在同一行
            if (total_haplotypes == 0) {
                total_haplotypes = site.haplotype.length();
            } else if (site.haplotype.length() != total_haplotypes) {
                cerr << "Error: Inconsistent haplotype length at site " << site.id << endl;
                exit(1);
            }
            sites.push_back(site);
        }
    }
    in.close();
}

// 写入site数据到文件
void write_file(const string& filename, const string& command_line, const string& seed_line,
                const vector<Site>& sites, int start_idx, int count, const string& haplotype_info) {
    ofstream out(filename);
    if (!out) {
        cerr << "Error: Cannot open output file " << filename << endl;
        exit(1);
    }
    // 写入COMMAND行并附加单倍型信息
    out << command_line << " " << haplotype_info << "\n";
    // 写入SEED行
    out << seed_line << "\n";
    // 写入site数据
    for (const auto& site : sites) {
        out << "SITE:\t" << site.id << "\t" << site.pos << "\t" << site.freq << "\t"
            << site.haplotype.substr(start_idx, count) << "\n";
    }
    out.close();
}

int main(int argc, char* argv[]) {
    // 解析参数
    Args args = parse_args(argc, argv);

    // 读取输入文件
    string command_line, seed_line;
    int total_haplotypes;
    vector<Site> sites;
    read_file(args.input_file, command_line, seed_line, sites, total_haplotypes);

    // 验证分离数量
    if (args.split_count >= total_haplotypes) {
        cerr << "Error: Split count (" << args.split_count
            << ") must be less than total haplotypes (" << total_haplotypes << ")\n";
        return 1;
    }

    // 生成输出文件名
    string panel_file = args.input_file + ".panel";
    string query_file = args.input_file + ".query";

    // 准备单倍型信息
    string panel_haplotype_info = "first " + to_string(total_haplotypes - args.split_count) + " haplotypes";
    string query_haplotype_info = "last " + to_string(args.split_count) + " haplotypes";

    // 写入panel文件（前total_haplotypes - split_count个单倍型）
    write_file(panel_file, command_line, seed_line, sites, 0, total_haplotypes - args.split_count, panel_haplotype_info);

    // 写入query文件（后split_count个单倍型）
    write_file(query_file, command_line, seed_line, sites, total_haplotypes - args.split_count, args.split_count, query_haplotype_info);

    cout << "Panel file created: " << panel_file << " (" << total_haplotypes - args.split_count << " haplotypes)\n";
    cout << "Query file created: " << query_file << " (" << args.split_count << " haplotypes)\n";

    return 0;
}