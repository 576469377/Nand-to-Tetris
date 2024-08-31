#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <vector>
#include <sstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unordered_map>

std::unordered_map<std::string, std::string> callee_caller;
std::string file_name;
std::vector<std::string> file_name_list; // corresponds to each command
bool has_sys_file = false;

bool is_directory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        // Cannot access the path
        return false;
    } else if (info.st_mode & S_IFDIR) {
        // Check if it is a directory
        return true;
    } else {
        // Not a directory
        return false;
    }
}

std::string generate_output_file_name(const std::string &input_file_name) {
    std::string output_file_name;

    // Check if input_file_name is a directory
    if (is_directory(input_file_name)) {
        // If it's a directory, use the directory name to create output file path
        size_t lastslash = input_file_name.find_last_of("/\\");
        if (lastslash == std::string::npos) {
            file_name = input_file_name;
            output_file_name = input_file_name + "/" + input_file_name + ".asm";
        } else {
            file_name = input_file_name.substr(lastslash + 1);
            output_file_name = input_file_name + "/" + file_name + ".asm";
        }
    } else {
        // If it's a file, proceed to extract the file name without extension
        size_t lastdot = input_file_name.find_last_of(".");
        size_t lastslash = input_file_name.find_last_of("/\\");
        
        if (lastslash == std::string::npos) {
            file_name = (lastdot == std::string::npos) ? input_file_name : input_file_name.substr(0, lastdot);
        } else {
            file_name = (lastdot == std::string::npos) ? 
                        input_file_name.substr(lastslash + 1) : 
                        input_file_name.substr(lastslash + 1, lastdot - lastslash - 1);
        }

        // Set the output file name by replacing or appending the ".asm" extension
        output_file_name = (lastdot == std::string::npos) ? input_file_name + ".asm" : input_file_name.substr(0, lastdot) + ".asm";
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
			file_name_list.push_back(file_name);
		}
	}

	file.close();
	return commands;
}

bool is_vm_file(const std::string &filename) {
    return filename.size() > 3 && filename.substr(filename.size() - 3) == ".vm";
}

std::vector<std::string> read_all_vm_files(const std::string &path) {
    std::vector<std::string> all_commands;
    struct stat s;

    if (stat(path.c_str(), &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            DIR* dir;
            struct dirent* ent;
            if ((dir = opendir(path.c_str())) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    std::string cur_file_name = ent->d_name;
                    std::string full_path = path + "/" + cur_file_name;

                    if (is_vm_file(cur_file_name)) {
                        if (cur_file_name == "Sys.vm") has_sys_file = true;

                        // Extract the file name without the extension
                        size_t lastdot = cur_file_name.find_last_of(".");
                        file_name = (lastdot == std::string::npos) ? cur_file_name : cur_file_name.substr(0, lastdot);
                        std::vector<std::string> file_commands = read_vm_file(full_path);
                        all_commands.insert(all_commands.end(), file_commands.begin(), file_commands.end());
                    }
                }
                closedir(dir);
            } else {
                std::cerr << "Could not open directory: " << path << std::endl;
            }
        } else if (s.st_mode & S_IFREG) {
            if (is_vm_file(path)) {
                std::string cur_file_name = path.substr(path.find_last_of("/\\") + 1);
                size_t lastdot = cur_file_name.find_last_of(".");
                file_name = (lastdot == std::string::npos) ? cur_file_name : cur_file_name.substr(0, lastdot);

                all_commands = read_vm_file(path);
            }
        }
    } else {
        std::cerr << "Error: Invalid file or directory" << std::endl;
    }

    return all_commands;
}


