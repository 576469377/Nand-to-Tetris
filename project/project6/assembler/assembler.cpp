#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <unordered_map>

std::unordered_map<std::string, int> symbol_table;
int cnt = 16;

std::unordered_map<std::string, std::string> comp_table = {
	{"0", "0101010"},
	{"1", "0111111"},
	{"-1", "0111010"},
	{"D", "0001100"},
	{"A", "0110000"}, {"M", "1110000"},
	{"!D", "0001101"},
	{"!A", "0110001"}, {"!M", "1110001"},
	{"-D", "0001111"},
	{"-A", "0110011"}, {"-M", "1110011"},
	{"D+1", "0011111"},
	{"A+1", "0110111"}, {"M+1", "1110111"},
	{"D-1", "0001110"},
	{"A-1", "0110010"}, {"M-1", "1110010"},
	{"D+A", "0000010"}, {"D+M", "1000010"},
	{"D-A", "0010011"}, {"D-M", "1010011"},
	{"A-D", "0000111"}, {"M-D", "1000111"},
	{"D&A", "0000000"}, {"D&M", "1000000"},
	{"D|A", "0010101"}, {"D|M", "1010101"}
};
std::unordered_map<std::string, std::string> dest_table = {
	{"null", "000"},
	{"M", "001"},
	{"D", "010"},
	{"MD", "011"},
	{"A", "100"},
	{"AM", "101"},
	{"AD", "110"},
	{"AMD", "111"}
};
std::unordered_map<std::string, std::string> jump_table = {
	{"null", "000"},
	{"JGT", "001"},
	{"JEQ", "010"},
	{"JGE", "011"},
	{"JLT", "100"},
	{"JNE", "101"},
	{"JLE", "110"},
	{"JMP", "111"}
};



void init_symbol_table() {
	// predefined symbol table
	for (int i = 0; i < 16; ++i) {
		symbol_table["R" + std::to_string(i)] = i;
	}
	symbol_table["SCREEN"] = 16384;
	symbol_table["KBD"] = 24576;
	symbol_table["SP"] = 0;
	symbol_table["LCL"] = 1;
	symbol_table["ARG"] = 2;
	symbol_table["THIS"] = 3;
	symbol_table["THAT"] = 4;
}

std::string translate(const std::string &str, int count) {
	std::string res = "";
	if (str[0] == '@') { // A instruction
		std::string ins = "";
		for (const char &c: str) {
			if (c != '@') ins += c;
		}
		if (isdigit(ins[0])) {
			int dec_num = std::stoi(ins);
			std::bitset<15> binary(dec_num);
			// convert to binary machine language
			res = std::to_string(0) + binary.to_string();
		} else {
			if (!symbol_table.count(ins)) {
				symbol_table[ins] = cnt++;
			}
			std::bitset<15> binary(symbol_table[ins]);
			// convert to binary machine language
			res = std::to_string(0) + binary.to_string();
		}
	} else { // C instruction
		std::string dest = "", comp = "", jump = "";
		std::string tmp = "";
		bool is_jump = 0;
		for (const char &c: str) {
			if (c == '=') {
				dest = tmp;
				tmp = "";
			} else if (c == ';') {
				is_jump = 1;
				comp = tmp;
				tmp = "";
			} else tmp += c;
		}
		if (is_jump) jump = tmp;
		else comp = tmp;
		if (dest == "") dest = "null";
		if (jump == "") jump = "null";
		// convert to binary machine language
		res = std::to_string(111) + comp_table[comp] + dest_table[dest] + jump_table[jump];
	}
	return res;
}

int main() {
	std::string input_filename;
	std::cout << "input file name: " << std::endl;
	std::cin >> input_filename;

    // open file
	std::ifstream infile("asm/" + input_filename);
	if (!infile) {
		std::cerr << "can't open the file" << std::endl;
		return 1;
	}

	// get input name(not include extend name)
	size_t dot_position = input_filename.find_last_of('.');
    std::string base_filename = (dot_position == std::string::npos) ? input_filename : input_filename.substr(0, dot_position);
    std::string output_filename = base_filename + ".hack"; // define the name of output file

    // create and open the output file
	std::ofstream outfile("hack/" + output_filename);
	if (!outfile) {
		std::cerr << "can't create output file" << std::endl;
		return 1;
	}

	// initialize the symbol table
	init_symbol_table();

	// read and process file line by line
	std::string line;
	int count = -1;
	// process labels
	while (std::getline(infile, line)) {
		// skip empty lines
		if (line.empty()) {
            continue;
        }
		// remove extra spaces
        std::string str = "";
        for (const char &c: line) {
        	if (c != ' ') str += c;
        	else if (c == '/') break;
        }
        if (!str.length() || str[0] == '/') continue; // skip comment lines
        if (str[0] == '(') {
        	std::string ins = "";
			for (const char &c: str) {
				if (c == ')') break;
				if (c != '(') ins += c;
			}
			symbol_table[ins] = count + 1;
		} else ++count;
	}

	// reset the file pointer to the begining
	infile.clear();
    infile.seekg(0, std::ios::beg);

	// process instructions
	while (std::getline(infile, line)) {
		// skip empty lines
		if (line.empty()) {
            continue;
        }
        // remove extra spaces
        std::string str = "";
        for (const char &c: line) {
        	if (c != ' ') str += c;
        	else if (c == '/') break;
        }
        if (!str.length() || str[0] == '/') continue; // skip comment lines
		if (str[0] != '(') {
			++count;
			std::string processed_line = translate(str, count);
			std::cout << processed_line << std::endl;
			outfile << processed_line << std::endl;
		}
	}

	infile.close();
	outfile.close();

	std::cout << "success" << std::endl;

	return 0;
}