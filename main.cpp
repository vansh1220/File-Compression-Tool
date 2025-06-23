#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include <algorithm> // For std::reverse

// Define MAX for binary conversions, though a dynamic approach is better.
// Assuming 16 bits is sufficient for character codes
const int MAX_CODE_LENGTH = 16;

// --- Huffman Tree Node Structure ---
struct Node {
    char character;
    int freq;
    Node *left, *right;

    // Constructor for leaf nodes
    Node(char c, int f)
        : character(c), freq(f), left(nullptr), right(nullptr) {}

    // Constructor for internal nodes
    Node(int f, Node* l, Node* r)
        : character('$'), freq(f), left(l), right(r) {}

    // Destructor to deallocate memory
    ~Node() {
        delete left;
        delete right;
    }

    // Check if it's a leaf node
    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

// --- Comparator for Min-Heap (Priority Queue) ---
// This will order nodes by frequency (smallest first)
struct CompareNodes {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

// --- Global map to store Huffman codes ---
std::map<char, std::string> huffmanCodes;

// --- Function to build Huffman codes recursively ---
void generateCodes(Node* root, std::string code) {
    if (!root) {
        return;
    }

    if (root->isLeaf()) {
        huffmanCodes[root->character] = code;
        return;
    }

    generateCodes(root->left, code + "0");
    generateCodes(root->right, code + "1");
}

// --- Function to build Huffman Tree ---
Node* buildHuffmanTree(const std::map<char, int>& frequencies) {
    std::priority_queue<Node*, std::vector<Node*>, CompareNodes> minHeap;

    // Create a leaf node for each character and add to min-heap
    for (auto const& [character, freq] : frequencies) {
        minHeap.push(new Node(character, freq));
    }

    // Continue until only one node remains in the heap (the root of the Huffman tree)
    while (minHeap.size() > 1) {
        // Extract the two nodes with the smallest frequencies
        Node* left = minHeap.top();
        minHeap.pop();
        Node* right = minHeap.top();
        minHeap.pop();

        // Create a new internal node with these two nodes as children
        // The frequency of the new node is the sum of its children's frequencies
        Node* top = new Node(left->freq + right->freq, left, right);
        minHeap.push(top);
    }

    return minHeap.top(); // The remaining node is the root of the Huffman tree
}

// --- Helper for converting binary string to decimal ---
int binaryToDecimal(const std::string& bin) {
    int dec = 0;
    int power = 1;
    for (int i = bin.length() - 1; i >= 0; --i) {
        if (bin[i] == '1') {
            dec += power;
        }
        power *= 2;
    }
    return dec;
}

// --- Helper for converting decimal to binary string (padded to length) ---
std::string decimalToBinary(int dec, int length) {
    std::string bin;
    while (dec > 0) {
        bin = (dec % 2 == 0 ? "0" : "1") + bin;
        dec /= 2;
    }
    // Pad with leading zeros if necessary
    while (bin.length() < length) {
        bin = "0" + bin;
    }
    return bin;
}

// --- Compression Function ---
void compressFile(const std::string& inputFile, const std::string& outputFile, Node* huffmanRoot) {
    std::ifstream ifs(inputFile, std::ios::binary);
    std::ofstream ofs(outputFile, std::ios::binary);

    if (!ifs.is_open() || !ofs.is_open()) {
        std::cerr << "Error opening files for compression." << std::endl;
        return;
    }

    // --- Write Huffman Tree metadata to output file ---
    // This is a simplified way to store the tree for decompression.
    // A more robust method would involve serializing the tree structure itself.
    // Here, we'll write character, code length, and decimal representation of code.
    // The number of unique characters needs to be written first.
    int uniqueCharCount = huffmanCodes.size();
    ofs.write(reinterpret_cast<const char*>(&uniqueCharCount), sizeof(int));

    for (auto const& [character, code] : huffmanCodes) {
        ofs.write(reinterpret_cast<const char*>(&character), sizeof(char));
        int codeLength = code.length();
        ofs.write(reinterpret_cast<const char*>(&codeLength), sizeof(int));
        int decimalCode = binaryToDecimal(code);
        ofs.write(reinterpret_cast<const char*>(&decimalCode), sizeof(int));
    }

    // --- Write compressed data ---
    std::string bitBuffer = "";
    char ch;
    while (ifs.get(ch)) {
        bitBuffer += huffmanCodes[ch];
        // Write full bytes to file
        while (bitBuffer.length() >= 8) {
            unsigned char byte = static_cast<unsigned char>(binaryToDecimal(bitBuffer.substr(0, 8)));
            ofs.write(reinterpret_cast<const char*>(&byte), sizeof(unsigned char));
            bitBuffer = bitBuffer.substr(8);
        }
    }

    // Handle any remaining bits (pad with zeros to form a full byte)
    if (!bitBuffer.empty()) {
        int paddingBits = 8 - bitBuffer.length();
        for (int i = 0; i < paddingBits; ++i) {
            bitBuffer += '0'; // Pad with zeros
        }
        unsigned char byte = static_cast<unsigned char>(binaryToDecimal(bitBuffer.substr(0, 8)));
        ofs.write(reinterpret_cast<const char*>(&byte), sizeof(unsigned char));

        // Optionally, write the number of padding bits as metadata for proper decompression
        ofs.write(reinterpret_cast<const char*>(&paddingBits), sizeof(int));
    } else {
        // If the last byte was exactly 8 bits, and no padding was needed, still write 0 padding.
        int paddingBits = 0;
        ofs.write(reinterpret_cast<const char*>(&paddingBits), sizeof(int));
    }

    ifs.close();
    ofs.close();
    std::cout << "File compressed successfully." << std::endl;
}

// --- Decompression Function ---
void decompressFile(const std::string& compressedFile, const std::string& decompressedFile) {
    std::ifstream ifs(compressedFile, std::ios::binary);
    std::ofstream ofs(decompressedFile, std::ios::binary);

    if (!ifs.is_open() || !ofs.is_open()) {
        std::cerr << "Error opening files for decompression." << std::endl;
        return;
    }

    // --- Rebuild Huffman Tree from metadata ---
    int uniqueCharCount;
    ifs.read(reinterpret_cast<char*>(&uniqueCharCount), sizeof(int));

    std::map<std::string, char> huffmanDecodingMap;
    Node* huffmanRoot = new Node(0, nullptr, nullptr); // Dummy root for building

    for (int i = 0; i < uniqueCharCount; ++i) {
        char character;
        int codeLength;
        int decimalCode;

        ifs.read(reinterpret_cast<char*>(&character), sizeof(char));
        ifs.read(reinterpret_cast<char*>(&codeLength), sizeof(int));
        ifs.read(reinterpret_cast<char*>(&decimalCode), sizeof(int));

        std::string code = decimalToBinary(decimalCode, codeLength);
        huffmanDecodingMap[code] = character;

        // Rebuild tree structure from codes
        Node* current = huffmanRoot;
        for (char bit : code) {
            if (bit == '0') {
                if (!current->left) {
                    current->left = new Node(0, nullptr, nullptr);
                }
                current = current->left;
            } else { // bit == '1'
                if (!current->right) {
                    current->right = new Node(0, nullptr, nullptr);
                }
                current = current->right;
            }
        }
        current->character = character; // Mark the leaf node
    }

    // Read padding information (last int in the file)
    ifs.seekg(-sizeof(int), std::ios::end); // Move to the end minus size of int
    int paddingBits;
    ifs.read(reinterpret_cast<char*>(&paddingBits), sizeof(int));
    ifs.seekg(sizeof(int) + uniqueCharCount * (sizeof(char) + 2 * sizeof(int)), std::ios::beg); // Move back to start of compressed data


    // --- Decompress data ---
    unsigned char byte;
    std::string bitStream = "";
    Node* current = huffmanRoot;

    // Read all bytes except the last one (which contains padding info)
    std::vector<unsigned char> compressedBytes;
    char tempByte;
    while(ifs.get(tempByte)) {
        compressedBytes.push_back(static_cast<unsigned char>(tempByte));
    }

    // Process all bytes except the last one (which holds padding information)
    for (size_t i = 0; i < compressedBytes.size(); ++i) {
        byte = compressedBytes[i];
        bitStream += decimalToBinary(byte, 8);
    }
    
    // Remove padding from the last byte
    if (compressedBytes.size() > 0) {
        bitStream.resize(bitStream.length() - paddingBits);
    }

    for (char bit : bitStream) {
        if (bit == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        if (current->isLeaf()) {
            ofs.put(current->character);
            current = huffmanRoot; // Reset to root for next code
        }
    }

    delete huffmanRoot; // Clean up the rebuilt tree
    ifs.close();
    ofs.close();
    std::cout << "File decompressed successfully." << std::endl;
}

// --- Main function for demonstration ---
int main() {
    std::string inputFileName = "input.txt";
    std::string compressedFileName = "compressed.bin";
    std::string decompressedFileName = "decompressed.txt";

    // Create a dummy input file for testing
    std::ofstream testInput(inputFileName);
    if (testInput.is_open()) {
        testInput << "this is a test string for huffman coding. a simple test to see if it works.";
        testInput.close();
        std::cout << "Created " << inputFileName << std::endl;
    } else {
        std::cerr << "Error creating input.txt" << std::endl;
        return 1;
    }

    // --- Step 1: Calculate character frequencies ---
    std::map<char, int> frequencies;
    std::ifstream ifs(inputFileName, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error opening " << inputFileName << std::endl;
        return 1;
    }
    char ch;
    while (ifs.get(ch)) {
        frequencies[ch]++;
    }
    ifs.close();

    // --- Step 2: Build Huffman Tree ---
    Node* huffmanRoot = buildHuffmanTree(frequencies);

    // --- Step 3: Generate Huffman Codes ---
    generateCodes(huffmanRoot, "");

    std::cout << "\nHuffman Codes:" << std::endl;
    for (auto const& [character, code] : huffmanCodes) {
        if (character == '\n') {
            std::cout << "'\\n': " << code << std::endl;
        } else if (character == ' ') {
            std::cout << "' ': " << code << std::endl;
        } else {
            std::cout << "'" << character << "': " << code << std::endl;
        }
    }

    // --- Step 4: Compress the file ---
    compressFile(inputFileName, compressedFileName, huffmanRoot);

    // --- Step 5: Decompress the file ---
    decompressFile(compressedFileName, decompressedFileName);

    // Clean up the Huffman tree
    delete huffmanRoot;

    return 0;
}
