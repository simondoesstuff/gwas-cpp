#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <map>
#include <zlib.h>

// include files from include directory
#include "header.h"
#include "utils.h"
#include "blocks.h"
#include "compress.h"
#include "decompress.h"

using namespace std;

int main(int argc, char* argv[]) {
    // COMPRESSION STEPS

    // 0. read config options
    string config_file = argv[1];
    cout << "Reading config options from: " << config_file << endl;
    map<string, string> config_options;
    config_options = read_config_file(config_file);
    string gwas_file = config_options["gwas_file"];
    int block_size = stoi(config_options["block_size"]);
    string compressed_file = config_options["out_file"];
    vector<string> codecs_list = split_string(config_options["codecs"], ',');

    cout << "\t...gwas_file: " << gwas_file << endl;
    cout << "\t...block_size: " << config_options["block_size"] << endl;
    cout << "\t...out_file: " << config_options["out_file"] << endl;
    cout << "\t...codecs: " << config_options["codecs"] << endl;
    cout << "Done." << endl << endl;

    // 1. get first half of header
    /* HEADER FIRST HALF
     * num_columns = None
     * num_blocks = None
     * column_names_list = None
     * codecs_list = None
     * codec_headers_list = None
     * block_sizes_tuple = None
     * */
    cout << "Getting file information..." << endl;
    // open gwas file and get first two lines
    ifstream file(gwas_file);
    // read first two lines of file
    string line_1;
    string line_2;
    getline(file, line_1);
    getline(file, line_2);
    // close file
    file.close();
    // get delimiter to read in file
    char delimiter = get_delimiter(line_1);
    cout << "\t...delimiter: " << delimiter << endl;
    string column_names_str = get_column_names(line_1, delimiter);
    cout << "\t...column names: " << column_names_str << endl;
    // split column names by comma and store the column title of each column
    vector<string> column_names_list;
    istringstream ss(column_names_str);
    string token;
    while(std::getline(ss, token, ',')) {
        column_names_list.push_back(token);
    }
    // get number of columns
    int num_columns = column_names_list.size();
    cout << "\t...num columns: " << num_columns << endl;
    cout << "Done." << endl << endl;

    // 2. get blocks
    cout << "Making blocks..." << endl;
    vector<vector<vector<string>>> all_blocks;
    all_blocks = make_blocks(gwas_file, num_columns, block_size, delimiter);
    cout << "\t...made " << all_blocks.size() << " blocks of size " << block_size  << " or less." << endl;
    cout << "Done." << endl << endl;

    // 3. compress blocks AND get second half of header
    cout << "Compressing blocks..." << endl;
    vector<vector<string>> compressed_blocks;
    // compress each block and add to compressed_blocks
    for (auto const& block : all_blocks) {
        vector<string> compressed_block = compress_block(block, codecs_list);
        compressed_blocks.push_back(compressed_block);
    }
    // get block headers
    vector<vector<string>> block_headers;
    for (auto const& block : compressed_blocks) {
        vector<string> block_header = get_block_header(block);
        block_headers.push_back(block_header);
    }

    // print block headers
    cout << "block headers: " << endl;
    for (auto const& block_header : block_headers) {
        cout << convert_vector_to_string(block_header) << endl;
    }

    // compress block headers
    vector<string> compressed_block_headers;
    for (auto const& block_header : block_headers) {
        string string_block_header = convert_vector_to_string(block_header);
        string compressed_block_header = zlib_compress(string_block_header);
        compressed_block_headers.push_back(compressed_block_header);
    }
    cout << "Done." << endl << endl;

    // 4. get second half of header
    /* HEADER SECOND HALF
     * compressed block header ends
     * compressed block ends
     */

    int num_blocks = compressed_blocks.size();

    // get ending index of each compressed block header and compressed block
    vector<string> block_header_lengths;
    vector<string> block_lengths;
    int curr_byte_idx = 0;
    for (int curr_block_idx = 0; curr_block_idx < num_blocks; curr_block_idx++) {
        int curr_block_header_length = compressed_block_headers[curr_block_idx].length();
        curr_byte_idx += curr_block_header_length;
        cout << "block header length: " << curr_block_header_length << endl;
        block_header_lengths.push_back(to_string(curr_byte_idx));
        int curr_block_length = get_block_length(compressed_blocks[curr_block_idx]);
        curr_byte_idx += curr_block_length;
        cout << "block length: " << curr_block_length << endl;
        block_lengths.push_back(to_string(curr_byte_idx));
    }

    // print block header lengths and block lengths
    cout << "block header lengths: " << convert_vector_to_string(block_header_lengths) << endl;
    cout << "block lengths: " << convert_vector_to_string(block_lengths) << endl;


    // get size of first and last block
    int first_block_size = all_blocks[0][0].size();
    int last_block_size = all_blocks[all_blocks.size() - 1][0].size();
    string block_sizes = to_string(first_block_size) + "," + to_string(last_block_size);

    vector<string> header = {
            to_string(num_columns),
            to_string(num_blocks),
            column_names_str,
            convert_vector_to_string(block_header_lengths),
            convert_vector_to_string(block_lengths),
            block_sizes
    };

    // 5. compress header
    cout << "Compressing header..." << endl;
    string header_str = convert_vector_to_string(header);
    string compressed_header = zlib_compress(header_str);
    cout << "compressed header length: " << compressed_header.length() << endl;
    cout << "Done." << endl << endl;


    // 6. open file to write compressed header and compressed blocks
    cout << "Writing compressed file to: " << compressed_file << endl;
    ofstream compressed_gwas;
    compressed_gwas.open(compressed_file);
    // clear file contents
    compressed_gwas.clear();

    // 7. write compressed header to file
    // reserve first 4 bytes for size of compressed header
    int compressed_header_size = compressed_header.length();
    char * compressed_header_size_bytes = int_to_bytes(compressed_header_size);
    compressed_gwas.write(compressed_header_size_bytes, 4);
    // write compressed header to file
    compressed_gwas << compressed_header;

    // 8. write compressed blocks to file
    int block_idx = 0;
    for (auto const& block : compressed_blocks) {
        // write compressed block header
        string compressed_block_header = compressed_block_headers[block_idx];
        compressed_gwas << compressed_block_header;

        // write compressed block
        for (auto const& column : block) {
            compressed_gwas << column;
        }
        block_idx++;
    }
    compressed_gwas.close();
    cout << "Done." << endl << endl;

    return 0;
}
