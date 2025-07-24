# Huffman Compression Tool

A C++ implementation of file compression and decompression using **Huffman Coding**.

## 🚀 Features

- Compress text files using Huffman encoding.
- Decompress encoded binary files.
- Displays Huffman codes used for encoding.

## 🧠 How It Works

1. Counts frequency of each character in the input file.
2. Builds a Huffman Tree based on those frequencies.
3. Generates binary codes for each character.
4. Encodes the text and writes compressed binary data.
5. Stores decoding metadata (codes, lengths, padding).
6. Decodes the file using the metadata to reproduce the original content.

## 📂 Files

- `main.cpp` — Complete implementation of the compression tool.

## ⚙️ Compilation

Use `g++` to compile the project:

```bash
g++ -o huffman main.cpp
