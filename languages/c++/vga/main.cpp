//
// Copyright (c) 2026 Serge Vakulenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <SDL.h>
#include <getopt.h>

#include <cstring>
#include <iostream>

#include "video_adapter.h"

//
// CLI options.
//
static const struct option long_options[] = {
    // clang-format off
    { "help",       no_argument,    nullptr,    'h' },
    { "version",    no_argument,    nullptr,    'V' },
    { "verbose",    no_argument,    nullptr,    'v' },
    {},
    // clang-format on
};

//
// Print usage message.
//
static void print_usage(std::ostream &out, const char *prog_name)
{
    out << "VGA Simulator, Version " VERSION_STRING "\n";
    out << "Usage:" << std::endl;
    out << "    " << prog_name << " [options...]" << std::endl;
    out << "Options:" << std::endl;
    out << "    -h, --help          Display available options" << std::endl;
    out << "    -V, --version       Print the version number and exit" << std::endl;
    out << "    -v, --verbose       Verbose mode" << std::endl;
}

//
// Main routine of the simulator,
// when invoked from a command line.
//
int main(int argc, char *argv[])
{
    // Get the program name.
    const char *prog_name = strrchr(argv[0], '/');
    if (prog_name == nullptr) {
        prog_name = argv[0];
    } else {
        prog_name++;
    }

    // Parse command line options.
    for (;;) {
        switch (getopt_long(argc, argv, "-hVvl:tT:d:rs", long_options, nullptr)) {
        case EOF:
            break;

        case 0:
            continue;

        case 1:
            // Regular argument.
            std::cerr << "Too many arguments: " << optarg << std::endl;
            print_usage(std::cerr, prog_name);
            exit(EXIT_FAILURE);

        case 'h':
            // Show usage message and exit.
            print_usage(std::cout, prog_name);
            exit(EXIT_SUCCESS);

        case 'v':
            // Verbose.
            // TODO
            continue;

        case 'V':
            // Show version and exit.
            std::cout << "Version " << VERSION_STRING << "\n";
            exit(EXIT_SUCCESS);

        default:
            print_usage(std::cerr, prog_name);
            exit(EXIT_FAILURE);
        }
        break;
    }

    Video_Adapter adapter(true);
    if (!adapter.has_window()) {
        std::cerr << "Failed to create SDL window." << std::endl;
        return EXIT_FAILURE;
    }

    // VGA attribute: light gray on black (0x07)
    constexpr uint8_t attr = 0x07;
    adapter.puts("Hello, VGA!", attr);

    // Move to the next line.
    adapter.set_cursor(0, 1);
    adapter.puts("FG/BG color table:", attr);

    // Print a table of all 16 foreground colors (rows) and 16 background
    // colors (columns). Each cell is two characters: the hex digit of the
    // foreground and background indices.
    //
    // Layout:
    //   - Row 2: header with BG hex digits 0..F
    //   - Rows 3..18: one row per FG color, starting at text row 3.
    //
    // This fits into 80x25 text mode comfortably.
    adapter.set_cursor(0, 2);
    adapter.puts("   BG:", attr);
    for (uint8_t bg = 0; bg < 16; ++bg) {
        uint8_t digit = (bg < 10) ? static_cast<uint8_t>('0' + bg)
                                  : static_cast<uint8_t>('A' + (bg - 10));
        adapter.putchar(' ', attr);
        adapter.putchar(digit, attr);
    }

    // One blank line after header.
    adapter.set_cursor(0, 3);

    for (uint8_t fg = 0; fg < 16; ++fg) {
        // Print FG label at the start of the row.
        unsigned col = 0;
        unsigned row = 3 + fg;
        adapter.set_cursor(col, row);

        uint8_t digit = (fg < 10) ? static_cast<uint8_t>('0' + fg)
                                  : static_cast<uint8_t>('A' + (fg - 10));
        adapter.puts("FG ", attr);
        adapter.putchar(digit, attr);
        adapter.puts(":", attr);

        // Now print one colored cell per background.
        for (uint8_t bg = 0; bg < 16; ++bg) {
            // Attribute: lower 4 bits = FG, upper 4 bits = BG (0..15),
            // using bit 7 to extend the number of background colors.
            uint8_t cell_attr =
                static_cast<uint8_t>(((bg & 0x0f) << 4) | (fg & 0x0f));

            adapter.putchar(' ', cell_attr);
            uint8_t bg_digit = (bg < 10) ? static_cast<uint8_t>('0' + bg)
                                         : static_cast<uint8_t>('A' + (bg - 10));
            adapter.putchar(bg_digit, cell_attr);
        }
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }
        adapter.refresh();
        SDL_Delay(50);
    }

    return EXIT_SUCCESS;
}
