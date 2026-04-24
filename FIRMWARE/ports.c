#include "ports.h"

/* ポート定義を別ファイルに分割 (元の board_pins 配列) */
PinDef board_pins[] = {
//-Rev B
//   {1,  DIR_OUTPUT, "CS1-12"},       // V2 PCBで追加    
    {2,  DIR_OUTPUT, "SLT_PWR"},
    {3,  DIR_INPUT,  "PWR_FLG"},
//-Rev B
//    {4,  DIR_INPUT,  "SLT_SW1"},
//    {5,  DIR_INPUT,  "SLT_SW2"},
//    {6,  DIR_OUTPUT, "LED_DOUT"},     // Neopixel (GP6)

//Rev B2-
    {4,  DIR_OUTPUT, "LED_DOUT"},     // Neopixel (GP6)
    {5,  DIR_INPUT,  "SLT_SW1"},

// -Rev B
//    {7,  DIR_OUTPUT, "PSG_CS"},
//    {8,  DIR_INPUT,  "CLOCK"},
//    {9,  DIR_OUTPUT, "SLT_A0"},
//    {10, DIR_OUTPUT, "SLT_A1"},
//    {11, DIR_OUTPUT, "SLT_A2"},
//    {12, DIR_OUTPUT, "SLT_A3"},
//    {13, DIR_OUTPUT, "SLT_A4"},
//    {14, DIR_OUTPUT, "SLT_A5"},
//    {15, DIR_OUTPUT, "SLT_A6"},
//    {16, DIR_OUTPUT, "SLT_A7"},
//    {17, DIR_OUTPUT, "SLT_A8"},
//    {18, DIR_OUTPUT, "SLT_A9"},
//    {19, DIR_OUTPUT, "SLT_A10"},
//    {20, DIR_OUTPUT, "SLT_A11"},
//    {21, DIR_OUTPUT, "SLT_A12"},
//    {22, DIR_OUTPUT, "SLT_A13"},
//    {23, DIR_OUTPUT, "SLT_A14"},
//    {24, DIR_OUTPUT, "SLT_A15"},
//    {25, DIR_OUTPUT, "SLT_SEL1"},            //V2 PCBではSLT_SEL1
//    {26, DIR_OUTPUT, "CS1-1"},       //CS1-1
//    {27, DIR_OUTPUT, "CS1-2"},       //CS1-2
//    {28, DIR_OUTPUT, "SLT_SEL2"},            //SLT_SEL2
//    {29, DIR_OUTPUT, "CS2-1"},            //CS2-1
//    {30, DIR_OUTPUT, "CS2-2"},           //CS2-2

// RevB2-
    {7,  DIR_OUTPUT,  "CLOCK"},
    {8,  DIR_OUTPUT, "SLT_A0"},
    {9,  DIR_OUTPUT, "SLT_A1"},
    {10, DIR_OUTPUT, "SLT_A2"},
    {11, DIR_OUTPUT, "SLT_A3"},
    {12, DIR_OUTPUT, "SLT_A4"},
    {13, DIR_OUTPUT, "SLT_A5"},
    {14, DIR_OUTPUT, "SLT_A6"},
    {15, DIR_OUTPUT, "SLT_A7"},
    {16, DIR_OUTPUT, "SLT_A8"},
    {17, DIR_OUTPUT, "SLT_A9"},
    {18, DIR_OUTPUT, "SLT_A10"},
    {19, DIR_OUTPUT, "SLT_A11"},
    {20, DIR_OUTPUT, "SLT_A12"},
    {21, DIR_OUTPUT, "SLT_A13"},
    {22, DIR_OUTPUT, "SLT_A14"},
    {23, DIR_OUTPUT, "SLT_A15"},
    {24, DIR_OUTPUT, "SLT_SEL1"},
    {25, DIR_OUTPUT, "CS1-12"},
    {26, DIR_OUTPUT, "CS1-1"},
    {27, DIR_OUTPUT, "CS1-2"},
    {28, DIR_OUTPUT, ""},
    {29, DIR_OUTPUT, ""},            //CS2-1
    {30, DIR_OUTPUT, ""},           //CS2-2


    //    {25, DIR_OUTPUT, "LED"},            //V2 PCBではSLT_SEL1
//    {26, DIR_OUTPUT, "SLT_SEL1"},       //CS1-1
//    {27, DIR_OUTPUT, "SLT_SEL2"},       //CS1-2
//    {28, DIR_OUTPUT, "CS1"},            //SLT_SEL2
//    {29, DIR_OUTPUT, "CS2"},            //CS2-1
//    {30, DIR_OUTPUT, "CS12"},           //CS2-2
    {31, DIR_OUTPUT, "SLT_RFSH"},
    {32, DIR_INPUT,  "SLT_WAIT"},
    {33, DIR_INPUT,  "SLT_INT"},
    {34, DIR_OUTPUT, "SLT_M1"},
    {35, DIR_OUTPUT, "SLT_IORQ"},
    {36, DIR_OUTPUT, "SLT_MEMRQ"},
    {37, DIR_OUTPUT, "SLT_WR"},
    {38, DIR_OUTPUT, "SLT_RD"},
    {39, DIR_OUTPUT, "SLT_RESET"},
    {40, DIR_INPUT,  "SLT_D0"},
    {41, DIR_INPUT,  "SLT_D1"},
    {42, DIR_INPUT,  "SLT_D2"},
    {43, DIR_INPUT,  "SLT_D3"},
    {44, DIR_INPUT,  "SLT_D4"},
    {45, DIR_INPUT,  "SLT_D5"},
    {46, DIR_INPUT,  "SLT_D6"},
    {47, DIR_INPUT,  "SLT_D7"},
};


const size_t NUM_PINS = sizeof(board_pins)/sizeof(PinDef); // ピン数
