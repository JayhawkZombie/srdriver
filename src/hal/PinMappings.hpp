#pragma once

#include <Arduino.h>
#include <vector>
#include <memory>
#include <map>
#include "config/JsonSettings.h"

#if !defined(D0)
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define A6 20
#define A7 21
#endif

static const std::map<String, int> pinMap = {
    {"D0", D0}, {"D1", D1}, {"D2", D2}, {"D3", D3}, {"D4", D4},
    {"D5", D5}, {"D6", D6}, {"D7", D7}, {"D8", D8}, {"D9", D9}, {"D10", D10},
    {"D11", D11}, {"D12", D12}, {"D13", D13},
    {"A0", A0}, {"A1", A1}, {"A2", A2}, {"A3", A3}, {"A4", A4}, {"A5", A5},
};
