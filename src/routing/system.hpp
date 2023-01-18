#ifndef SRC_ROUTING_SYSTEM_HPP_
#define SRC_ROUTING_SYSTEM_HPP_

#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <list>
#include <queue>
#include <map>
#include <exception>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <climits>
#include <set>

namespace routing::backend {


struct Row {
    // store column index
    int horizontal;
    int vertical;
    int next;
    int temp; // temp store
};

struct Column {
    Column() {}
    Column(int top, int bottom) : top_pin(top), bottom_pin(bottom) {}

    int top_pin;
    int bottom_pin;
};

struct Net {
    Net() {
        right = -1;
        left = -1;
        valid = false;
    }
    bool valid;
    int left, right;
    int length;
};

struct System {
    std::vector<Column> columns;
    std::vector<Row> rows;
    int num_columns;
    std::map<int, Net> nets;
    int density;

    std::vector<std::vector<int>> all_vertical_branches;
    std::vector<std::vector<int>> all_horizontal_trunks;
};


struct net {
    std::vector<int> top_column;
    std::vector<int> bottom_column;
    int top_y;
    int bottom_y;
    int col_link_id;
    bool is_locked;
    int top_right;
    int top_left;
    int bottom_right;
    int bottom_left;

    //
    bool is_same_side;
    bool is_top;
    bool is_bottom;
    bool is_selected;
};

}  // namespace routing::backend
#endif  // SRC_ROUTING_SYSTEM_HPP_

