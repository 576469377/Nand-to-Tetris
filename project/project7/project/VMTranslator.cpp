#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <vector>
#include <sstream>

std::string file_name;

// generate the output file name with .asm extension
std::string generate_output_input_file_name(const std::string &input_file_name) {
	size_t lastdot = input_file_name.find_last_of(".");
	std::string output_file_name;
	if (lastdot == std::string::npos) {
		output_file_name = input_file_name + ".asm";
	} else output_file_name = input_file_name.substr(0, lastdot) + ".asm";

	// Extract the file name without path and extension
	size_t lastslash = output_file_name.find_last_of("/\\");
	if (lastslash == std::string::npos) {
		file_name = output_file_name.substr(0, lastdot);
	} else {
		file_name = output_file_name.substr(lastslash + 1, lastdot - lastslash - 1);
	}
	
	return output_file_name;
}

std::vector<std::string> read_vm_file(const std::string &input_file_name) {
	std::vector<std::string> commands;
	std::ifstream file(input_file_name);
	if (!file.is_open()) {
		std::cerr << "Error: Cannot open file " << input_file_name << std::endl;
		return commands;
	}

	std::string line;
	while (std::getline(file, line)) {
		// Remove comments and trim the line if needed
		if (line.find("//") != std::string::npos) {
			line = line.substr(0, line.find("//"));
		}
		// Trim whitespace from both ends of the line
		line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
		line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));

		if (!line.empty()) {
			commands.push_back(line);
		}
	}

	file.close();
	return commands;
}

