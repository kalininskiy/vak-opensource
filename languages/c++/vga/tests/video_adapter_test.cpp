//
// Unit tests for Video_Adapter.
//
// Copyright (c) 2026 Serge Vakulenko
//
#include "video_adapter.h"

#include <gtest/gtest.h>

#include <cstring>

#include "vga_font_9x16.h"

//
// Adapter created without window for headless tests.
//
class Video_AdapterTest : public ::testing::Test {
protected:
    Video_Adapter adapter{ false };
};

TEST_F(Video_AdapterTest, TextBufferNonNull)
{
    ASSERT_NE(adapter.text_buffer(), nullptr);
}

TEST_F(Video_AdapterTest, TextBufferSize)
{
    EXPECT_EQ(adapter.text_buffer_size(), Video_Adapter::TEXT_BUFFER_SIZE);
    EXPECT_EQ(adapter.text_buffer_size(), 4000u);
}

TEST_F(Video_AdapterTest, FontBufferNonNull)
{
    ASSERT_NE(adapter.font_buffer(), nullptr);
}

TEST_F(Video_AdapterTest, FontBufferSize)
{
    EXPECT_EQ(adapter.font_buffer_size(), Video_Adapter::FONT_BUFFER_SIZE);
    EXPECT_EQ(adapter.font_buffer_size(), 8192u);
}

TEST_F(Video_AdapterTest, HasNoWindowWhenCreatedWithoutWindow)
{
    EXPECT_FALSE(adapter.has_window());
}

TEST_F(Video_AdapterTest, PutcharWritesToBufferAndAdvancesCursor)
{
    const uint8_t ch   = 'A';
    const uint8_t attr = 0x07;
    adapter.putchar(ch, attr);
    const uint8_t *buf = adapter.text_buffer();
    EXPECT_EQ(buf[0], ch);
    EXPECT_EQ(buf[1], attr);
    unsigned col, row;
    adapter.get_cursor(col, row);
    EXPECT_EQ(col, 1u);
    EXPECT_EQ(row, 0u);
}

TEST_F(Video_AdapterTest, PutsWritesString)
{
    adapter.set_cursor(0, 0);
    adapter.puts("Hi", 0x0f);
    const uint8_t *buf = adapter.text_buffer();
    EXPECT_EQ(buf[0], 'H');
    EXPECT_EQ(buf[1], 0x0f);
    EXPECT_EQ(buf[2], 'i');
    EXPECT_EQ(buf[3], 0x0f);
    unsigned col, row;
    adapter.get_cursor(col, row);
    EXPECT_EQ(col, 2u);
    EXPECT_EQ(row, 0u);
}

TEST_F(Video_AdapterTest, PutcharWrapsAt80Columns)
{
    adapter.set_cursor(79, 0);
    adapter.putchar('X', 0x07);
    unsigned col, row;
    adapter.get_cursor(col, row);
    EXPECT_EQ(col, 0u);
    EXPECT_EQ(row, 1u);
}

TEST_F(Video_AdapterTest, PutcharScrollsWhenAtBottom)
{
    adapter.set_cursor(0, 24);
    adapter.putchar('Z', 0x07);
    unsigned col, row;
    adapter.get_cursor(col, row);
    EXPECT_EQ(col, 1u);
    EXPECT_EQ(row, 24u);
    const uint8_t *buf = adapter.text_buffer();
    unsigned off       = (24 * Video_Adapter::TEXT_COLS + 0) * 2;
    EXPECT_EQ(buf[off], 'Z');
    EXPECT_EQ(buf[off + 1], 0x07);
}

TEST_F(Video_AdapterTest, SetCursorClampsToValidRange)
{
    adapter.set_cursor(100, 30);
    unsigned col, row;
    adapter.get_cursor(col, row);
    EXPECT_EQ(col, 79u);
    EXPECT_EQ(row, 24u);
}

TEST_F(Video_AdapterTest, FontPreloadMatchesHeaderForChar0)
{
    const uint8_t *font = adapter.font_buffer();
    for (unsigned row = 0; row < 16; row++) {
        uint16_t expected = vga_font_9x16[0][row];
        uint16_t actual =
            static_cast<uint16_t>(font[row * 2]) | (static_cast<uint16_t>(font[row * 2 + 1]) << 8);
        EXPECT_EQ(actual, expected) << "char 0 row " << row;
    }
}

TEST_F(Video_AdapterTest, FontPreloadMatchesHeaderForChar65)
{
    const uint8_t *font = adapter.font_buffer();
    unsigned off        = 65 * 32;
    for (unsigned row = 0; row < 16; row++) {
        uint16_t expected = vga_font_9x16[65][row];
        uint16_t actual   = static_cast<uint16_t>(font[off + row * 2]) |
                          (static_cast<uint16_t>(font[off + row * 2 + 1]) << 8);
        EXPECT_EQ(actual, expected) << "char 65 row " << row;
    }
}

TEST_F(Video_AdapterTest, RefreshUpdatesSnapshot)
{
    uint8_t *buf = adapter.text_buffer();
    buf[0]       = 'X';
    buf[1]       = 0x0e;
    adapter.refresh();
    const uint8_t *snap = adapter.text_buffer();
    EXPECT_EQ(snap[0], 'X');
    EXPECT_EQ(snap[1], 0x0e);
    adapter.refresh();
    EXPECT_EQ(snap[0], 'X');
    EXPECT_EQ(snap[1], 0x0e);
}

TEST_F(Video_AdapterTest, DirectMemoryWriteVisibleAfterRefresh)
{
    uint8_t *buf = adapter.text_buffer();
    buf[10]      = 'D';
    buf[11]      = 0x0a;
    adapter.refresh();
    EXPECT_EQ(buf[10], 'D');
    EXPECT_EQ(buf[11], 0x0a);
}

TEST_F(Video_AdapterTest, AttributeHighBitExtendsBackground)
{
    adapter.set_cursor(0, 0);
    // Foreground = 7, Background = 8 (uses high bit of attribute byte).
    adapter.putchar('B', 0x87);
    const uint8_t *buf = adapter.text_buffer();
    EXPECT_EQ(buf[1], 0x87u);
}
