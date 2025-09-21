#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "pktinfo.h"
#include "div.h"
#include "bmp.h"
#define GUARD_BAND_SIZE 8
#define PACKET_SIZE (32 * 4)
#define SEARCH_PATTERN {0x40, 0x01, 0x01, 0x00}
#define VIDEO_SEARCH_PATTERN {0xc0}

int q_htotal;
int q_vtotal;
int q_hactive;
int q_vactive;
int q_colordepth;
int q_colorspace;

bool compare_bytes(const uint8_t *bytes1, const uint8_t *bytes2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (bytes1[i] != bytes2[i]) {
            return false;
        }
    }
    return true;
}

uint8_t binaryToUint8(const char* binary) {
    uint8_t number = 0;

    for (int i = 0; i < 8; ++i) {
        if (binary[i] == '1') {
            number |= (1U << (7 - i));
        }
    }

    return number;
}

char* uint64ToFormattedBinaryString(uint64_t num) {
    char* binary = (char*)malloc(65 * sizeof(char));
    if (binary == NULL) return NULL;
    int index = 64;
    binary[index] = '\0';
    for (int i = 0; i < 64; ++i) {
        binary[--index] = (num & (1ULL << i)) ? '1' : '0';
    }
    return binary;
}


uint32_t reverse_bytes_in_32bit(uint32_t num) {
    uint32_t reversed = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t byte = (num >> (i * 8)) & 0xFF;
        uint8_t reversed_byte = reverse_byte(byte);
        reversed |= reversed_byte << (i * 8);
    }
    return reversed;
}

long getFileSize(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Failed to seek to end of file");
        fclose(file);
        return -1;
    }

    long size = ftell(file);
    if (size == -1L) {
        perror("Failed to get file size");
    }

    fclose(file);
    return size;
}



int hdmi_parse_packet(uint8_t (*data)[4])
{
    int ret = 0;
    uint32_t head = 0;
    uint32_t body_low32 = 0;
    uint32_t body_high32 = 0;
    for (int i = 0; i < 32; i++) {
        uint8_t fourth_byte = data[i][3];
        uint8_t bit2 = (fourth_byte >> 2) & 1;
        head |= bit2 << (31 - i);
    }

    char binary_str[33];
    char strQ[1000];
    char strR[1000];

    int_to_binary_string(head >> 8, binary_str);
    strm2div(binary_str, "111000001", strQ, strR);
    printf("head 32-bit result: 0x%08X\n", head);
    printf("ecc %s\n", strR);
    if (binaryToUint8(strR) != uint8_t(head & 0xff)) {
        uint8_t value = binaryToUint8(strR);
        printf("detect ecc err correct: 0x%x err: 0x%x\n", binaryToUint8(strR), head & 0xff);
        for (int i = 31; i >= 24; i--) {
            data[i][3] = (data[i][3] & ~0x04) | ((value & 1) << 2);
            value = value >> 1;
        }
        ret = -1;
    }

    head = reverse_bytes_in_32bit(head);
    int bit_position = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t byte2_bit0 = (data[i][2] & 1);
        body_low32 |= (byte2_bit0 << (31 - bit_position));
        bit_position++;
        uint8_t byte3_bit0 = (data[i][1] & 1);
        body_low32 |= (byte3_bit0 << (31 - bit_position));
        bit_position++;
    }
    printf("body_low 32-bit result: 0x%08X\n", body_low32);

    bit_position = 0;

    for (int i = 16; i < 32; i++) {
        uint8_t byte2_bit0 = (data[i][2] & 1);
        body_high32 |= (byte2_bit0 << (31 - bit_position));
        bit_position++;
        uint8_t byte3_bit0 = (data[i][1] & 1);
        body_high32 |= (byte3_bit0 << (31 - bit_position));
        bit_position++;
    }

    uint64_t body = uint64_t(body_low32) << 32 | body_high32;
    char* binaryString = uint64ToFormattedBinaryString(body >> 8);
    strm2div(binaryString, "111000001", strQ, strR);
    if (binaryToUint8(strR) != uint8_t(body & 0xff)) {
        uint8_t value = binaryToUint8(strR);
        for (int i = 31; i >= 28; i--) {
            data[i][1] = (data[i][2] & ~0x01) | ((value & 1));
            value = value >> 1;
            data[i][2] = (data[i][1] & ~0x01) | ((value & 1));
            value = value >> 1;
        }
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 4; j++) {
            }
        }
        ret = -1;
    }
    body_low32 = reverse_bytes_in_32bit(body_low32);
    body_high32 = reverse_bytes_in_32bit(body_high32);
    hdmi_parse_pkt_info(&head, &body_low32, &body_high32);
    return ret;
}