void process_commands(const std::vector<std::string> &commands, const std::string &output_input_file_name) {
	std::ofstream output_file(output_input_file_name);
	if (!output_file.is_open()) {
		std::cerr << "Error: Cannot open output file " << output_input_file_name << std::endl;
		return;
	}

	int label_counter = 0;

	for (const auto &command: commands) {
		output_file << "// " << command << std::endl; // write VM command as a comment

		// translation logic

		// parse the command into parts
		std::istringstream iss(command);
	    std::vector<std::string> parts;
	    std::string part;
	    while (iss >> part) {
	        parts.push_back(part);
	    }
	    std::string op = parts[0];

		if (op == "add") {
			// SP--
			// *(SP - 1) = *(SP - 1) + *SP
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "A=A-1" << std::endl;
			output_file << "M=D+M" << std::endl;
		} else if (op == "sub") {
			// SP--
			// *(SP - 1) = *(SP - 1) - *SP
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "A=A-1" << std::endl;
			output_file << "M=M-D" << std::endl;
		} else if (op == "neg") {
			// *(SP - 1) = -*(SP - 1)
			output_file << "@SP" << std::endl;
			output_file << "A=M-1" << std::endl;
			output_file << "M=-M" << std::endl;
		} else if (op == "eq" || op == "gt" || op == "lt") {
			// SP--
			// if (*(SP - 1) op *SP) *(SP - 1) = 1
			// else *(SP - 1) = 0
			std::string jump;
			if (op == "eq") jump = "JEQ";
			if (op == "gt") jump = "JGT";
			if (op == "lt") jump = "JLT";
			std::string label_true = "TRUE" + std::to_string(label_counter);
			std::string label_end = "END" + std::to_string(label_counter);
			label_counter++;
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "A=A-1" << std::endl;
			output_file << "D=M-D" << std::endl;
			output_file << "@" << label_true << std::endl;
			output_file << "D;" << jump << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M-1" << std::endl;
			output_file << "M=0" << std::endl; // 0: 00000000
			output_file << "@" << label_end << std::endl;
			output_file << "0;JMP" << std::endl;
			output_file << "(" << label_true << ")" << std::endl;
			output_file << "  @SP" << std::endl;
			output_file << "  A=M-1" << std::endl;
			output_file << "  M=-1" << std::endl; // -1: 11111111
			output_file << "(" << label_end << ")" << std::endl;
		} else if (op == "and") {
			// SP--
			// *(SP - 1) = *(SP - 1) & *SP
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "A=A-1" << std::endl;
			output_file << "M=D&M" << std::endl;
		} else if (op == "or") {
			// SP--
			// *(SP - 1) = *(SP - 1) | *SP
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "A=A-1" << std::endl;
			output_file << "M=D|M" << std::endl;
		} else if (op == "not") {
			// *(SP - 1) = !(*(SP - 1))
			output_file << "@SP" << std::endl;
			output_file << "A=M-1" << std::endl;
			output_file << "M=!M" << std::endl;
		} else if (op == "pop") {
			// pop segment i
			std::string segment = parts[1];
			std::string i = parts[2];

			// pop segment i
			// local, argument, this, that
			// addr = segment_pointer + i, SP--, *addr = *SP
			if (segment == "local" || segment == "argument" || segment == "this" || segment == "that") {
				if (segment == "local") output_file << "@LCL" << std::endl;
				else if (segment == "argument") output_file << "@ARG" << std::endl;
				else if (segment == "this") output_file << "@THIS" << std::endl;
				else if (segment == "that") output_file << "@THAT" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@" + i << std::endl;
				output_file << "D=D+A" << std::endl;
				output_file << "@R13" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "AM=M-1" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@R13" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
			} else if (segment == "static") {
				// pop static i
				// addr = file_name.i, SP--, *addr = *SP
				output_file << "@SP" << std::endl;
				output_file << "AM=M-1" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@" << file_name << "." << i << std::endl;
				output_file << "M=D" << std::endl;
			} else if (segment == "temp") {
				// pop temp i
				// addr = R[5 + i], SP--, *addr = *SP
				output_file << "@SP" << std::endl;
				output_file << "AM=M-1" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@R" << 5 + stoi(i) << std::endl;
				output_file << "M=D" << std::endl;
			} else if (segment == "pointer") {
				// pop pointer 0/1
				// SP--, THIS/THAT = *SP
				output_file << "@SP" << std::endl;
				output_file << "AM=M-1" << std::endl;
				output_file << "D=M" << std::endl;
				if (i == "0") output_file << "@THIS" << std::endl;
				else if (i == "1") output_file << "@THAT" << std::endl;
				output_file << "M=D" << std::endl;
			}
		} else if (op == "push") {
			// push segment i
			std::string segment = parts[1];
			std::string i = parts[2];

			// push segment i
			// local, argument, this, that
			// addr = segment_pointer + i, *SP = *addr, SP++
			if (segment == "local" || segment == "argument" || segment == "this" || segment == "that") {
				if (segment == "local") output_file << "@LCL" << std::endl;
				else if (segment == "argument") output_file << "@ARG" << std::endl;
				else if (segment == "this") output_file << "@THIS" << std::endl;
				else if (segment == "that") output_file << "@THAT" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@" + i << std::endl;
				output_file << "A=D+A" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "M=M+1" << std::endl;
			} else if (segment == "constant") {
				// push constant i
				// *SP = i, SP++
				output_file << "@" + i << std::endl;
				output_file << "D=A" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "M=M+1" << std::endl;
			} else if (segment == "static") {
				// push static i
				// addr = file_name.i, *SP = *addr, SP++
				output_file << "@" << file_name << "." << i << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "M=M+1" << std::endl;
			} else if (segment == "temp") {
				// push temp i
				// addr = R[5 + i], *SP = *addr, SP++
				output_file << "@R" << 5 + stoi(i) << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "M=M+1" << std::endl;
			} else if (segment == "pointer") {
				// push pointer 0/1
				// *SP = THIS/THAT, SP++
				if (i == "0") output_file << "@THIS" << std::endl;
				else if (i == "1") output_file << "@THAT" << std::endl;
				output_file << "D=M" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "A=M" << std::endl;
				output_file << "M=D" << std::endl;
				output_file << "@SP" << std::endl;
				output_file << "M=M+1" << std::endl;
			}
		}

	}
	output_file << "(END)" << std::endl;
	output_file << "  @END" << std::endl;
	output_file << "  0;JMP" << std::endl;

	output_file.close();
	std::cout << "Translation complete. Output written to " << output_input_file_name << std::endl;
}

int main(int argc, char *argv[]) {
	// processing error input
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " fileName.vm" << std::endl;
		return 1;
	}

	std::string input_file_name = argv[1];
	std::vector<std::string> commands = read_vm_file(input_file_name);

	// empty file
	if (commands.empty()) {
		std::cerr << "No valid VM commands found in file " << input_file_name << std::endl;
		return 1;
	}

	std::string output_input_file_name = generate_output_input_file_name(input_file_name);
	process_commands(commands, output_input_file_name);

	return 0;
}