void process_commands(const std::vector<std::string> &commands, const std::string &output_file_name) {
	std::ofstream output_file(output_file_name);
	if (!output_file.is_open()) {
		std::cerr << "Error: Cannot open output file " << output_file_name << std::endl;
		return;
	}

	int label_counter = 1;
	int return_counter = 1;
	int command_line_counter = 0;
	std::string cur_function_name = "";

	// bootstrap code
	if (has_sys_file) {
		output_file << "@256" << std::endl;
		output_file << "D=A" << std::endl;
		output_file << "@SP" << std::endl;
		output_file << "M=D" << std::endl;
		// call Sys.init 0 is in main()
	}
	

	for (const auto &command: commands) {
		file_name = file_name_list[command_line_counter++];
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
				// every file has its own static variables
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
				// every file has its own static variables
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
		} else if (op == "label") {
			// label label
			std::string label = cur_function_name + "$" + parts[1];
			output_file << "(" << label << ")" << std::endl;
		} else if (op == "goto") {
			// goto label
			std::string label = cur_function_name + "$" + parts[1];
			output_file << "@" << label << std::endl;
			output_file << "0;JMP" << std::endl;
		} else if (op == "if-goto") {
			// if-goto label
			std::string label = cur_function_name + "$" + parts[1];
			// SP--
			// if (*(SP - 1) == 0) goto label
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@" << label << std::endl;
			output_file << "D;JNE" << std::endl;
		} else if (op == "call") {
			// call functionName nArgs
			std::string functionName = parts[1];
			int nArgs = std::stoi(parts[2]);
			std::string return_label = functionName + "$ret." + std::to_string(return_counter++);

			// assembly code that saves the caller's state,
			// sets up the function call, and then:
			// goto functionName

			// push returnAddress    // (A label declared below)
			// push LCL               // Saves LCL of the caller
			// push ARG               // Saves ARG of the caller
			// push THIS              // Saves THIS of the caller
			// push THAT              // Saves THAT of the caller
			// ARG = SP - 5 - nArgs   // Repositions ARG
			// LCL = SP               // Repositions LCL
			// goto functionName      // Transfers control to the called function
			// (returnAddress)        // Declares a label for the return-address

			// push returnAddress    // (A label declared below)
			output_file << "@" << return_label << std::endl;
			output_file << "D=A" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=M+1" << std::endl;

			// push LCL               // Saves LCL of the caller
			output_file << "@LCL" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=M+1" << std::endl;

			// push ARG               // Saves ARG of the caller
			output_file << "@ARG" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=M+1" << std::endl;

			// push THIS              // Saves THIS of the caller
			output_file << "@THIS" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=M+1" << std::endl;

			// push THAT              // Saves THAT of the caller
			output_file << "@THAT" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=M+1" << std::endl;

			// ARG = SP - 5 - nArgs   // Repositions ARG
			output_file << "@" << nArgs << std::endl;
			output_file << "D=A" << std::endl;
			output_file << "@5" << std::endl;
			output_file << "D=D+A" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "D=M-D" << std::endl;
			output_file << "@ARG" << std::endl;
			output_file << "M=D" << std::endl;

			// LCL = SP               // Repositions LCL
			output_file << "@SP" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@LCL" << std::endl;
			output_file << "M=D" << std::endl;

			// goto functionName      // Transfers control to the called function
			output_file << "@" << functionName << std::endl;
			output_file << "0;JMP" << std::endl;

			// (returnAddress)        // Declares a label for the return-address
			output_file << "(" << return_label << ")" << std::endl;
		} else if (op == "function") {
			// function functionName nVars
			std::string functionName = parts[1];
			int nVars = std::stoi(parts[2]);

			cur_function_name = functionName;

		    // (functionName)       // declares a label for the function entry
		    // repeat nVars times:  // nVars = number of lacal variables
		    // push 0               // initializes the local variables to 0

		    // (functionName)       // declares a label for the function entry
		    output_file << "(" << functionName << ")" << std::endl;

		    // repeat nVars times:  // nVars = number of lacal variables
		    // push constant 0               // initializes the local variables to 0
		    for (int i = 0; i < nVars; ++i) {
		    	output_file << "@0" << std::endl;
		    	output_file << "D=A" << std::endl;
		    	output_file << "@SP" << std::endl;
		    	output_file << "A=M" << std::endl;
		    	output_file << "M=D" << std::endl;
		    	output_file << "@SP" << std::endl;
		    	output_file << "M=M+1" << std::endl;
		    }
		} else if (op == "return") {
			// return

			// endFrame = LCL              // endFrame is a temporary variable
			// retAddr = *(endFrame - 5)   // gets the return address
			// *ARG = pop()                // Repositions the return value for the caller
			// SP = ARG + 1                // Repositions SP of the caller
			// THAT = *(endFrame - 1)      // Restores THAT of the caller
			// THIS = *(endFrame - 2)      // Restores THIS of the caller
			// ARG = *(endFrame - 3)       // Restores ARG of the caller
			// LCL = *(endFrame - 4)       // Restores LCL of the caller
			// goto retAddr                // goes to return address in the caller's code

			// endFrame = LCL              // endFrame is a temporary variable
			output_file << "@LCL" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@R13" << std::endl;
			output_file << "M=D" << std::endl;

			// retAddr = *(endFrame - 5)   // gets the return address
			output_file << "@5" << std::endl;
			output_file << "A=D-A" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@R14" << std::endl;
			output_file << "M=D" << std::endl;

			// *ARG = pop()                // Repositions the return value for the caller
			output_file << "@SP" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@ARG" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "M=D" << std::endl;

			// SP = ARG + 1                // Repositions SP of the caller
			output_file << "@ARG" << std::endl;
			output_file << "D=M+1" << std::endl;
			output_file << "@SP" << std::endl;
			output_file << "M=D" << std::endl;

			// THAT = *(endFrame - 1)      // Restores THAT of the caller
			output_file << "@R13" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@THAT" << std::endl;
			output_file << "M=D" << std::endl;

			// THIS = *(endFrame - 2)      // Restores THIS of the caller
			output_file << "@R13" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@THIS" << std::endl;
			output_file << "M=D" << std::endl;

			// ARG = *(endFrame - 3)       // Restores ARG of the caller
			output_file << "@R13" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@ARG" << std::endl;
			output_file << "M=D" << std::endl;

			// LCL = *(endFrame - 4)       // Restores LCL of the caller
			output_file << "@R13" << std::endl;
			output_file << "AM=M-1" << std::endl;
			output_file << "D=M" << std::endl;
			output_file << "@LCL" << std::endl;
			output_file << "M=D" << std::endl;

			// goto retAddr                // goes to return address in the caller's code
			output_file << "@R14" << std::endl;
			output_file << "A=M" << std::endl;
			output_file << "0;JMP" << std::endl;
		}

	}

	output_file.close();
	std::cout << "Translation complete. Output written to " << output_file_name << std::endl;
}

int main(int argc, char *argv[]) {
	// processing error input
	if (argc != 2) {
		std::cerr << "Error input" << std::endl;
		return 1;
	}

	std::string input_file_name = argv[1];
	std::vector<std::string> commands = read_all_vm_files(input_file_name);
	if (has_sys_file) {
		commands.insert(commands.begin(), "call Sys.init 0");
		file_name_list.insert(file_name_list.begin(), " ");
	}

	// empty file
	if (commands.empty()) {
		std::cerr << "No valid VM commands found in file " << input_file_name << std::endl;
		return 1;
	}

	std::string output_file_name = generate_output_file_name(input_file_name);
	process_commands(commands, output_file_name);

	return 0;
}