int hdmi_parse_blank_packet_and_fdet(char *input_file, int *htotal, int *hactive, int *vtotal, int *vactive, long *offset)
{
    FILE *file = fopen(input_file, "rb+");
    printf("file %s", input_file);
    if (!file) {
        perror("Failed to open file");
        return -1;
    }
    int h_cnt = 0;
    int h_b_cnt = 0;
    int hactive_detect = 0;
    int hblank_detect = 0;
    long frame_pixel = getFileSize(input_file);
    uint8_t pkt_data[32][4];
    unsigned char buffer[4 + GUARD_BAND_SIZE + PACKET_SIZE];
    unsigned char search_pattern[4] = SEARCH_PATTERN;
    unsigned char video_search_pattern[1] = VIDEO_SEARCH_PATTERN;
    int pattern_count = 0; // Counter for the search pattern occurrences
    int hactive_count = 0;
    int hblank_count = 0;
    bool first = true;
    while (1) {
        size_t bytes_read = fread(buffer, 1, 4, file);
        if (bytes_read != 4) {
            if (feof(file)) {
                break; // End of file reached
            } else {
                perror("Failed to read data");
                fclose(file);
                return -1;
            }
        }

        if (memcmp(buffer, search_pattern, 3) == 0) {
            pattern_count++;
            // Check if this is the 8th (or any other specific) occurrence
            // For simplicity, let's assume we're looking for the 8th non-consecutive occurrence
            // In a real-world scenario, you might have a more complex logic for this
            if (pattern_count == 8) { // Adjust the condition as needed
                // Read guard band
                size_t guard_bytes_read = fread(buffer, 1, GUARD_BAND_SIZE, file);
                if (guard_bytes_read != GUARD_BAND_SIZE) {
                    fprintf(stderr, "Guard band mismatch\n");
                    fclose(file);
                    return -1;
                }

                // Read packet
                size_t packet_bytes_read = fread(buffer, 1, PACKET_SIZE, file);
                for (int i = 0; i < 32; i++) {
                    for (int j = 0; j < 4; j++)
                        pkt_data[i][j] = buffer[i * 4 + j];
                }
                if (hdmi_parse_packet(pkt_data) == -1) {
                    fseek(file, -PACKET_SIZE, SEEK_CUR);
                    fwrite(pkt_data[0], 1, PACKET_SIZE, file);
                    fflush(file);
                }
                if (packet_bytes_read != PACKET_SIZE) {
                    fprintf(stderr, "Failed to read full packet data\n");
                    fclose(file);
                    return -1;
                }

                // Here you can process the packet data stored in `buffer`
                // For demonstration, we'll just print a message
                printf("Packet read successfully\n\n");
                printf("************************\n");
                // Reset the pattern counter since we've already read the packet
                pattern_count = 0; // Optionally, you might want to set a different logic here

                // If you only need one packet, you can break the loop here
                // break;
            }
        } else {
            pattern_count = 0; // Reset the counter if the pattern doesn't match
            //memset(buffer, 0, sizeof(buffer));
        }
        if (memcmp(buffer, video_search_pattern, 1) == 0) {
            if (first) {
                *offset = ftell(file) - 4;
                first = false;
            }
            hactive_detect = 1;
            hactive_count++;
            // 如果当前是消隐期，则退出消隐期
            if (hblank_detect) {
                hblank_detect = 0;
                h_b_cnt = hblank_count;
                hblank_count = 0;  // 可选：如果需要在进入活动期时重置消隐计数器
            }
        } else {
            // 如果之前处于活动期，则现在进入消隐期
            if (hactive_detect) {
                h_cnt = hactive_count;
                hactive_detect = 0;
                hblank_detect = 1;
            }
            // 在消隐期内，增加消隐计数器
            if (hblank_detect) {
                hblank_count++;
            }
            // 重置活动计数器
            hactive_count = 0;
        }

    }
    *htotal = h_b_cnt + h_cnt;
    *vtotal = frame_pixel / *htotal / 4;
    *hactive = h_cnt;
    int vblank = *offset / 4 / *htotal;
    *vactive = *vtotal - vblank;
    printf("frame_pixel = %ld\n", frame_pixel);
    printf("hcnt = %d colordepth = %d\n", h_cnt, gcp_info.sbpkt.colordepth);
    printf("hblank = %d\n", h_b_cnt);
    printf("vactive = %d\n", *vactive);
    printf("vblank = %d\n", vblank);
    printf("offset = %ld\n", *offset);
    if(avi_info.cont.v4.colorindicator == 3)
        q_htotal = h_b_cnt / 2 + h_cnt;
    else
        q_htotal = *htotal;
    q_vtotal = *vtotal;
    q_vactive = *vactive;
    q_hactive = *hactive;
    q_colordepth = gcp_info.sbpkt.colordepth;
    q_colorspace = avi_info.cont.v4.colorindicator;
    fclose(file);
    return 0;
}

