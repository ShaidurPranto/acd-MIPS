#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <iomanip> // For formatting hexadecimal output

using namespace std;

// Map for opcodes
unordered_map<string, string> opcodeMap = {
    {"lw", "0000"}, {"nor", "0001"}, {"addi", "0010"}, {"bneq", "0011"}, {"beq", "0100"}, {"ori", "0101"}, {"andi", "0110"}, {"add", "0111"}, {"and", "1000"}, {"subi", "1001"}, {"sub", "1010"}, {"srl", "1011"}, {"sll", "1100"}, {"sw", "1101"}, {"or", "1110"}, {"j", "1111"}};

// Map for registers
unordered_map<string, string> registerMap = {
    {"$zero", "0000"}, {"$t1", "0001"}, {"$t2", "0010"}, {"$t3", "0011"}, {"$t4", "0100"}, {"$t0", "0101"}, {"$sp", "0110"}};

// Function to convert immediate/address values to binary
string toBinary(int value, int bits)
{
    return bitset<16>(value).to_string().substr(16 - bits, bits);
}

string binaryToTwosComplement(string binary)
{
    bool foundOne = false;
    for (int i = binary.length() - 1; i >= 0; i--)
    {
        if (binary[i] == '1' && !foundOne)
        {
            foundOne = true;
            continue;
        }
        binary[i] = (binary[i] == '0') ? '1' : '0';
    }
    return binary;
}

// Function to convert binary string to hexadecimal string
string binaryToHex(const string &binary)
{
    stringstream ss;
    unsigned long decimalValue = bitset<32>(binary).to_ulong(); // Convert binary to decimal
    ss << hex /* << uppercase */ << setw(binary.length() / 4) << setfill('0') << decimalValue; // Convert decimal to hex
    return ss.str();
}

unordered_map<string, int> labelMap;

// Function to parse and convert assembly instruction to binary
string convertToBinary(const string &line, int lineNumber)
{
    stringstream ss(line);
    string instruction, rd, rs, rt, imm, jumpAddress, shiftAmt;
    ss >> instruction;

    // Identify instruction type based on opcode
    string opcode = opcodeMap[instruction];
    string binaryInstruction;
    
    // a comment start with #
    if (instruction[0] == '#') 
    {
        cerr << "COMMENT " << line << endl;
    }
    else if (instruction == "j")
    {
        // J-type: Opcode + Address
        // two possiblity either a number was given or a label
        ss >> jumpAddress;
        int address;
        if (isdigit(jumpAddress[0])) 
        {
            address = stoi(jumpAddress);
        }
        else 
        {
            address = labelMap[jumpAddress];
        }

        binaryInstruction = opcode + toBinary(address, 8) + "00000000";

        cerr << "JMP " << opcode << " " << toBinary(address, 8) << " " << "00000000" << endl;
    }
    else if (instruction == "sub" || instruction == "and" || instruction == "or" || 
             instruction == "nor" || instruction == "add") 
    {
        // NORMAL R TYPE
        // R-type: Opcode + rs + rt + rd + shift
        ss >> rd >> rs >> rt;
        binaryInstruction = opcode + registerMap[rs] + registerMap[rt] + registerMap[rd] + "0000";

        cerr << "ALU " << opcode << " " << registerMap[rs] << " " << registerMap[rt] << " " << registerMap[rd] << " " << "0000" << endl;
    }
    else if (instruction == "srl" || instruction == "sll")
    {
        // SHIFT R TYPE
        // R-type: Opcode + rs + rt + rd + shift
        ss >> rd >> rt >> shiftAmt;
        int shiftAmount = stoi(shiftAmt);
        // WHEN SHIFTING USE RS AS NUMBER , RT AS DESTINATION REGISTER , SHIFT AMOUNT AS SHIFT AMOUNT
        binaryInstruction = opcode + registerMap[rt] + registerMap[rd] + toBinary(shiftAmount, 8);

        cerr << "SHI " << opcode << " " << registerMap[rt] << " " << registerMap[rd] << " " << toBinary(shiftAmount, 8) << endl;
    }
    else if(instruction == "lw"   || instruction == "sw"   || 
            instruction == "addi" || instruction == "subi" || 
            instruction == "andi" || instruction == "ori")
    {
        // I-type: Opcode + rs + rt + Immediate
        ss >> rt >> rs >> imm;
        int immediate = stoi(imm);

        if(immediate < 0) 
        {
            immediate = immediate + 512;
        }

        binaryInstruction = opcode + registerMap[rs] + registerMap[rt] + toBinary(immediate, 8);
    
        cerr << "IMM " << opcode << " " << registerMap[rs] << " " << registerMap[rt] << " " << toBinary(immediate, 8) << endl;
    }
    else if(instruction == "beq" || instruction == "bneq")
    {
        // BRANCH I TYPE
        // I-type: Opcode + rs + rt + Immediate
        ss >> rt >> rs >> imm;
        // imm is a label or a address if number
        int address = labelMap[imm];
        
        // calculate the offset
        int offset = address - lineNumber - 1;
        if(offset < 0) offset = 256 + offset;

        binaryInstruction = opcode + registerMap[rs] + registerMap[rt] + toBinary(offset, 8);
        cerr << "BRANCH " << opcode << " " << registerMap[rs] << " " << registerMap[rt] << " " << toBinary(offset, 8) << endl;
        return binaryInstruction;
    }
    else {
        // LABEL
        // LABEL: Opcode + Address
        string label = instruction;
        label.pop_back();
        labelMap[label] = lineNumber;
        cerr << "LABEL " << label << " " << lineNumber << endl;

        return "";
    }

    return binaryInstruction;
}


int main()
{
    string path = "C://Users//mdsha//OneDrive - BUET//Documents//academics//CSE 210 - Archi//MIPS//ASSEMBLY//";
    string binaryPath = "C://Users//mdsha//OneDrive - BUET//Documents//academics//CSE 210 - Archi//MIPS//BINARY//";
    
    string filename = "test1.mips";
    // string filename = "REGISTER_DEMO.mips";

    string inputFileName = path;
    inputFileName += filename;

    string outputFilename = binaryPath;
    outputFilename += "BIN_";
    outputFilename += filename.substr(0, filename.find("."));
    outputFilename += ".txt";

    ifstream inputFile(inputFileName);
    ofstream outputFile(outputFilename);

    if (!inputFile.is_open() || !outputFile.is_open())
    {
        cerr << "Error opening file." << endl;
        return 1;
    }

    outputFile << "v2.0 raw" << endl; // Set output file format to raw

    string line;
    int lineNumber = 0;
    while (getline(inputFile, line))
    {
        if (line.empty() || line == "\n") continue;
        string binary = convertToBinary(line, lineNumber);
        if(binary.empty() || binary == "") continue;
        string hexValue = binaryToHex(binary); // Convert binary to hexadecimal
        outputFile << hexValue << endl; // Save hexadecimal value to file
        // cerr << binary << endl; // Print binary value to console
        lineNumber++;
    }

    inputFile.close();
    outputFile.close();

    cout << "Conversion complete. Check '" << outputFilename << "' for results in hexadecimal format." << endl;

    return 0;
}

/*

addi $t0 $zero 10
addi $t1 $zero 20
sw   $t0 $zero 0
sw   $t1 $zero 1 
lw   $t0 $zero 0
lw   $t1 $zero 1
add  $t0 $t1 $t2
sll  $t2 $t2 1
sw   $t2 $zero 2 
j    0

*/