int hdmi_generate_bmpfile(char *filename, char* outputimage_file, int *htotal, int *vtotal, int *hactive, int *vactive)
{
    int start_col;
    int num_cols;
    int start_row;
    std::string outputImageFile = outputimage_file;
    std::string input_file_path = filename;
    std::ifstream input_file(input_file_path, std::ios::binary);
    if (!input_file) {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    // 输入和输出文件路径
    const std::string output_file_path = "1.bin";
    // 打开输出文件
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open output file." << std::endl;
        input_file.close();
        return -1;
    }
    num_cols = *hactive;
    start_col = *htotal - *hactive;
    start_row = *vtotal - *vactive;
    const std::streamsize bytes_per_row = num_cols * 4;

    for (int row = start_row; row < *vtotal; ++row) {
        input_file.seekg((row * *htotal + start_col) * 4, std::ios::beg);

        std::vector<char> buffer(bytes_per_row);

        input_file.read(buffer.data(), bytes_per_row);

        if (input_file.gcount() != bytes_per_row) {
            break;
        }

        output_file.write(buffer.data(), bytes_per_row);
    }

    input_file.close();
    output_file.close();

    printf("gcp colordepth %d\n", gcp_info.sbpkt.colordepth);
    if (gcp_info.sbpkt.colordepth == 4)
        hdmi_create_bmp_from_8bit_data(output_file_path, outputImageFile, *hactive, *vactive, avi_info.cont.v4.colorindicator);
    else if (gcp_info.sbpkt.colordepth == 5)
        hdmi_create_bmp_from_10bit_data(output_file_path, outputImageFile, *hactive / 10 * 8, *vactive, avi_info.cont.v4.colorindicator);
    else if (gcp_info.sbpkt.colordepth == 6)
        hdmi_create_bmp_from_12bit_data(output_file_path, outputImageFile, *hactive / 12 * 8, *vactive, avi_info.cont.v4.colorindicator);
    return 0;
}

void copy_file(const char *src, const char *dst) {
    FILE *src_file = fopen(src, "rb");
    if (src_file == NULL) {
        perror("fail to open src file");
        exit(EXIT_FAILURE);
    }

    FILE *dst_file = fopen(dst, "wb");
    if (dst_file == NULL) {
        perror("fail to open dst file");
        fclose(src_file);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytes;

    // 从源文件读取数据并写入到目标文件
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
    printf("raw file has been generatred success %s\n", dst);
}

int hdmi_replace(char *filename, char *rgbfile, int *htotal, int *hactive, int *vactive, long *offset)
{
    FILE *raw_file = fopen(filename, "rb+");
    if (!raw_file) {
        perror("Error opening input file");
        return -1;
    }
    FILE *rgb_file = fopen(rgbfile, "rb");
    if (!rgb_file) {
        perror("Error opening input file");
        return -1;
    }
    unsigned char *input_buffer = (unsigned char *)malloc(*hactive * 3);
    unsigned char *output_buffer = (unsigned char *)malloc(*hactive * 4);

    for (int j = 0; j < *vactive; j++) {
        memset(input_buffer, 0, *hactive * 3);
        memset(output_buffer, 0, *hactive * 4);
        fread(input_buffer, 1, *hactive * 3, rgb_file);
        for (int i = 0; i < *hactive; ++i) {
            output_buffer[i * 4] = 0xc0; // 添加0xc0字节
            memcpy(output_buffer + i * 4 + 1, input_buffer + i * 3, 3); // 复制3字节数据
        }
        fseek(raw_file, (*offset) + j * (*htotal * 4), SEEK_SET);
        fwrite(output_buffer, 1, *hactive * 4, raw_file);
    }

    fclose(raw_file);
    fclose(rgb_file);
    free(input_buffer);
    free(output_buffer);
    printf("A frame has been generatred success %s\n", filename);
    return 0;
}

int hdmi_parse(char *input_file, char *output_file) {
    int htotal;
    int hactive;
    int vtotal;
    int vactive;
    long offset;
    int ret;

    ret = hdmi_parse_blank_packet_and_fdet(input_file, &htotal, &hactive, &vtotal, &vactive, &offset);
    if (ret == -1) {
        std::cerr << "Failed to parse blank." << std::endl;
        return -1;
    }
    ret = hdmi_generate_bmpfile(input_file, output_file, &htotal, &vtotal, &hactive, &vactive);
    if (ret == -1) {
        std::cerr << "Failed to gen bmpfile." << std::endl;
        return -1;
    }
    return 0;
}

int hdmi_encode(char *filename, char *rgbfile, char *outputfile)
{
    int htotal;
    int hactive;
    int vtotal;
    int vactive;
    long offset;
    int ret;

    ret = hdmi_parse_blank_packet_and_fdet(filename, &htotal, &hactive, &vtotal, &vactive, &offset);
    if (ret == -1) {
        std::cerr << "Failed to parse blank." << std::endl;
        return -1;
    }
    copy_file(filename, outputfile);
    hdmi_replace(outputfile, rgbfile, &htotal, &hactive, &vactive, &offset);
    return 0;
}
